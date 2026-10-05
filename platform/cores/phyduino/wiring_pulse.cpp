/**
 * @file wiring_pulse.cpp
 * @brief pulseIn(), pulseInLong(): measure a pulse length in microseconds.
 *
 * Both use the microsecond timer, so interrupts don't affect accuracy;
 * pulseInLong() is the same function, kept for Arduino compatibility.
 */
#include "Arduino.h"
#include "hardware/gpio.h"
#include "pico/time.h"

// Waits while the pin reads `level`; false if `deadline` passes first.
static bool waitWhile(uint gpio, bool level, uint64_t deadline) {
    while (gpio_get(gpio) == level) {
        if (time_us_64() >= deadline) return false;
    }
    return true;
}

unsigned long pulseInLong(pin_size_t pin, uint8_t state, unsigned long timeout) {
    const uint8_t gpio = digitalPinToGpio(pin);
    if (gpio == NOT_A_GPIO) return 0;

    const bool level = state != LOW;
    const uint64_t deadline = time_us_64() + timeout;

    if (!waitWhile(gpio, level, deadline))  return 0;  // finish any pulse in progress
    if (!waitWhile(gpio, !level, deadline)) return 0;  // wait for the pulse to start
    const uint64_t start = time_us_64();
    if (!waitWhile(gpio, level, deadline))  return 0;  // wait for it to end
    return static_cast<unsigned long>(time_us_64() - start);
}

unsigned long pulseIn(pin_size_t pin, uint8_t state, unsigned long timeout) {
    return pulseInLong(pin, state, timeout);
}
