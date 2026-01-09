/**
  ******************************************************************************
  * @file os.c
  * @author lx
  * @version v1.0
  * @date 2021-07-21
  * @brief
 =============================================================================
                     #####  #####
 =============================================================================

  ******************************************************************************
  * @attention
  *
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "os.h"

static OS_TCB * OSTcbBase = NULL;
static OS_EVENT OSTimeoutList = {0};
static uint32_t OSReadyEventGroup;

void OSStart(OS_TCB * tcb, int number)
{
    int index = 0;
    OS_TICK tick;
    OSTaskFunction initFunction;
    OSTaskFunction taskFunction;

    OSTcbBase = tcb;
    OSTimeoutList.Next = NULL;

    for (int i = 0; i < number; i++) {
        tcb[i].Id = i;
        tcb[i].Flag = 0;
        tcb[i].Counter = 0;
        tcb[i].MaxTick = 0;
        if (tcb[i].Init != NULL) {
            initFunction = (OSTaskFunction)tcb[i].Init;
            initFunction(&tcb[i]);
        }
    }
    number = number - 1;

    while (1) {
        OSCirculateBeginHook();

        OSCriticalEnter();
        OSReadyEventGroup = tcb[index].Flag;
        tcb[index].Flag = 0;
        OSCriticalExit();
        if (OSReadyEventGroup != 0) {
            taskFunction = (OSTaskFunction)tcb[index].Task;
            tick = OSTimestampGet();
            taskFunction(&tcb[index]);
            tick = OSTimestampGet() - tick;
            tcb[index].MaxTick = tcb[index].MaxTick < tick ? tick : tcb[index].MaxTick;
            index = 0;
            continue;
        }

        if (index < number) {
            index++;
        } else {
            index = 0;
            OSIdelTask();
        }

        OSCirculateEndHook();
    }
}

bool OSEventBind(OS_TCB * tcb, OS_EVENT * event)
{
    if (tcb->Counter >= OS_EVENT_MAX_NUM) {
        return false;
    }

    if (event->Id >= 0) {
        return false;
    }

    OSCriticalEnter();
    event->Next = NULL;
    event->Timeout = 0;
    event->Id = tcb->Id;
    event->Mask = 1u << tcb->Counter;
    tcb->Counter = tcb->Counter + 1;
    OSCriticalExit();

    return true;
}

bool OSEventBindISR(OS_TCB * tcb, OS_EVENT * event)
{
    if (tcb->Counter >= OS_EVENT_MAX_NUM) {
        return false;
    }

    if (event->Id >= 0) {
        return false;
    }

    event->Next = NULL;
    event->Timeout = 0;
    event->Id = tcb->Id;
    event->Mask = 1u << tcb->Counter;
    tcb->Counter = tcb->Counter + 1;

    return true;
}

void OSEventPost(OS_EVENT * event)
{
    if (event->Id < 0) {
        return;
    }
    OSCriticalEnter();
    OSTcbBase[event->Id].Flag = OSTcbBase[event->Id].Flag | event->Mask;
    OSCriticalExit();
}

void OSEventPostISR(OS_EVENT * event)
{
    if (event->Id < 0) {
        return;
    }
    OSTcbBase[event->Id].Flag = OSTcbBase[event->Id].Flag | event->Mask;
}

bool OSEventAssert(OS_EVENT * event) { return ((OSReadyEventGroup & event->Mask) == event->Mask); }

OS_TICK OSTickGetMini(void)
{
    OS_EVENT * pEvent = &OSTimeoutList;

    if (pEvent->Next != NULL) {
        return pEvent->Next->Timeout;
    } else {
        return 0;
    }
}

void OSTickHandle(OS_TICK tick)
{
    OS_EVENT * pTemp;
    OS_EVENT * pEvent = &OSTimeoutList;

    if (pEvent->Next == NULL || tick == 0) {
        return;
    }

    OSCriticalEnter();

    while (pEvent->Next != NULL) {
        if (pEvent->Next->Timeout <= tick) {
            tick = tick - pEvent->Next->Timeout;
            OSTcbBase[pEvent->Next->Id].Flag =
                OSTcbBase[pEvent->Next->Id].Flag | pEvent->Next->Mask;
            pTemp = pEvent->Next;
            pEvent->Next = pEvent->Next->Next;
            pTemp->Next = NULL;
            pTemp->Timeout = 0;
        } else {
            pEvent->Next->Timeout = pEvent->Next->Timeout - tick;
            break;
        }
    }

    OSCriticalExit();
}

void OSTickHandleISR(OS_TICK tick)
{
    OS_EVENT * pTemp;
    OS_EVENT * pEvent = &OSTimeoutList;

    if (pEvent->Next == NULL || tick == 0) {
        return;
    }

    while (pEvent->Next != NULL) {
        if (pEvent->Next->Timeout <= tick) {
            tick = tick - pEvent->Next->Timeout;
            OSTcbBase[pEvent->Next->Id].Flag =
                OSTcbBase[pEvent->Next->Id].Flag | pEvent->Next->Mask;
            pTemp = pEvent->Next;
            pEvent->Next = pEvent->Next->Next;
            pTemp->Next = NULL;
            pTemp->Timeout = 0;
        } else {
            pEvent->Next->Timeout = pEvent->Next->Timeout - tick;
            break;
        }
    }
}

void OSTimeoutStart(OS_EVENT * event, OS_TICK tick)
{
    OS_TICK base = 0;
    OS_EVENT * pEvent = &OSTimeoutList;

    OSCriticalEnter();

    // 定时器已经在运行，则先将其从运行链表中取出。
    if (event->Timeout > 0) {
        while (pEvent->Next != NULL && pEvent->Next != event) {
            pEvent = pEvent->Next;
        }
        pEvent->Next = pEvent->Next->Next;
        pEvent = pEvent->Next;
        if (pEvent->Next != NULL) {
            pEvent->Next->Timeout = pEvent->Next->Timeout + event->Timeout;
        }
        event->Next = NULL;
        event->Timeout = 0;
        pEvent = &OSTimeoutList;
    }

    // 在运行链表中接到适合定时器的位置。
    while (pEvent->Next != NULL) {
        base = base + pEvent->Next->Timeout;
        if (base >= tick) {
            break;
        } else {
            pEvent = pEvent->Next;
        }
    }

    if (pEvent->Next == NULL) {
        event->Timeout = tick - base;
        event->Next = NULL;
        pEvent->Next = event;
    } else {
        event->Timeout = tick - (base - pEvent->Next->Timeout);
        pEvent->Next->Timeout = base - tick;
        event->Next = pEvent->Next;
        pEvent->Next = event;
    }

    OSCriticalExit();
}

void OSTimeoutStartISR(OS_EVENT * event, OS_TICK tick)
{
    OS_TICK base = 0;
    OS_EVENT * pEvent = &OSTimeoutList;

    // 定时器已经在运行，则先将其从运行链表中取出。
    if (event->Timeout > 0) {
        while (pEvent->Next != NULL && pEvent->Next != event) {
            pEvent = pEvent->Next;
        }
        pEvent->Next = pEvent->Next->Next;
        pEvent = pEvent->Next;
        if (pEvent->Next != NULL) {
            pEvent->Next->Timeout = pEvent->Next->Timeout + event->Timeout;
        }
        event->Next = NULL;
        event->Timeout = 0;
        pEvent = &OSTimeoutList;
    }

    // 在运行链表中接到适合定时器的位置。
    while (pEvent->Next != NULL) {
        base = base + pEvent->Next->Timeout;
        if (base >= tick) {
            break;
        } else {
            pEvent = pEvent->Next;
        }
    }

    if (pEvent->Next == NULL) {
        event->Timeout = tick - base;
        event->Next = NULL;
        pEvent->Next = event;
    } else {
        event->Timeout = tick - (base - pEvent->Next->Timeout);
        pEvent->Next->Timeout = base - tick;
        event->Next = pEvent->Next;
        pEvent->Next = event;
    }
}

void OSTimeoutStop(OS_EVENT * event)
{
    OS_EVENT * pEvent = &OSTimeoutList;

    if (event->Timeout == 0) {
        return;
    }

    OSCriticalEnter();

    while (pEvent->Next != NULL && pEvent->Next != event) {
        pEvent = pEvent->Next;
    }
    pEvent->Next = pEvent->Next->Next;
    pEvent = pEvent->Next;
    if (pEvent->Next != NULL) {
        pEvent->Next->Timeout = pEvent->Next->Timeout + event->Timeout;
    }
    event->Next = NULL;
    event->Timeout = 0;

    OSCriticalExit();
}

void OSTimeoutStopISR(OS_EVENT * event)
{
    OS_EVENT * pEvent = &OSTimeoutList;

    if (event->Timeout == 0) {
        return;
    }

    while (pEvent->Next != NULL && pEvent->Next != event) {
        pEvent = pEvent->Next;
    }
    pEvent->Next = pEvent->Next->Next;
    pEvent = pEvent->Next;
    if (pEvent->Next != NULL) {
        pEvent->Next->Timeout = pEvent->Next->Timeout + event->Timeout;
    }
    event->Next = NULL;
    event->Timeout = 0;
}

__WEAK void OSCirculateBeginHook(void) {}
__WEAK void OSCirculateEndHook(void) {}

__WEAK void OSCriticalEnter(void) {}
__WEAK void OSCriticalExit(void) {}

__WEAK void OSIdelTask(void) {}

__WEAK OS_TICK OSTimestampGet(void) { return 0; }
