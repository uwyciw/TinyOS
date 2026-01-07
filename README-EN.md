# TinyOS

中文说明[在此](README-CN.md)。

TinyOS is a lightweight kernel that features non-preemptive scheduling, weak priority levels, and a single stack space shared by all tasks. Its design purpose is to provide an OS kernel for scenarios where requirements for real-time performance and concurrency are not stringent, but there is a need to split the overall business logic into multiple relatively independent tasks, along with basic event interaction demands between tasks, and between tasks and external inputs.

## Feature

- TinyOS is designed with the core tenet of ultimate compactness, and its resource consumption is extremely close to that of bare-metal programming.
- The kernel does not support task preemption, nor does it allocate separate stack spaces for individual tasks. Each task execution is equivalent to a normal call to the task function, and all task functions are executed sequentially on the main stack.
- Tasks must be declared in the task array before kernel initialization. Their priorities are implicitly defined by their order in the array—the first element in the array has the highest priority. Although this approach sacrifices the flexibility of dynamic task creation, it saves considerable system resources.
- Only one simple event type is provided, which can be used either as a signal-like event or a timer-like event.
- The timing unit only supports tick counts instead of standard time units. Moreover, it is not mandatory to invoke the timing function on every clock tick, making this design highly suitable for low-power consumption scenarios.

## Event

- An event must be bound to a task before it can be used, and once bound, the association cannot be unbound. A single task can be bound to a maximum of 32 events.
- When an event is triggered, the task bound to it enters an active state, and the kernel will schedule the task at an appropriate time.
- Once a task is executed, all events bound to it will revert to the non-triggered state.
- Although the kernel provides only one event type, it can be used either as a signal-like event or a timer-like event.
  - When used as a non-counting signal-like event, the event can be triggered by sending a signal via the `Post` function.
  - When used as a timer-like event, the event can be added to the timer queue via the `Timeout` function, waiting to be triggered upon timeout.
- The kernel provides a dedicated macro `__OS_EVENT_ALLOC` for declaring events.
- The type definition of an event is as follows:
```
typedef struct os_event_t {
    int Id;                   // The event corresponding task ID
    uint32_t Mask;            // Event Mask
    uint32_t Timeout;         // Event Timeout
    struct os_event_t * Next; // Event Linked List Pointer
} OS_EVENT;
```

## Task

- For the sake of ultra-minimalist design, a task is simplified into two functions: an optional initialization function and a mandatory main function.
  - The initialization function is executed once in the order of priority during the kernel startup phase, and it is recommended for performing event binding operations.
  - The main function is called each time an active task is scheduled for execution by the kernel. Since this is merely a normal function call, the lifecycle of local variables within the main function differs from that of local variables in general RTOS task functions, which requires special attention.
- Within a task's main function, the `Assert` function must be called to determine which events activated the task and to perform corresponding processing.
- The kernel provides a dedicated macro `__OS_TASK_INSERT` for declaring tasks.
- Before starting the kernel, it is necessary to first declare a task array and pass it to the kernel during kernel startup. The type of the task array is the task control block OS_TCB.
- The type definition of the task control block is as follows:
```
typedef struct {
    int Id;        // Task ID
    void * Init;   // Initialization Function
    void * Task;   // Task Function
    uint32_t Flag; // Ready Event Flag
    int Counter;   // The number of events allocated to this task
} OS_TCB;
```

## Schedule

- During each scheduling round, the kernel checks whether each task is activated in the order of the task array.
- If a task is detected in the active state, the kernel first copies the trigger state of each event associated with the task, then resets all these events to the non-triggered state, and subsequently executes the task’s main function.
- After the main function of the current task is executed, the kernel restarts the check for activated tasks from the first element of the task array (i.e., the task with the highest priority).

## Timer

- When an event is used as a timer-like event, it functions as a timer.
- All timers are one-shot—there are no periodic timers in the kernel. Tasks can be restarted after a timer expires to achieve periodic timing.

## Sub-directories Overview

```
.
├── core
│   ├── inc
│   │    ├── os.h             # External Interface Header File
│   │    └── os_internal.h    # Internal Header File
│   └── src
│        └── os.c             # Kernel Code
└── _example                  # An example program that runs on ST’s NUCLEO-L053R8, with project code generated using STM32CubeMX
```
