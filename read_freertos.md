# introduction to freertos

## 2 freertos kernel distribution

**freertos port**: combination of compiler and processor

**FreeRTOSConfig.h**: kernel configuration, dependency of `FreeRTOS.h`

**common source files**:

| name            | necessity      | description                                     |
| --------------- | -------------- | ----------------------------------------------- |
| tasks.c         | required       | ?                                               |
| list.c          | required       | ?                                               |
| queue.c         | often required | queue and semaphore services                    |
| timers.c        | optional       | software timers                                 |
| event_groups.c  | optional       | ?                                               |
| stream_buffer.c | optional       | ?                                               |
| croutine.c      | rarely used    | coroutines for extremely small microcontrollers |

**port-specific source files**:

- portable/< compiler >/< architecture >/
- portale/MemMang/: impl of heap allocation schemes

**user defined types**:

- TickType_t: type of tick count which is used to measure time. configured by `configTICK_TYPE_WIDTH_IN_BITS`. needed to comply to arch's bits
- BaseType_t: with bits often equals to arch's bits

> deprecation: configUSE_16_BIT_TICKS is deprecated by configTICK_TYPE_WIDTH_IN_BITS

**naming convention**:

- (leading) 'p': pointer type
- (leading) 'u': unsigned type
- 'v': void
- 'c': char
- 's': short
- 'l': long
- 'x': BaseType_t, struct, handles, etc.
- functions: < ret type >< **S**rc file name >< **F**unctionality >
- private functions: < prv >...
- macros: (port|task|pd|config|err)< NAME >

> pd: project definition

## 3 heap management

how to use dynamic allocation: set `configSUPPORT_DYNAMIC_ALLOCATION`; api: `pvPortMalloc` and `vPortFree`

**example implementations(heap_x.c)**:
1. no reuse
2. reuse with fixed block size
3. wrapping malloc() and free()
4. combines adjacent blocks
5. support separated physical spaces

defining heap:

- configTOTAL_HEAP_SIZE
- configAPPLICATION_ALLOCATED_HEAP: up to user to define ucHeap(the heap instance)

heap monitoring:
- xPortGetFreeHeapSize()
- xPortGetMinimumEverFreeHeapSize()
- vPortGetHeapStats()
- vTaskGetInfo()
- configUSE_MALLOC_FAILED_HOOK: if set, requires implemntation `void vApplicationMallocFailedHook(void)`
- fast stack alloc:...

static alloc:...

## 4 task

**task function behavior**: of type `void funct_t`, never returns, otherwise use `vTaskDelete(NULL)` inside function to end itself

**task states**:
- not running
  - suspended: suspended purposefully
  - blocked: the state awaiting event
  - ready: that can be selected to run by the scheduler
- running

**related functions**: 
- `xTaskCreate(pv func, pc name, u stack depth, pv parms, ux pri, px task_handle|NULL)`, `vTaskStartScheduler()`
- `vTaskPrioritySet(x task_handle, pri)`, `uxTaskPriorityGet(p task_handle)->pri`
- `vTaskDelete(px task_handle|NULL)`
- `vTaskDelay(ticks)`, `vTaskDelayUntil(p prev_tick, ticks)`
- `vTaskSuspend`, `vTaskResume`

> `pdMS_TO_TICKS`

**scheduler behavior**: select highest priority having task(s) - > execute a task for a tick(time-slice)(depends on configTICK_RATE_HZ). **preemption** means a higher task get ready when a lower task is running and immediately runs, configured by `configUSE_PREEMPTION`; to select task with same priority: `configUSE_TIME_SLICING` set: to select earliest ended(in-turn), unset: to select latest ended(indefinite until) 


hooking idle:...

task local storage(TLS): ...

cooperative scheduling:...

## 5 queue

**motivation of queue**: facilitate sender/receiver design mode

