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
/// durations wait until interrupts() (millis() keeps counting). Only the
/// calling core is affected: the other core keeps running (@ref core_multicore).
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

// --- Phyduino analog acquisition: every analog pin (wiring_analog.cpp, analog_burst.cpp)
/**
 * @ingroup core_analog
 * @brief Sets how many samples are averaged for each analog result, on every analog pin.
 *
 * Applies to analogRead(), analogReadMillivolts(), analogReadMultiple() and
 * analogReadDifferentialMillivolts(); not to bursts. The external ADC
 * (A3-A5 on the Phyduino Pico) averages in hardware; the RP2040 ADC (A0-A2)
 * takes the samples one after another (about 2 µs each) and averages them.
 * @param samples 1 (the default, no averaging), 4, 8, 16 or 32. Other values
 *                use the next lower one (e.g. 10 gives 8).
 */
void analogReadAveraging(uint8_t samples);

/**
 * @ingroup core_analog
 * @brief Voltage that reads as full scale on an analog pin.
 * @param pin Analog pin (A0-A5).
 * @return Full scale in mV: 3300 for A0-A2 and 2048 for A3-A5 on the
 *         Phyduino Pico. 0 for a pin that is not analog.
 */
uint32_t analogFullScaleMillivolts(pin_size_t pin);

/**
 * @ingroup core_analog
 * @brief Reads an analog pin as a voltage, whatever its ADC and full scale.
 *
 * Uses the full 12 bits, whatever analogReadResolution() is set to, and the
 * analogReadAveraging() setting.
 * @param pin Analog pin (A0-A5).
 * @return Voltage in mV; 0 for a pin that is not analog.
 */
uint32_t analogReadMillivolts(pin_size_t pin);

/**
 * @ingroup core_analog
 * @brief Voltage between two analog pins, `plus` minus `minus`.
 *
 * On the Phyduino Pico, A3 with A4 (either way round) is a true differential
 * measurement: the external ADC samples both pins at the same instant, so
 * noise common to both cancels. It covers ±1024 mV in 0.5 mV steps; a larger
 * difference is measured as two separate readings instead.
 *
 * Any other pair is computed from two readings taken one after the other
 * (a few µs apart), so it is not simultaneous: fine for slow signals, not
 * for removing fast common noise.
 * @param plus  Analog pin (A0-A5).
 * @param minus Analog pin (A0-A5).
 * @return `plus` - `minus` in mV, negative if `minus` is higher. 0 if either
 *         pin is not analog.
 */
int32_t analogReadDifferentialMillivolts(pin_size_t plus, pin_size_t minus);

/**
 * @ingroup core_analog
 * @brief Reads several analog pins as close together in time as possible.
 *
 * Pins on the external ADC are converted in one scan, a few µs apart; pins on
 * the RP2040 ADC follow each other about 2 µs apart. The two ADCs are read
 * one after the other, not at the same time.
 * @param pins    Analog pins, in any order; a pin may appear more than once.
 * @param count   Number of pins.
 * @param results Array of `count` values, in the same order as `pins`, in
 *                analogRead() units (see analogReadResolution()).
 * @return false if a pin is not analog (its result is 0).
 */
bool analogReadMultiple(const pin_size_t* pins, uint8_t count, uint16_t* results);

/**
 * @ingroup core_analog
 * @brief Samples one analog pin at a fixed rate into an array, and waits until it is full.
 *
 * The samples are taken by the hardware at evenly spaced times; the sketch
 * only waits. Averaging is not applied. Rates:
 *
 * | Pins (Phyduino Pico) | Highest rate | How |
 * |---|---|---|
 * | A0-A2 (RP2040 ADC) | 500000 Hz | ADC timer and DMA; rates below 733 Hz use a timer interrupt |
 * | A3-A5 (MAX11635) | 300000 Hz | Timer interrupt up to 20000 Hz; above that, the ADC clocked by SPI with DMA |
 *
 * The rate actually used can differ slightly from the one asked for (the
 * hardware divides a fixed clock); it is returned.
 * @param pin            Analog pin (A0-A5).
 * @param buffer         Array for the samples, in analogRead() units.
 * @param count          Number of samples.
 * @param sample_rate_hz Samples per second, 1 up to the highest rate above.
 * @return Rate used in Hz, or 0 if the arguments are invalid or a burst is
 *         already running.
 */
