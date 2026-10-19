/**
 * @file RP2040.cpp
 * @brief `rp2040`: inter-core FIFO, stopping the other core, core 1 start-up.
 *
 * The heap functions are in heap.cpp.
 *
 * Without FreeRTOS, core 1 runs setup1()/loop1() directly, and both cores are
 * set up as multicore "lockout victims" so either can stop the other
 * (idleOtherCore(), which uses the hardware FIFO). With FreeRTOS, freertos.cpp
 * starts the cores and does the stopping.
 */
#include "wiring_private.h"
#include "hardware/clocks.h"
#include "hardware/sync.h"
#include "pico/multicore.h"

RP2040Class rp2040;

// --- FIFO ------------------------------------------------------------------------------
// pico_util queues are safe between cores and, with FreeRTOS, block the waiting task.
void RP2040FifoClass::begin() {
    if (started_) return;
    for (queue_t& q : queues_) {
        queue_init(&q, sizeof(uint32_t), kDepth);
    }
    started_ = true;
}

void RP2040FifoClass::push(uint32_t value) {
    queue_add_blocking(&queues_[get_core_num() ^ 1u], &value);
}

bool RP2040FifoClass::push_nb(uint32_t value) {
    return queue_try_add(&queues_[get_core_num() ^ 1u], &value);
}

uint32_t RP2040FifoClass::pop() {
    uint32_t value;
    queue_remove_blocking(&queues_[get_core_num()], &value);
    return value;
}

bool RP2040FifoClass::pop_nb(uint32_t* value) {
    uint32_t v;
    if (!queue_try_remove(&queues_[get_core_num()], &v)) return false;
    *value = v;
    return true;
}

int RP2040FifoClass::available() {
    return static_cast<int>(queue_get_level(&queues_[get_core_num()]));
}

// --- Core 1 (without FreeRTOS) ---------------------------------------------------------
static bool core1_started = false;

static void core1Main() {
    multicore_lockout_victim_init();  // core 0 can now stop this core
    if (setup1) setup1();
    for (;;) {
        if (loop1) {
            loop1();
        } else {
            __wfe();  // nothing more to run on core 1
        }
    }
}

void startCore1() {
    if (!sketchUsesCore1()) return;
    multicore_launch_core1(core1Main);
    multicore_lockout_victim_init();  // and core 1 can stop core 0
    core1_started = true;
}

void RP2040Class::restartCore1() {
#if !PHYDUINO_FREERTOS
    if (!core1_started || get_core_num() != 0) return;
    if (idling_other_) resumeOtherCore();
    multicore_reset_core1();
    multicore_launch_core1(core1Main);
#endif
}

// --- Stopping the other core -------------------------------------------------------------
void RP2040Class::idleOtherCore() {
    if (idling_other_) return;
#if PHYDUINO_FREERTOS
    if (!freeRtosIdleOtherCore()) return;
#else
    if (!multicore_lockout_victim_is_initialized(get_core_num() ^ 1u)) return;  // not running a sketch
    multicore_lockout_start_blocking();
#endif
    idling_other_ = true;
}

void RP2040Class::resumeOtherCore() {
    if (!idling_other_) return;
#if PHYDUINO_FREERTOS
    freeRtosResumeOtherCore();
#else
    multicore_lockout_end_blocking();
#endif
    idling_other_ = false;
}

// --- Chip information --------------------------------------------------------------------
int RP2040Class::cpuid() {
    return static_cast<int>(get_core_num());
}

uint32_t RP2040Class::f_cpu() {
    return clock_get_hz(clk_sys);
}
