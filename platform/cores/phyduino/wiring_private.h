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
