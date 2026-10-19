/**
 * @file wiring_time.cpp
 * @brief millis(), micros(), delay(), delayMicroseconds().
 *
 * All based on the RP2040's 64-bit microsecond timer, which starts at boot.
 * With FreeRTOS, delay() blocks the calling task, so other tasks run meanwhile.
 */
#include "Arduino.h"
#include "pico/time.h"

#if PHYDUINO_FREERTOS
#include "FreeRTOS.h"
#include "task.h"
#endif

unsigned long millis() {
    return static_cast<unsigned long>(time_us_64() / 1000u);
}

unsigned long micros() {
    return time_us_32();
}

void delay(unsigned long ms) {
    const uint64_t end = time_us_64() + static_cast<uint64_t>(ms) * 1000u;
#if PHYDUINO_FREERTOS
    // Block the task for all but the last 1 ms tick, so other tasks get the
    // core; then wait out the rest exactly, below.
    if (ms > 1 && __get_current_exception() == 0 &&
        xTaskGetSchedulerState() == taskSCHEDULER_RUNNING) {
        vTaskDelay(pdMS_TO_TICKS(ms - 1));
    }
#endif
    do {
        yield();
    } while (time_us_64() < end);
}

void delayMicroseconds(unsigned int us) {
    busy_wait_us_32(us);
}
