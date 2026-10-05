/**
 * @file max11635.h
 * @brief Minimal driver for the on-board MAX11635 12-bit ADC (U5).
 *
 * Used by variant.cpp so analogRead(A3..A5) works transparently. Configuration:
 * external 2.048 V reference, unipolar single-ended inputs, conversions timed
 * internally and started by a CNVST pulse; EOC goes low when the result is ready.
 * Pins are in pins_arduino.h (PHYDUINO_EXT_ADC_*).
 */
#pragma once

#include <stdint.h>

namespace max11635 {

/// Configures SPI0 + control pins and resets the ADC. Safe to call again.
void begin();

/// One conversion on `channel` (0-3). Returns 0-4095, or 0 if the ADC
/// does not respond.
uint16_t read(uint8_t channel);

}  // namespace max11635
