/**
 * @file analog_burst.cpp
 * @brief analogReadBurst(), analogStartBurst(), analogBurstDone(), analogStopBurst().
 *
 * RP2040 ADC pins: from 733 Hz up, the ADC's own clock divider times the
 * samples and DMA copies them from its FIFO, so the CPU is free. Below that
 * (the divider's limit) a repeating timer interrupt takes each sample.
 * Variant pins go to variantAnalogStartBurst() and friends.
 *
 * Samples arrive as 12-bit values; analogBurstDone() rescales the buffer to
 * analogReadResolution() once the burst has finished.
 */
#include "Arduino.h"
#include "wiring_private.h"
#include "hardware/adc.h"
#include "hardware/dma.h"
#include "pico/time.h"

namespace {

constexpr uint32_t kAdcClockHz = 48000000;   // clk_adc, from the USB PLL at every CPU speed
constexpr uint32_t kAdcCyclesPerSample = 96; // fastest: 500 ksps
constexpr uint32_t kMaxRp2040RateHz = kAdcClockHz / kAdcCyclesPerSample;
constexpr uint32_t kMinDividerRateHz = 733;  // divider tops out at 65536 cycles (732.4 Hz)
constexpr uint32_t kUsPerSecond = 1000000;

enum class Owner : uint8_t { None, Rp2040, Variant };

volatile Owner owner = Owner::None;
uint16_t* burst_buffer = nullptr;
size_t burst_count = 0;

// RP2040 ADC, divider-paced
int dma_channel = -1;

// RP2040 ADC, timer-paced
repeating_timer_t timer;
volatile size_t timer_index = 0;
volatile bool timer_running = false;

bool onTimer(repeating_timer_t*) {
    const size_t index = timer_index;
    burst_buffer[index] = adc_read();
    timer_index = index + 1;
    if (index + 1 < burst_count) return true;
    timer_running = false;
    return false;  // last sample: stop the timer
}

uint32_t startRp2040(uint8_t gpio, uint32_t sample_rate_hz) {
    if (sample_rate_hz > kMaxRp2040RateHz) return 0;
    adc_gpio_init(gpio);
    adc_select_input(gpio - 26u);

    if (sample_rate_hz < kMinDividerRateHz) {
        const uint32_t period_us = (kUsPerSecond + sample_rate_hz / 2) / sample_rate_hz;
        timer_index = 0;
        timer_running = true;
        // Negative delay: periods are measured start to start, so the rate does not drift.
        if (!add_repeating_timer_us(-static_cast<int64_t>(period_us), onTimer, nullptr, &timer)) {
            timer_running = false;
            return 0;
        }
        return (kUsPerSecond + period_us / 2) / period_us;
    }

    // Sample period in ADC clocks, in 1/256 steps (the divider is 16.8 fixed point).
    uint64_t period_256 = (static_cast<uint64_t>(kAdcClockHz) * 256 + sample_rate_hz / 2) / sample_rate_hz;
    if (period_256 < kAdcCyclesPerSample * 256) period_256 = kAdcCyclesPerSample * 256;
    adc_set_clkdiv(static_cast<float>(period_256 - 256) / 256.0f);

    adc_fifo_setup(true, true, 1, false, false);  // FIFO on, DMA request per sample, 12-bit values
    adc_fifo_drain();
    dma_channel = dma_claim_unused_channel(true);
    dma_channel_config config = dma_channel_get_default_config(static_cast<uint>(dma_channel));
    channel_config_set_transfer_data_size(&config, DMA_SIZE_16);
    channel_config_set_read_increment(&config, false);
    channel_config_set_write_increment(&config, true);
    channel_config_set_dreq(&config, DREQ_ADC);
    dma_channel_configure(static_cast<uint>(dma_channel), &config, burst_buffer, &adc_hw->fifo,
                          burst_count, true);
    adc_run(true);
    return static_cast<uint32_t>((static_cast<uint64_t>(kAdcClockHz) * 256 + period_256 / 2) / period_256);
}

// Puts the RP2040 ADC back for single reads.
void endRp2040(bool abort) {
    if (dma_channel >= 0) {
        adc_run(false);
        if (abort) dma_channel_abort(static_cast<uint>(dma_channel));
        dma_channel_unclaim(static_cast<uint>(dma_channel));
        dma_channel = -1;
        adc_fifo_setup(false, false, 0, false, false);
        adc_fifo_drain();
        adc_set_clkdiv(0);
    } else if (timer_running) {
        cancel_repeating_timer(&timer);
        timer_running = false;
    }
}

}  // namespace

uint32_t analogStartBurst(pin_size_t pin, uint16_t* buffer, size_t count, uint32_t sample_rate_hz) {
    if (!analogBurstDone() || buffer == nullptr || count == 0 || sample_rate_hz == 0) return 0;

    burst_buffer = buffer;
    burst_count = count;
    uint32_t rate = 0;
    switch (analogSourceOf(pin)) {
        case AnalogSource::Rp2040:
            rate = startRp2040(digitalPinToGpio(pin), sample_rate_hz);
            if (rate > 0) owner = Owner::Rp2040;
            break;
        case AnalogSource::Variant:
            rate = variantAnalogStartBurst(pin, buffer, count, sample_rate_hz);
            if (rate > 0) owner = Owner::Variant;
            break;
        default:
            break;
    }
    return rate;
}

bool analogBurstDone() {
    switch (owner) {
        case Owner::None:
            return true;
        case Owner::Rp2040:
            if (dma_channel >= 0 ? dma_channel_is_busy(static_cast<uint>(dma_channel)) : timer_running) {
                return false;
            }
            endRp2040(false);
            break;
        case Owner::Variant:
            if (!variantAnalogBurstDone()) return false;
            break;
    }
    const int bits = analogReadBits();
    for (size_t i = 0; i < burst_count; i++) {
        burst_buffer[i] = static_cast<uint16_t>(rescaleBits(burst_buffer[i], 12, bits));
    }
    owner = Owner::None;
    return true;
}

void analogStopBurst() {
    if (owner == Owner::Rp2040) endRp2040(true);
    if (owner == Owner::Variant) variantAnalogStopBurst();
    owner = Owner::None;
}

uint32_t analogReadBurst(pin_size_t pin, uint16_t* buffer, size_t count, uint32_t sample_rate_hz) {
    const uint32_t rate = analogStartBurst(pin, buffer, count, sample_rate_hz);
    if (rate == 0) return 0;
    while (!analogBurstDone()) yield();
    return rate;
}

void analogWaitForBurst(AnalogSource source) {
    const Owner busy = source == AnalogSource::Rp2040 ? Owner::Rp2040 : Owner::Variant;
    while (owner == busy && !analogBurstDone()) yield();
}
