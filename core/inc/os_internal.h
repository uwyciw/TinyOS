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
#include <stdatomic.h>

/**
 * @brief 每个任务支持的最大事件数目。
 */
#define OS_EVENT_MAX_NUM 32
#if (OS_EVENT_MAX_NUM == 32)
#define OS_EVENT_MAX_NUM_32
#elif (OS_EVENT_MAX_NUM == 64)
#define OS_EVENT_MAX_NUM_64
#else
#error "OS_EVENT_MAX_NUM must be 32 or 64."
#endif

 /**
  * @brief tick类型。
  */
typedef uint32_t OS_TICK_T;

/**
 * @brief 任务控制块。
 */
typedef struct os_tcb_t {
    int Id;                                 // 任务ID。
    int Counter;                            // 此任务已经分配的事件数目。
    OS_TICK_T MaxTick;                      // 任务花费的最大的tick数。
    void (*Init)(struct os_tcb_t * tcb);    // 初始化函数。
    void (*Task)(struct os_tcb_t * tcb);    // 任务函数。
    #ifdef OS_EVENT_MAX_NUM_32
        atomic_uint_least32_t Flag;         // 就绪事件标志。
    #else
        atomic_uint_least64_t Flag;         // 就绪事件标志。
    #endif
} OS_TCB_T;

/**
 * @brief 事件。
 */
typedef struct os_event_t {
    int Id;                         // 事件对应任务的ID。
    OS_TICK_T Timeout;              // 事件超时。
    struct os_event_t * Next;       // 事件链表指针。
    #ifdef OS_EVENT_MAX_NUM_32
        uint32_t Mask;              // 事件掩码。
    #else
        uint64_t Mask;              // 事件掩码。
    #endif
} OS_EVENT_T;

#endif // _OS_INTERNAL_H_
