/**
 * @file FreeRTOSConfig.h
 * @brief FreeRTOS kernel configuration for Tools > Operating System > FreeRTOS.
 *
 * Compiled into the FreeRTOS SDK bundle (platform/system/<board>_freertos/), so
 * a sketch cannot change it: changing this file requires regenerating that
 * bundle (docs/maintenance/update-pico-sdk.html). Sketches see the same values
 * through the bundle's include folder.
 *
 * Also the configuration under CMSIS-RTOS2 (sdk-builder/cmsis-rtos2/), whose
 * checks (freertos_os2.h) need 56 priorities and the INCLUDE_ options below.
 *
 * SMP on both RP2040 cores. The core (platform/cores/phyduino/freertos.cpp)
 * starts the scheduler with setup()/loop() as a task on core 0 and
 * setup1()/loop1() as a task on core 1, and provides the hook functions
 * enabled below. Design: docs/decisions/009-multicore-freertos.html
 *
 * Every SDK file includes this header (through freertos_sdk_config.h), so it
 * must stay valid C and C++ and include nothing.
 */
#ifndef PHYDUINO_FREERTOS_CONFIG_H
#define PHYDUINO_FREERTOS_CONFIG_H

// --- Scheduler -----------------------------------------------------------------
#define configNUMBER_OF_CORES                   2
#define configTICK_CORE                         0
#define configRUN_MULTIPLE_PRIORITIES           1
#define configUSE_CORE_AFFINITY                 1   // vTaskCoreAffinitySet(): pin a task to a core
#define configUSE_TASK_PREEMPTION_DISABLE       1   // used by rp2040.idleOtherCore()
#define configUSE_PASSIVE_IDLE_HOOK             0
#define configUSE_PREEMPTION                    1
#define configUSE_TIME_SLICING                  1   // equal-priority tasks share a core
#define configUSE_TICKLESS_IDLE                 0
#define configTICK_RATE_HZ                      1000    // 1 ms tick: pdMS_TO_TICKS(ms) == ms
#define configCPU_CLOCK_HZ                      (ulPhyduinoCpuClockHz())  // clk_sys, set by Tools > CPU Speed
// 0 (idle) .. 55. CMSIS-RTOS2 needs exactly 56: osPriority_t p is FreeRTOS
// priority p - 1 (osPriorityNormal 24 is 23).
#define configMAX_PRIORITIES                    56
#define configUSE_PORT_OPTIMISED_TASK_SELECTION 0   // CMSIS-RTOS2: more than 32 priorities
#define configMINIMAL_STACK_SIZE                256 // words (1 KB)
#define configMAX_TASK_NAME_LEN                 16
#define configTICK_TYPE_WIDTH_IN_BITS           TICK_TYPE_WIDTH_32_BITS
#define configIDLE_SHOULD_YIELD                 1
#define configSTACK_DEPTH_TYPE                  uint32_t
#define configMESSAGE_BUFFER_LENGTH_TYPE        size_t
#define configENABLE_BACKWARD_COMPATIBILITY     1
#define configNUM_THREAD_LOCAL_STORAGE_POINTERS 5
#define configUSE_NEWLIB_REENTRANT              0

// --- Synchronisation -------------------------------------------------------------
#define configUSE_TASK_NOTIFICATIONS            1
#define configTASK_NOTIFICATION_ARRAY_ENTRIES   3
#define configUSE_MUTEXES                       1
#define configUSE_RECURSIVE_MUTEXES             1
#define configUSE_COUNTING_SEMAPHORES           1
#define configUSE_QUEUE_SETS                    1
#define configQUEUE_REGISTRY_SIZE               8

// --- Memory ----------------------------------------------------------------------
// heap_3: pvPortMalloc() uses malloc(), so tasks, String and new share one heap
// (no fixed configTOTAL_HEAP_SIZE to choose). malloc() is made thread safe by
// the SDK's PICO_USE_MALLOC_MUTEX.
#define configSUPPORT_DYNAMIC_ALLOCATION        1
#define configSUPPORT_STATIC_ALLOCATION         1
#define configKERNEL_PROVIDED_STATIC_MEMORY     1   // idle/timer task memory comes from the kernel

// --- Hooks (defined in the core; a sketch may replace them) ----------------------
#define configUSE_IDLE_HOOK                     0
#define configUSE_TICK_HOOK                     0
#define configCHECK_FOR_STACK_OVERFLOW          2   // calls vApplicationStackOverflowHook()
#define configUSE_MALLOC_FAILED_HOOK            1   // calls vApplicationMallocFailedHook()
#define configUSE_DAEMON_TASK_STARTUP_HOOK      0

// --- Software timers ---------------------------------------------------------------
// Also needed by the SDK interop: an SDK mutex or semaphore released in an
// interrupt (e.g. USB Serial's 1 ms poll, SDK alarms) wakes waiting tasks
// through the timer task's queue (xEventGroupSetBitsFromISR). A full queue
// would lose the wake-up, so it is longer than FreeRTOS's usual 10.
#define configUSE_TIMERS                        1
#define configTIMER_TASK_PRIORITY               (configMAX_PRIORITIES - 1)
#define configTIMER_QUEUE_LENGTH                32
#define configTIMER_TASK_STACK_DEPTH            1024

// --- Diagnostics -------------------------------------------------------------------
#define configUSE_TRACE_FACILITY                1   // uxTaskGetSystemState()
#define configUSE_STATS_FORMATTING_FUNCTIONS    1   // vTaskListTasks()
#define configGENERATE_RUN_TIME_STATS           0
#define configUSE_CO_ROUTINES                   0

// A failed kernel check stops the board (the core's vPhyduinoFreeRtosAssert()).
// ulPhyduinoCpuClockHz() is clock_get_hz(clk_sys), also in the core.
#ifdef __cplusplus
extern "C" {
#endif
void vPhyduinoFreeRtosAssert(const char* file, int line);
unsigned long ulPhyduinoCpuClockHz(void);
#ifdef __cplusplus
}
#endif
#define configASSERT(x) \
    do { if (!(x)) vPhyduinoFreeRtosAssert(__FILE__, __LINE__); } while (0)

// --- RP2040 port -------------------------------------------------------------------
// SDK mutexes, semaphores, queues and sleep_ms() block the calling task instead
// of spinning (rp2040_config.h).
#define configSUPPORT_PICO_SYNC_INTEROP         1
#define configSUPPORT_PICO_TIME_INTEROP         1

// --- Optional API functions --------------------------------------------------------
#define INCLUDE_vTaskPrioritySet                1
#define INCLUDE_uxTaskPriorityGet               1
#define INCLUDE_vTaskDelete                     1
#define INCLUDE_vTaskSuspend                    1
#define INCLUDE_xResumeFromISR                  1
#define INCLUDE_vTaskDelayUntil                 1
#define INCLUDE_vTaskDelay                      1
#define INCLUDE_xTaskGetSchedulerState          1
#define INCLUDE_xTaskGetCurrentTaskHandle       1
#define INCLUDE_uxTaskGetStackHighWaterMark     1
#define INCLUDE_xTaskGetIdleTaskHandle          1
#define INCLUDE_eTaskGetState                   1
#define INCLUDE_xTimerPendFunctionCall          1
#define INCLUDE_xTaskAbortDelay                 1
#define INCLUDE_xTaskGetHandle                  1
#define INCLUDE_xTaskResumeFromISR              1
#define INCLUDE_xQueueGetMutexHolder            1

#endif  // PHYDUINO_FREERTOS_CONFIG_H
