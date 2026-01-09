/**
  ******************************************************************************
  * @file os.h
  * @author lx
  * @version v1.0
  * @date 2021-07-21
  * @brief
 =============================================================================
                     #####  #####
 =============================================================================

  ******************************************************************************
  * @attention
  * 1、带有ISR后缀的接口函数可以在中断中调用，其与正常函数的唯一区别是在操作临界代码时，没有调用临界保护函数。
  * 2、临界区保护函数需要用户结合需求情况选择是否重新定义；
  *
  ******************************************************************************
  */

#ifndef _OS_H_
#define _OS_H_

/* Includes ------------------------------------------------------------------*/
#include "os_internal.h"

/**
 * @brief 声明任务数组时使用，用于将任务函数声明成任务控制块。
 * @note
 * @param __INIT__：任务的初始化函数，如不需要可以传NULL；__TASK__：任务函数。
 * @retval
 */
#define __OS_TASK_INSERT(__INIT__, __TASK__)                                                       \
    ((OS_TCB_T){.Init = (void *)(__INIT__), .Task = (void *)(__TASK__)})

/**
 * @brief 声明一个事件。
 * @note
 * @param __NAME__：事件变量名。
 * @retval
 */
#define __OS_EVENT_ALLOC(__NAME__) OS_EVENT_T __NAME__ = {.Id = -1, .Timeout = 0}

/**
 * @brief 任务函数。
 * @note
 * @param tcb：对应的任务控制块；flag。
 * @retval
 */
typedef void (*OSTaskFunction_T)(OS_TCB_T * tcb);

/**
 * @brief 弱类型函数。临界区保护函数。
 * @note 当需要临界区保护时，用户需根据需求重定义。
 * @param
 * @retval
 */
void OSCriticalEnter(void);
void OSCriticalExit(void);

/**
 * @brief 启动系统。
 * @note
 * @param tcb：任务控制块首地址；number：任务数目。
 * @retval
 */
void OSStart(OS_TCB_T * tcb, int number);

/**
 * @brief 弱类型函数。系统循环的钩子函数。
 * @note 在每次系统循环开始和结束时，会调用它们。
 * @param
 * @retval
 */
void OSCirculateBeginHook(void);
void OSCirculateEndHook(void);

/**
 * @brief 弱类型函数。在一次系统循环中，如果没有任务被激活则会调用该函数。
 * @note
 * @param
 * @retval
 */
void OSIdelTask(void);

/**
 * @brief 将一个事件与目标任务绑定。
 * @note 事件必须绑定才能发送，且一个事件在任何时刻最多只能与一个任务绑定；再次执行绑定，将自动解除前面的绑定。
 * @param tcb：当前任务的任务控制块；event：事件结构体指针。
 * @retval 创建成功，返回true；创建失败，返回false。
 */
bool OSEventBind(OS_TCB_T * tcb, OS_EVENT_T * event);
bool OSEventBindISR(OS_TCB_T * tcb, OS_EVENT_T * event);

/**
 * @brief 发送一个事件。
 * @note 事件必须绑定才能发送。
 * @param event：事件。
 * @retval
 */
void OSEventPost(OS_EVENT_T * event);
void OSEventPostISR(OS_EVENT_T * event);

/**
 * @brief 启动一个定时器。
 * @note 定时器均为单次触发，无周期定时器；对一个已经启动的定时器调用该函数，则该定时器会重新开始计时。
 * @param event：用于超时的事件；tick：超时的滴答数。
 * @retval
 */
void OSTimeoutStart(OS_EVENT_T * event, OS_TICK_T tick);
void OSTimeoutStartISR(OS_EVENT_T * event, OS_TICK_T tick);

/**
 * @brief 停止一个定时器。
 * @note 对一个没有运行的定时器调用该函数，不会产生影响。
 * @param event：用于超时的事件。
 * @retval
 */
void OSTimeoutStop(OS_EVENT_T * event);
void OSTimeoutStopISR(OS_EVENT_T * event);

/**
 * @brief 系统滴答处理函数。
 * @note
 * @param tick：相距于上次调用，间隔的滴答数。
 * @retval
 */
void OSTickHandle(OS_TICK_T tick);
void OSTickHandleISR(OS_TICK_T tick);

/**
 * @brief 断言任务是否被某一事件激活。
 * @note
 * @param event：用于断言的事件。
 * @retval 断言结果，任务若是被这一事件激活，则返回true，若不然，则返回false。
 */
bool OSEventAssert(OS_EVENT_T * event);

/**
 * @brief 查询定时器链表里，第一个定时器的剩余滴答数。
 * @note
 * @param
 * @retval 剩余滴答数，若链表为空则返回0。
 */
OS_TICK_T OSTickGetMini(void);

/**
 * @brief 弱类型函数。获得时间戳，用于统计任务最大tick开销。
 * @note
 * @param
 * @retval
 */
OS_TICK_T OSTimestampGet(void);

#endif // _OS_H_