uint32_t analogReadBurst(pin_size_t pin, uint16_t* buffer, size_t count, uint32_t sample_rate_hz);

/**
 * @ingroup core_analog
 * @brief Starts a burst like analogReadBurst(), but returns at once; the samples are taken in the background.
 *
 * Check analogBurstDone() before using `buffer`. Only one burst runs at a
 * time. While it runs, other reads of an analog pin on the same ADC wait
 * until it has finished.
 * @param pin            Analog pin (A0-A5).
 * @param buffer         Array for the samples; must stay valid until the burst is done.
 * @param count          Number of samples.
 * @param sample_rate_hz Samples per second (see analogReadBurst()).
 * @return Rate used in Hz, or 0 if the arguments are invalid or a burst is
 *         already running.
 */
uint32_t analogStartBurst(pin_size_t pin, uint16_t* buffer, size_t count, uint32_t sample_rate_hz);

/**
 * @ingroup core_analog
 * @brief Whether the burst started by analogStartBurst() has finished.
 * @return true once every sample is in the buffer (and when no burst was started).
 */
bool analogBurstDone();

/**
 * @ingroup core_analog
 * @brief Stops the burst started by analogStartBurst() before it has finished.
 *
 * Sampling stops at once and the ADC is free again for analogRead() and the
 * other analog functions. The buffer's contents are then not valid: some
 * samples may be missing and the rest are not converted to
 * analogReadResolution(). Afterwards analogBurstDone() returns true and a new
 * burst can be started. Does nothing if no burst is running.
 */
void analogStopBurst();

// --- Variant hooks: every variants/<board>/variant.cpp must define these ------
// (initVariant() is declared by ArduinoCore-API.) They handle analog pins with
// no RP2040 GPIO (e.g. an external ADC); sketches use the analog functions above.
/// @name Variant hooks (not for sketches)
/// @{

/**
 * @brief Reads an analog pin that has no RP2040 GPIO, with the variant's averaging setting.
 * @param pin Arduino pin number.
 * @return 12-bit value (0-4095), or 0 if the pin is not one of the variant's.
 */
uint16_t variantAnalogRead(pin_size_t pin);

/**
 * @brief Full scale of a variant analog pin.
 * @param pin Arduino pin number.
 * @return Full scale in mV, or 0 if the pin is not one of the variant's.
 */
uint32_t variantAnalogFullScaleMillivolts(pin_size_t pin);

/**
 * @brief Sets the averaging for the variant's analog pins.
 * @param samples 1, 4, 8, 16 or 32 (already rounded by the core).
 */
void variantAnalogReadAveraging(uint8_t samples);

/**
 * @brief Reads several variant analog pins in one go.
 * @param pins  Variant analog pins.
 * @param count Number of pins.
 * @param raw   Array of `count` 12-bit results, in the order of `pins`.
 * @return false if a pin is not one of the variant's or the ADC does not answer.
 */
bool variantAnalogReadMultiple(const pin_size_t* pins, uint8_t count, uint16_t* raw);

/**
 * @brief Measures `plus` - `minus` if the variant can do it in hardware.
 * @param plus       Arduino pin number.
 * @param minus      Arduino pin number.
 * @param millivolts Result in mV.
 * @return false if the pair has no hardware differential mode or the result
 *         is out of its range; the core then uses two single readings.
 */
bool variantAnalogReadDifferential(pin_size_t plus, pin_size_t minus, int32_t* millivolts);

/**
 * @brief Starts a background burst on a variant analog pin; 12-bit samples into `buffer`.
 * @param pin            Arduino pin number.
 * @param buffer         Array for `count` samples.
 * @param count          Number of samples.
 * @param sample_rate_hz Samples per second.
 * @return Rate used in Hz, or 0 if not possible.
 */
uint32_t variantAnalogStartBurst(pin_size_t pin, uint16_t* buffer, size_t count, uint32_t sample_rate_hz);

/**
 * @brief Whether the variant's burst has finished (true if none was started).
 * @return true once every sample is in the buffer.
 */
bool variantAnalogBurstDone();

/// Stops the variant's burst.
void variantAnalogStopBurst();

/// @}

// --- Serial ports ----------------------------------------------------------------
#include "SerialUSB.h"
#include "SerialUART.h"

// --- Multicore: setup1()/loop1(), rp2040.fifo -----------------------------------
#include "RP2040.h"

#endif  // __cplusplus
