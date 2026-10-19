/**
 * @file RP2040.h
 * @brief `rp2040`: the second core (setup1()/loop1()), the inter-core FIFO and chip information.
 *
 * Same names as the arduino-pico core (Earle Philhower), so multicore sketches
 * written for it also build here. Included by Arduino.h.
 *
 * A sketch uses the second core by defining setup1() and loop1(); they run on
 * core 1 at the same time as setup() and loop() on core 0. With Tools >
 * Operating System > FreeRTOS they run as FreeRTOS tasks instead (setup()/loop()
 * pinned to core 0, setup1()/loop1() to core 1). Design:
 * docs/decisions/009-multicore-freertos.html
 */
#pragma once

#include <stddef.h>
#include <stdint.h>

#include "pico/util/queue.h"

/**
 * @ingroup core_multicore
 * @brief Passes 32-bit values between the two cores: `rp2040.fifo`.
 *
 * There is one queue for each direction: a value pushed on core 0 is popped
 * on core 1, and the other way round. Each queue holds kDepth values. A value
 * can be a number or a pointer cast to `uint32_t`.
 *
 * This is a queue in RAM, not the RP2040's hardware FIFO, which the core uses
 * to stop the other core (idleOtherCore()) and FreeRTOS uses for scheduling.
 * Do not use the SDK's multicore_fifo_*() functions in a sketch.
 *
 * With FreeRTOS, "this core" is the core the calling task runs on: use the
 * FIFO from loop() and loop1() (each pinned to its core) or from tasks pinned
 * with vTaskCoreAffinitySet(). A task waiting in push() or pop() is blocked,
 * so other tasks run meanwhile.
 */
class RP2040FifoClass {
public:
    /// Values each direction can hold before push() waits.
    static constexpr unsigned kDepth = 16;

    /**
     * @brief Sends a value to the other core, waiting while its queue is full.
     * @param value Value to send.
     */
    void push(uint32_t value);

    /**
     * @brief Sends a value to the other core if there is room, without waiting.
     * @param value Value to send.
     * @return true if sent; false if the other core's queue is full.
     */
    bool push_nb(uint32_t value);

    /**
     * @brief Takes the next value sent by the other core, waiting until there is one.
     * @return The value.
     */
    uint32_t pop();

    /**
     * @brief Takes the next value sent by the other core if there is one, without waiting.
     * @param value Set to the value; unchanged if there is none.
     * @return true if a value was taken.
     */
    bool pop_nb(uint32_t* value);

    /**
     * @brief Number of values the other core has sent that this core has not popped yet.
     * @return 0 to kDepth.
     */
    int available();

    /// @private Sets up the queues; called by the core at start-up, not by sketches.
    void begin();

private:
    queue_t queues_[2];  // queues_[c] holds the values sent to core c
    bool started_ = false;
};

/**
 * @ingroup core_multicore
 * @brief The second core and the chip: `rp2040`.
 */
class RP2040Class {
public:
    /// Inter-core FIFO: see RP2040FifoClass.
    RP2040FifoClass fifo;

    /**
     * @brief Stops the other core until resumeOtherCore(), and returns once it has stopped.
     *
     * The other core waits with its interrupts off, so it cannot touch
     * anything in the meantime: use this around code that must not be
     * disturbed (for example writing to flash, which the other core may be
     * running code from). Keep the time short. Does nothing if the other core
     * is not running a sketch (no setup1()/loop1() without FreeRTOS) or is
     * already stopped.
     *
     * With FreeRTOS, the calling task also stays on its core and is not
     * pre-empted until resumeOtherCore().
     */
    void idleOtherCore();

    /// Lets the core stopped by idleOtherCore() carry on. Does nothing if none is stopped.
    void resumeOtherCore();

    /**
     * @brief Resets core 1 and starts it again from setup1().
     *
     * Only without FreeRTOS, and only if the sketch defines setup1() or
     * loop1(); does nothing otherwise. Call it from core 0.
     */
    void restartCore1();

    /**
     * @brief Which core the calling code runs on.
     * @return 0 or 1.
     */
    int cpuid();

    /**
     * @brief CPU clock (Tools > CPU Speed).
     * @return Frequency in Hz, e.g. 133000000.
     */
    uint32_t f_cpu();

    /**
     * @brief Size of the heap (memory for `new`, `malloc()`, String and, with FreeRTOS, tasks and queues).
     * @return Bytes.
     */
    int getTotalHeap();

    /**
     * @brief Heap memory in use.
     * @return Bytes.
     */
    int getUsedHeap();

    /**
     * @brief Heap memory still free: getTotalHeap() - getUsedHeap().
     * @return Bytes. Not necessarily one block: a single allocation of this
     *         size can still fail.
     */
    int getFreeHeap();

private:
    bool idling_other_ = false;
};

/// @ingroup core_multicore
/// The second core, inter-core FIFO and chip information (arduino-pico compatible).
extern RP2040Class rp2040;

extern "C" {
/**
 * @ingroup core_multicore
 * @brief Written by the sketch (optional): runs once on core 1, at the same time as setup() on core 0.
 *
 * Defining setup1() or loop1() starts core 1. Core 1 has its own 4 KB stack
 * (8 KB task stack with FreeRTOS).
 */
void setup1(void) __attribute__((weak));

/**
 * @ingroup core_multicore
 * @brief Written by the sketch (optional): runs over and over on core 1 after setup1().
 */
void loop1(void) __attribute__((weak));
}
