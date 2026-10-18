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
/// @ingroup core_interrupts
/// Enables interrupts again after noInterrupts().
#define interrupts()    __asm volatile("cpsie i" ::: "memory")
/// @ingroup core_interrupts
/// Disables interrupts, for example while reading a variable an interrupt changes.
/// Keep the section short: USB Serial, attachInterrupt() functions and tone()
/// durations wait until interrupts() (millis() keeps counting).
#define noInterrupts()  __asm volatile("cpsid i" ::: "memory")

#ifdef __cplusplus

using namespace arduino;

// --- Pin mapping ---------------------------------------------------------------
/// RP2040 GPIO for each Arduino pin, indexed by pin number (PHYDUINO_PIN_TO_GPIO).
#if __cplusplus >= 201703L
inline constexpr uint8_t kPinToGpio[] = PHYDUINO_PIN_TO_GPIO;
#else
static constexpr uint8_t kPinToGpio[] = PHYDUINO_PIN_TO_GPIO;  // C++14: no inline variables
#endif
static_assert(sizeof(kPinToGpio) == PINS_COUNT,
              "pins_arduino.h: PHYDUINO_PIN_TO_GPIO needs exactly PINS_COUNT entries");

/**
 * @ingroup core_digital
 * @brief RP2040 GPIO behind an Arduino pin, for use with the pico-sdk.
 * @param pin Arduino pin number (e.g. 2, A0, LED_BUILTIN).
 * @return GPIO number, or NOT_A_GPIO for a pin with no GPIO behind it (A3-A5)
 *         or a pin number out of range.
 */
inline uint8_t digitalPinToGpio(pin_size_t pin) {
    return pin < PINS_COUNT ? kPinToGpio[pin] : NOT_A_GPIO;
}

/// @ingroup core_interrupts
/// Interrupt number for attachInterrupt(): every pin can interrupt, so it is the pin itself.
#define digitalPinToInterrupt(pin)  (pin)

// --- Phyduino extensions to the Arduino API ------------------------------------
/**
 * @ingroup core_analog
 * @brief Sets the number of bits analogRead() returns, for every analog pin.
 * @param bits 1-12; the default is 10 (0-1023). Other values are ignored.
 */
void analogReadResolution(int bits);

/**
 * @ingroup core_analog
 * @brief Sets the range analogWrite() takes, for every pin.
 * @param bits 1-16; the default is 8 (0-255). Other values are ignored.
 */
void analogWriteResolution(int bits);

/**
 * @ingroup core_analog
 * @brief Sets the PWM frequency used by later analogWrite() calls.
 *
 * Pins on the same PWM slice share one frequency, set by the last
 * analogWrite() or tone() on any of them (table under analogWrite()).
 * @param hz Frequency in Hz; the default is 1000. 0 is ignored. The lowest
 *           is about 8 Hz at the default 133 MHz CPU speed (lower values
 *           run at that).
 */
void analogWriteFrequency(uint32_t hz);

// --- Variant hooks: every variants/<board>/variant.cpp must define these ------
// (initVariant() is declared by ArduinoCore-API.)
/**
 * @brief Reads an analog pin that has no RP2040 GPIO (e.g. an external ADC).
 *
 * Called by analogRead(); sketches use analogRead() instead.
 * @param pin Arduino pin number.
 * @return 12-bit value (0-4095).
 */
uint16_t variantAnalogRead(pin_size_t pin);

// --- Serial ports ----------------------------------------------------------------
#include "SerialUSB.h"
#include "SerialUART.h"

#endif  // __cplusplus
