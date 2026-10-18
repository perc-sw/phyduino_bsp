/**
 * @file max11635.h
 * @brief Driver for the on-board MAX11635 12-bit ADC (U5).
 *
 * Used by variant.cpp for A3-A5, so sketches normally use the core's analog
 * functions (analogRead(), analogReadAveraging(), analogReadBurst(), ...),
 * which work the same on every analog pin. This header is the chip-level
 * interface underneath, by channel: 0-2 are A3-A5, 3 measures VBUS.
 *
 * Configuration: external 2.048 V reference (LM4040, U7), unipolar
 * single-ended inputs, conversions timed internally and started by a CNVST
 * pulse (clock mode 00); EOC goes low when the results are in the chip's
 * 16-entry FIFO. Bursts above kMaxTimedBurstHz switch to clock mode 11 (the
 * SPI clock times the conversions) for their duration.
 * Pins are in pins_arduino.h (PHYDUINO_EXT_ADC_*).
 */
#pragma once

#include <stddef.h>
#include <stdint.h>

namespace max11635 {

/// Number of input channels (AIN0-AIN3).
constexpr uint8_t kChannels = 4;
/// Fastest burst taken with a timer interrupt (clock mode 00), in samples per second.
constexpr uint32_t kMaxTimedBurstHz = 20000;
/// Fastest burst (clock mode 11, SCLK = 16 x rate, at most 4.8 MHz), in samples per second.
constexpr uint32_t kMaxBurstHz = 300000;

/// Configures SPI0 and the control pins, resets the ADC and applies the averaging setting. Safe to call again.
void begin();

/**
 * @brief Sets how many conversions the chip averages for each result.
 *
 * Applies to read(), scan() and readDifferential(); not to bursts.
 * @param samples 1 (off), 4, 8, 16 or 32; other values use the next lower one.
 */
void setAveraging(uint8_t samples);

/**
 * @brief Current averaging setting.
 * @return 1, 4, 8, 16 or 32.
 */
uint8_t averaging();

/**
 * @brief Runs one conversion (averaged if setAveraging() is on).
 * @param channel Input 0-3 (0-2 are A3-A5, 3 is VBUS).
 * @return 0-4095 (0 V to 2.048 V), or 0 if the ADC does not respond.
 */
uint16_t read(uint8_t channel);

/**
 * @brief Converts channels 0 to `last_channel` in one scan, a few µs apart.
 * @param last_channel Highest channel, 0-3.
 * @param results      Array of `last_channel` + 1 values, 0-4095.
 * @return false if the arguments are invalid or the ADC does not respond.
 */
bool scan(uint8_t last_channel, uint16_t* results);

/**
 * @brief True differential conversion of a channel pair, both inputs sampled at once.
 *
 * Pair 0 is AIN0 - AIN1 (A3 - A4); pair 1 is AIN2 - AIN3 (A5 - VBUS monitor).
 * The pair is set back to single-ended afterwards.
 * @param pair    0 or 1.
 * @param bipolar false: unipolar, 0 to 2.048 V (a negative difference reads 0);
 *                true: bipolar, -1.024 V to +1.024 V.
 * @param code    Result: 0-4095 unipolar, -2048 to 2047 bipolar (0.5 mV steps).
 * @return false if the arguments are invalid or the ADC does not respond.
 */
bool readDifferential(uint8_t pair, bool bipolar, int16_t* code);

/**
 * @brief Starts sampling one channel at a fixed rate into `buffer`, in the background.
 *
 * Up to kMaxTimedBurstHz a repeating timer interrupt starts each conversion;
 * above, up to kMaxBurstHz, the SPI clock times the conversions and DMA
 * moves the data. Averaging is off during a burst. Other calls into this
 * driver wait until the burst has finished.
 * @param channel        Input 0-3.
 * @param buffer         Array for `count` 12-bit samples; must stay valid until burstDone().
 * @param count          Number of samples.
 * @param sample_rate_hz Samples per second, 1 to kMaxBurstHz.
 * @return Rate used in Hz (it can differ slightly from the one asked for), or
 *         0 if the arguments are invalid or a burst is already running.
 */
uint32_t startBurst(uint8_t channel, uint16_t* buffer, size_t count, uint32_t sample_rate_hz);

/**
 * @brief Whether the burst has finished; tidies up the chip and SPI when it has.
 * @return true once every sample is in the buffer (and when no burst was started).
 */
bool burstDone();

/// Stops a running burst; the buffer's contents are then not valid.
void stopBurst();

}  // namespace max11635
