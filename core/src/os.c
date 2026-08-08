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

static OS_TCB_T * gOSTcbBase = NULL;
static OS_EVENT_T gOSTimeoutList = { 0 };
static uint32_t gOSReadyEventGroup;

static void OSTickHandle(OS_TICK_T tick);

void OSStart(OS_TCB_T * tcb, int number)
{
    int index = 0;
    OS_TICK_T currentTick, lastTick;
    OS_TICK_T maxTick;
    
    if (tcb == NULL || number <= 0) {
        return;
    }

    gOSTcbBase = tcb;

    for (int i = 0; i < number; i++) {
        tcb[i].Id = i;
        tcb[i].Flag = 0;
        tcb[i].Counter = 0;
        tcb[i].MaxTick = 0;
        if (tcb[i].Init != NULL) {
            tcb[i].Init(&tcb[i]);
        }
    }

    number = number - 1;
    lastTick = OSGetTick();

    while (1) {
        currentTick = OSGetTick();
        OSTickHandle(currentTick - lastTick);
        lastTick = currentTick;
        gOSReadyEventGroup = atomic_exchange_explicit(&tcb[index].Flag, 0, memory_order_relaxed);
        if (gOSReadyEventGroup != 0) {
            maxTick = OSGetTick();
            tcb[index].Task(&tcb[index]);
            maxTick = OSGetTick() - maxTick;
            tcb[index].MaxTick = tcb[index].MaxTick < maxTick ? maxTick : tcb[index].MaxTick;
            index = 0;
            continue;
        }

        if (index < number) {
            index++;
        } else {
            index = 0;
            OSIdelTask();
        }
    }
}

bool OSEventBind(OS_TCB_T * tcb, OS_EVENT_T * event)
{
    if (tcb->Counter >= OS_EVENT_MAX_NUM || event->Id >= 0) {
        return false;
    }

    event->Id = tcb->Id;
    event->Mask = 1u << tcb->Counter;
    tcb->Counter = tcb->Counter + 1;

    return true;
}

void OSEventPost(const OS_EVENT_T * event)
{
    if (event->Id >= 0) {
        atomic_fetch_or_explicit(&gOSTcbBase[event->Id].Flag, event->Mask, memory_order_relaxed);
    }
}

bool OSEventAssert(const OS_EVENT_T * event)
{
    if (event->Id >= 0) {
        return ((gOSReadyEventGroup & event->Mask) == event->Mask);
    } else {
        return false;
    }
}

OS_TICK_T OSGetNeededTick(void)
{
    OS_EVENT_T * pEvent = &gOSTimeoutList;

    if (pEvent->Next != NULL) {
        return pEvent->Next->Timeout;
    } else {
        return 0;
    }
}

OS_TICK_T OSGetMaxTick(const OS_TCB_T * tcb)
{
    return tcb->MaxTick;
}

void OSTimerStart(OS_EVENT_T * event, OS_TICK_T tick)
{
    OS_TICK_T base = 0;
    OS_EVENT_T * pEvent = &gOSTimeoutList;
    
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
        pEvent = &gOSTimeoutList;
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

void OSTimerStop(OS_EVENT_T * event)
{
    OS_EVENT_T * pEvent = &gOSTimeoutList;
    
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

__WEAK void OSIdelTask(void) {}

__WEAK OS_TICK_T OSGetTick(void) { return 0; }

/**
 * @brief 系统滴答处理函数。
 * @note
 * @param tick：相距于上次调用，间隔的滴答数。
 * @retval
 */
static void OSTickHandle(OS_TICK_T tick)
{
    OS_EVENT_T * pTemp;
    OS_EVENT_T * pEvent = &gOSTimeoutList;

    if (pEvent->Next == NULL || tick == 0) {
        return;
    }

    while (pEvent->Next != NULL) {
        if (pEvent->Next->Timeout <= tick) {
            atomic_fetch_or_explicit(&gOSTcbBase[pEvent->Next->Id].Flag, pEvent->Next->Mask, memory_order_relaxed);
            tick = tick - pEvent->Next->Timeout;
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
