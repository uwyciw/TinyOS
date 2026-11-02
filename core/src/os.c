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
  * 除OSEventPost外，其他函数仅建议在无并发环境下使用，否则会有数据竞争风险，需要增加保护机制。
  *
  ******************************************************************************
***/

/* Includes ------------------------------------------------------------------*/
#include "os.h"

static OS_TCB_T * gOSTcbBase = NULL;
static OS_TIMER_T gOSTimeoutList = { 0 };
static OS_EVENT_MASK_T gOSReadyEventGroup;

static void OSTimerInsert(OS_TIMER_T * timer, OS_TICK_T tick);
static void OSTimerRemove(OS_TIMER_T * timer);
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
    if (gOSTimeoutList.Next != NULL) {
        return gOSTimeoutList.Next->Timeout;
    } else {
        return 0;
    }
}

OS_TICK_T OSGetMaxTick(const OS_TCB_T * tcb)
{
    return tcb->MaxTick;
}

void OSTimerInit(OS_TIMER_T * timer, OS_EVENT_T * event, OS_TIMEOUT_TYPE_T type)
{
    if (event == NULL || timer == NULL) {
        return;
    }

    if (timer->IsRun == true) {
        OSTimerRemove(timer);
    }

    timer->Next = NULL;
    timer->Event = event;
    timer->Timeout = 0;
    timer->Interval = 0;
    timer->Type = type;
    timer->IsRun = false;
}

void OSTimerStart(OS_TIMER_T * timer, OS_TICK_T tick)
{
    if (timer == NULL || timer->Event == NULL) {
        return;
    }

    // 定时器已经在运行，则先将其从运行链表中取出。
    if (timer->IsRun == true) {
        OSTimerRemove(timer);
    }

    timer->Interval = tick;
    OSTimerInsert(timer, tick);
}

void OSTimerStop(OS_TIMER_T * timer)
{
    if (timer == NULL || timer->IsRun == false) {
        return;
    }

    OSTimerRemove(timer);
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
    OS_TIMER_T * pExpired;
    OS_TIMER_T * pTimer = &gOSTimeoutList;

    if (pTimer->Next == NULL || tick == 0) {
        return;
    }

    while (pTimer->Next != NULL) {
        if (pTimer->Next->Timeout > tick) {
            pTimer->Next->Timeout = pTimer->Next->Timeout - tick;
            break;
        }

        tick = tick - pTimer->Next->Timeout;
        pExpired = pTimer->Next;
        pTimer->Next = pExpired->Next;
        pExpired->Next = NULL;
        pExpired->IsRun = false;
        OSEventPost(pExpired->Event);

        // 周期定时器重新装载，若剩余tick足够，会在本轮内再次超时。
        if (pExpired->Type == OS_TIMEOUT_TYPE_PERIODIC && pExpired->Interval > 0) {
            OSTimerInsert(pExpired, pExpired->Interval);
        }
    }
}

/**
 * @brief 将定时器按超时先后插入运行链表。
 * @note tick为相对于当前时刻的定时时长，插入后定时器的Timeout为相对于前一个定时器的剩余tick数。
 * @param timer：要插入的定时器；tick：相对于当前时刻的定时时长。
 * @retval
 */
static void OSTimerInsert(OS_TIMER_T * timer, OS_TICK_T tick)
{
    OS_TICK_T base = 0;
    OS_TIMER_T * pTimer = &gOSTimeoutList;

    while (pTimer->Next != NULL) {
        base = base + pTimer->Next->Timeout;
        if (base >= tick) {
            break;
        } else {
            pTimer = pTimer->Next;
        }
    }

    if (pTimer->Next == NULL) {
        timer->Timeout = tick - base;
        timer->Next = NULL;
        pTimer->Next = timer;
    } else {
        timer->Timeout = tick - (base - pTimer->Next->Timeout);
        pTimer->Next->Timeout = base - tick;
        timer->Next = pTimer->Next;
        pTimer->Next = timer;
    }

    timer->IsRun = true;
}

/**
 * @brief 将一个正在运行的定时器从运行链表中取出。
 * @note
 * @param timer：要取出的定时器。
 * @retval
 */
static void OSTimerRemove(OS_TIMER_T * timer)
{
    OS_TIMER_T * pTimer = &gOSTimeoutList;

    while (pTimer->Next != NULL && pTimer->Next != timer) {
        pTimer = pTimer->Next;
    }

    if (pTimer->Next == NULL) {
        timer->IsRun = false;
        return;
    }

    pTimer->Next = timer->Next;
    if (timer->Next != NULL) {
        timer->Next->Timeout = timer->Next->Timeout + timer->Timeout;
    }
    timer->Next = NULL;
    timer->IsRun = false;
}
