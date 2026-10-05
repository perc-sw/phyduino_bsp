/**
 * @file wiring_time.cpp
 * @brief millis(), micros(), delay(), delayMicroseconds().
 *
 * All based on the RP2040's 64-bit microsecond timer, which starts at boot.
 */
#include "Arduino.h"
#include "pico/time.h"

unsigned long millis() {
    return static_cast<unsigned long>(time_us_64() / 1000u);
}

unsigned long micros() {
    return time_us_32();
}

void delay(unsigned long ms) {
    const uint64_t end = time_us_64() + static_cast<uint64_t>(ms) * 1000u;
    do {
        yield();
    } while (time_us_64() < end);
}

void delayMicroseconds(unsigned int us) {
    busy_wait_us_32(us);
}
