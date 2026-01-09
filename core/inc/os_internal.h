/**
  ******************************************************************************
  * @file os_internal.h
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

#ifndef _OS_INTERNAL_H_
#define _OS_INTERNAL_H_

/* Includes ------------------------------------------------------------------*/
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

/**
 * @brief 每个任务支持的最大事件数目。
 */
#define OS_EVENT_MAX_NUM 32

/**
 * @brief tick类型。
 */
typedef uint32_t OS_TICK;

/**
 * @brief 任务控制块。
 */
typedef struct {
    int Id;          // 任务ID。
    void * Init;     // 初始化函数。
    void * Task;     // 任务函数。
    uint32_t Flag;   // 就绪事件标志。
    int Counter;     // 此任务已经分配的事件数目。
    OS_TICK MaxTick; // 任务花费的最大的tick数。
} OS_TCB;

/**
 * @brief 事件。
 */
typedef struct os_event_t {
    int Id;                   // 事件对应任务的ID。
    uint32_t Mask;            // 事件掩码。
    OS_TICK Timeout;          // 事件超时。
    struct os_event_t * Next; // 事件链表指针。
} OS_EVENT;

/**
 * @brief 弱类型符号定义。
 */
#ifndef __WEAK
#if defined(__ICCARM__)
#if __ICCARM_V8
#define __WEAK __attribute__((weak))
#else
#define __WEAK _Pragma("__weak")
#endif
#else
#define __WEAK __attribute__((weak))
#endif
#endif

#endif // _OS_INTERNAL_H_
