/**
 * @file wiring_private.h
 * @brief Helpers shared between core source files. Not for sketches.
 */
#pragma once

#include "Arduino.h"

// Configures a PWM slice to run at `hz`; returns the counter top (wrap) value.
// Shared by analogWrite() (pwm.cpp) and tone() (tone.cpp).
uint16_t pwmConfigureSlice(uint slice, uint32_t hz);

// Rescales `value` from `from_bits` to `to_bits` resolution.
inline uint32_t rescaleBits(uint32_t value, int from_bits, int to_bits) {
    if (from_bits == to_bits) return value;
    if (from_bits > to_bits)  return value >> (from_bits - to_bits);
    return value << (to_bits - from_bits);
}

// --- Analog (wiring_analog.cpp, analog_burst.cpp) ----------------------------------
// Which ADC reads an Arduino pin.
enum class AnalogSource : uint8_t {
    None,     // not an analog pin
    Rp2040,   // RP2040 ADC (GPIO26-29)
    Variant,  // the board's own ADC, through the variantAnalog*() hooks
};
AnalogSource analogSourceOf(pin_size_t pin);

// Current analogReadResolution() setting, in bits.
int analogReadBits();

// Waits while a burst is using `source`'s ADC (analog_burst.cpp).
void analogWaitForBurst(AnalogSource source);
