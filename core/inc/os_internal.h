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
***/

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
#if (OS_EVENT_MAX_NUM == 8)
typedef _Atomic(uint8_t) OS_EVENT_FLAG_T;
typedef uint8_t OS_EVENT_MASK_T;
#elif (OS_EVENT_MAX_NUM == 16)
typedef _Atomic(uint16_t) OS_EVENT_FLAG_T;
typedef uint16_t OS_EVENT_MASK_T;
#elif (OS_EVENT_MAX_NUM == 32)
typedef _Atomic(uint32_t) OS_EVENT_FLAG_T;
typedef uint32_t OS_EVENT_MASK_T;
#elif (OS_EVENT_MAX_NUM == 64)
typedef _Atomic(uint64_t) OS_EVENT_FLAG_T;
typedef uint64_t OS_EVENT_MASK_T;
#else
#error "OS_EVENT_MAX_NUM must be 8, 16, 32, or 64."
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
    OS_EVENT_FLAG_T Flag;                   // 就绪事件标志。
} OS_TCB_T;

/**
 * @brief 事件。
 */
typedef struct os_event_t {
    int Id;                         // 事件对应任务的ID。
    OS_EVENT_MASK_T Mask;           // 事件掩码。
} OS_EVENT_T;

/**
 * @brief 定时器类型。
 */
typedef enum os_timeout_type_t {
    OS_TIMEOUT_TYPE_SINGLE = 0,     // 单次定时器，超时触发一次后停止。
    OS_TIMEOUT_TYPE_PERIODIC,       // 周期定时器，每次超时后按定时时长重新装载。
} OS_TIMEOUT_TYPE_T;

/**
 * @brief 定时器。
 * @note 定时器在初始化时与一个事件关联，超时时触发该事件。
 */
typedef struct os_timer_t {
    struct os_timer_t * Next;       // 定时器链表指针。
    OS_EVENT_T * Event;             // 定时器超时后触发的事件。
    OS_TICK_T Timeout;              // 相对于前一个定时器的剩余tick数。
    OS_TICK_T Interval;             // 定时时长，周期定时器超时后按该值重新装载。
    OS_TIMEOUT_TYPE_T Type;         // 定时器类型。
    bool IsRun;                     // 定时器是否正在运行。
} OS_TIMER_T;

#endif // _OS_INTERNAL_H_
