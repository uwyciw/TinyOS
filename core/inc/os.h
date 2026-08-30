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
  * 除OSEventPost外，其他函数仅建议在无并发环境下使用，否则会有数据竞争风险，需要增加保护机制。
  *
  ******************************************************************************
***/

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
#define __OS_TASK_INSERT(__INIT__, __TASK__) ((OS_TCB_T){.Init = __INIT__, .Task = __TASK__})

/**
 * @brief 声明一个事件。
 * @note
 * @param __NAME__：事件变量名。
 * @retval
 */
#define __OS_EVENT_ALLOC(__NAME__) OS_EVENT_T __NAME__ = {.Id = -1}

/********************************************************************************
 *                          内核API
 ********************************************************************************/
/**
 * @brief 启动系统。
 * @note
 * @param tcb：任务控制块首地址；number：任务数目。
 * @retval
 */
void OSStart(OS_TCB_T * tcb, int number);

/**
 * @brief 将一个事件与目标任务绑定。
 * @note 事件必须绑定才能发送，且事件一旦与任务绑定，就不能再与其他任务绑定。
 * @param tcb：当前任务的任务控制块；event：事件结构体指针。
 * @retval 绑定成功，返回true；绑定失败，返回false。
 */
bool OSEventBind(OS_TCB_T * tcb, OS_EVENT_T * event);

/**
 * @brief 发送一个事件。
 * @note 事件必须绑定才能发送。
 * @param event：事件。
 * @retval
 */
void OSEventPost(const OS_EVENT_T * event);

/**
 * @brief 启动一个定时器。
 * @note 定时器均为单次触发，无周期定时器；对一个已经启动的定时器调用该函数，则该定时器会重新开始计时。
 * @param event：用于定时的事件；tick：定时的滴答数。
 * @retval
 */
void OSTimerStart(OS_EVENT_T * event, OS_TICK_T tick);

/**
 * @brief 停止一个定时器。
 * @note 对一个没有运行的定时器调用该函数，不会产生影响。
 * @param event：用于定时的事件。
 * @retval
 */
void OSTimerStop(OS_EVENT_T * event);

/**
 * @brief 断言任务是否被某一事件激活。
 * @note
 * @param event：用于断言的事件。
 * @retval 断言结果，任务若是被这一事件激活，则返回true，若不然，则返回false。
 */
bool OSEventAssert(const OS_EVENT_T * event);

/**
 * @brief 查询定时链表里，确定还需要多少个tick就会有定时器触发。
 * @note
 * @param
 * @retval 还需要的tick数，若链表为空则返回0。
 */
OS_TICK_T OSGetNeededTick(void);

/**
 * @brief 查询任务在运行过程中，消耗tick的峰值。
 * @note
 * @param tcb：要查询的任务。
 * @retval 消耗tick的峰值。
 */
OS_TICK_T OSGetMaxTick(const OS_TCB_T * tcb);

/********************************************************************************
*                          内核运行中会调用到的扩展接口
********************************************************************************/
/**
 * @brief 弱类型函数。在一次系统循环中，如果没有任务被激活则会调用该函数。
 * @note
 * @param
 * @retval
 */
void OSIdelTask(void);

/**
 * @brief 弱类型函数。内核使用该函数获得当前tick，并用于定时和统计任务最大tick开销。
 * @note 如果用户需要定时器或统计各任务在运行过程中消耗tick的峰值，则需要提供该函数。
 * @param
 * @retval
 */
OS_TICK_T OSGetTick(void);

#endif // _OS_H_
