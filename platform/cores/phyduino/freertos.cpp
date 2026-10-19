/**
 * @file freertos.cpp
 * @brief Tools > Operating System > FreeRTOS: the sketch's tasks, hook functions, idleOtherCore().
 *
 * Compiles to nothing without FreeRTOS. The kernel itself is in the FreeRTOS
 * SDK bundle (system/<board>_freertos/), configured by
 * sdk-builder/freertos/FreeRTOSConfig.h. Design:
 * docs/decisions/009-multicore-freertos.html; with CMSIS-RTOS2,
 * docs/decisions/010-cmsis-rtos2.html
 */
#include "wiring_private.h"

#if PHYDUINO_FREERTOS

#include "FreeRTOS.h"
#include "task.h"
#include "hardware/clocks.h"
#include "hardware/sync.h"

// setup()/loop() and setup1()/loop1(): 8 KB stacks (core 0's stack without
// FreeRTOS is 4 KB), one task per core. Priority: the lowest above idle; with
// CMSIS-RTOS2, osPriorityNormal (24, FreeRTOS 23), the default for
// osThreadNew(), as CMSIS-RTOS2 code expects of the main thread.
static constexpr configSTACK_DEPTH_TYPE kSketchStackWords = 2048;
#if PHYDUINO_CMSIS_RTOS2
static constexpr UBaseType_t kSketchPriority = 24 - 1;
#else
static constexpr UBaseType_t kSketchPriority = tskIDLE_PRIORITY + 1;
#endif

static void loopTask(void*) {
    setup();
    for (;;) {
        loop();
    }
}

static void loop1Task(void*) {
    if (setup1) setup1();
    if (!loop1) vTaskDelete(nullptr);  // nothing more to run on core 1
    for (;;) {
        loop1();
    }
}

void startFreeRtos() {
    xTaskCreateAffinitySet(loopTask, "loop", kSketchStackWords, nullptr, kSketchPriority,
                           1u << 0, nullptr);
    if (sketchUsesCore1()) {
        xTaskCreateAffinitySet(loop1Task, "loop1", kSketchStackWords, nullptr, kSketchPriority,
                               1u << 1, nullptr);
    }
    vTaskStartScheduler();  // starts core 1 too; returns only if out of memory
    panic("FreeRTOS: not enough memory to start");
}

// --- Stopping the other core -----------------------------------------------------------
// A task at the highest priority, pinned to the core to stop, waits with its
// interrupts off until released. Created the first time it is needed.
static TaskHandle_t idler[2];
static volatile bool idle_requested[2];
static volatile bool idle_parked[2];

static void idlerTask(void* arg) {
    const uint core = static_cast<uint>(reinterpret_cast<uintptr_t>(arg));
    for (;;) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        const uint32_t saved = save_and_disable_interrupts();
        idle_parked[core] = true;
        while (idle_requested[core]) {
            tight_loop_contents();
        }
        idle_parked[core] = false;
        restore_interrupts(saved);
    }
}

bool freeRtosIdleOtherCore() {
    vTaskPreemptionDisable(nullptr);  // stay on this core until freeRtosResumeOtherCore()
    const uint other = get_core_num() ^ 1u;
    if (idler[other] == nullptr &&
        xTaskCreateAffinitySet(idlerTask, "idleOther", configMINIMAL_STACK_SIZE,
                               reinterpret_cast<void*>(static_cast<uintptr_t>(other)),
                               configMAX_PRIORITIES - 1, 1u << other, &idler[other]) != pdPASS) {
        idler[other] = nullptr;
        vTaskPreemptionEnable(nullptr);
        return false;
    }
    idle_requested[other] = true;
    xTaskNotifyGive(idler[other]);
    while (!idle_parked[other]) {
        tight_loop_contents();
    }
    return true;
}

void freeRtosResumeOtherCore() {
    const uint other = get_core_num() ^ 1u;
    idle_requested[other] = false;
    while (idle_parked[other]) {
        tight_loop_contents();
    }
    vTaskPreemptionEnable(nullptr);
}

// --- Hook functions (FreeRTOSConfig.h) ---------------------------------------------------
// Weak, so a sketch can define its own. panic() stops the board; USB Serial
// stops with it, so nothing is printed. The CMSIS-RTOS2 wrapper has weak
// versions too; these are linked first, so they are the ones used.
extern "C" {

void vApplicationStackOverflowHook(TaskHandle_t, char* task_name) __attribute__((weak));
void vApplicationStackOverflowHook(TaskHandle_t, char* task_name) {
    panic("FreeRTOS: stack overflow in task %s", task_name);
}

void vApplicationMallocFailedHook(void) __attribute__((weak));
void vApplicationMallocFailedHook(void) {
    panic("FreeRTOS: out of memory");
}

void vPhyduinoFreeRtosAssert(const char* file, int line) {
    panic("FreeRTOS: check failed in %s line %d", file, line);
}

unsigned long ulPhyduinoCpuClockHz(void) {
    return clock_get_hz(clk_sys);
}

}  // extern "C"

#endif  // PHYDUINO_FREERTOS
