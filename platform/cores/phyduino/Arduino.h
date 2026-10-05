/**
 * @file Arduino.h
 * @brief Entry header for sketches: Arduino API + board pin map + Phyduino extras.
 *
 * The pico-sdk is available too; sketches can include e.g. "hardware/pwm.h"
 * and use digitalPinToGpio() to get the RP2040 GPIO behind an Arduino pin.
 */
#pragma once

#include "api/ArduinoAPI.h"
#include "api/itoa.h"                          // itoa(), utoa(), ltoa(), ultoa()
#include "api/deprecated-avr-comp/avr/dtostrf.h"  // dtostrf(), common in AVR-era sketches
#include "pico.h"

#include "pins_arduino.h"

// --- Interrupt enable/disable (ArduinoCore-API leaves these to the core) -----
#define interrupts()    __asm volatile("cpsie i" ::: "memory")
#define noInterrupts()  __asm volatile("cpsid i" ::: "memory")

#ifdef __cplusplus

using namespace arduino;

// --- Pin mapping ---------------------------------------------------------------
inline constexpr uint8_t kPinToGpio[] = PHYDUINO_PIN_TO_GPIO;
static_assert(sizeof(kPinToGpio) == PINS_COUNT,
              "pins_arduino.h: PHYDUINO_PIN_TO_GPIO needs exactly PINS_COUNT entries");

/// RP2040 GPIO behind an Arduino pin, or NOT_A_GPIO (e.g. A3-A5, out of range).
inline uint8_t digitalPinToGpio(pin_size_t pin) {
    return pin < PINS_COUNT ? kPinToGpio[pin] : NOT_A_GPIO;
}

#define digitalPinToInterrupt(pin)  (pin)

// --- Phyduino extensions to the Arduino API ------------------------------------
void analogReadResolution(int bits);    // default 10 (0-1023), max 12
void analogWriteResolution(int bits);   // default 8 (0-255), max 16
void analogWriteFrequency(uint32_t hz); // default 1000 Hz

// --- Variant hooks: every variants/<board>/variant.cpp must define these ------
// (initVariant() is declared by ArduinoCore-API.)
/// Reads an analog pin that has no RP2040 GPIO (e.g. an external ADC).
/// Returns a 12-bit value (0-4095).
uint16_t variantAnalogRead(pin_size_t pin);

// --- Serial ports ----------------------------------------------------------------
#include "SerialUSB.h"
#include "SerialUART.h"

#endif  // __cplusplus
