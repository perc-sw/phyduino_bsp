/**
 * @file max11635.cpp
 * @brief MAX11635 register sequences (see datasheet "Programming" tables 1-7).
 *
 * Normal operation is clock mode 00: write the conversion register (channel,
 * scan mode), pulse CNVST, wait for EOC, read the results from the FIFO.
 * Fast bursts use clock mode 11 for their duration (see startClockedBurst()).
 */
#include "max11635.h"

#include "Arduino.h"
#include "hardware/dma.h"
#include "hardware/gpio.h"
#include "hardware/spi.h"
#include "pico/time.h"

namespace {

constexpr uint32_t kSpiHz            = 4 * 1000 * 1000;
constexpr uint32_t kMinClockedSclkHz = 100 * 1000;          // datasheet: externally clocked, 0.1-4.8 MHz
constexpr uint32_t kMaxClockedSclkHz = 4800 * 1000;
constexpr uint32_t kSclkPerSample    = 16;                  // clock mode 11: 16 SCLK per conversion
constexpr uint32_t kUsPerSecond      = 1000000;
// EOC wait: about 5 µs per conversion (acquisition + conversion) plus wake-up;
// allow twice that.
constexpr uint32_t kEocBaseUs        = 20;
constexpr uint32_t kEocPerSampleUs   = 12;

// Register select bits (top bits of each command byte).
constexpr uint8_t kRegConversion = 0x80;   // 1xxx xxxx
constexpr uint8_t kRegSetup      = 0x40;   // 01xx xxxx
constexpr uint8_t kRegAveraging  = 0x20;   // 001x xxxx
constexpr uint8_t kRegReset      = 0x10;   // 0001 xxxx

// Conversion: channel in bits 5:3, scan mode in bits 2:1.
constexpr uint8_t kScanFromZero  = 0x00;   // SCAN = 00: channels 0..N
constexpr uint8_t kNoScan        = 0x06;   // SCAN = 11: channel N once
// Setup: REFSEL = 01 (external single-ended reference) always.
constexpr uint8_t kRefExternal   = 0x04;
constexpr uint8_t kClockMode00   = 0x00;   // internal timing, CNVST start
constexpr uint8_t kClockMode11   = 0x30;   // SCLK times each conversion
constexpr uint8_t kUnipolarNext  = 0x02;   // DIFFSEL = 10: unipolar-mode byte follows
constexpr uint8_t kBipolarNext   = 0x03;   // DIFFSEL = 11: bipolar-mode byte follows
constexpr uint8_t kSetup         = kRegSetup | kClockMode00 | kRefExternal;
// Averaging: AVGON bit 4, NAVG bits 3:2 (4, 8, 16, 32 samples).
constexpr uint8_t kAverageOn     = 0x10;
// Reset: bit 3 = 1 clears the FIFO only; 0 resets every register.
constexpr uint8_t kResetFifo     = kRegReset | 0x08;
constexpr uint8_t kResetAll      = kRegReset;

bool started = false;
uint8_t average_samples = 1;

// --- Burst state -------------------------------------------------------------------
enum class Burst : uint8_t { Idle, Timed, Clocked };
volatile Burst burst = Burst::Idle;
uint16_t* burst_buffer = nullptr;
size_t burst_count = 0;
volatile size_t burst_index = 0;
repeating_timer_t burst_timer;
int dma_tx = -1;
int dma_rx = -1;
uint16_t clocked_tx_word = 0;   // conversion byte, then a zero byte: one 16-bit frame per sample

void select()   { gpio_put(PHYDUINO_EXT_ADC_GPIO_CS, false); }
void deselect() { gpio_put(PHYDUINO_EXT_ADC_GPIO_CS, true); }

void writeBytes(const uint8_t* data, size_t length) {
    select();
    spi_write_blocking(PHYDUINO_EXT_ADC_SPI, data, length);
    deselect();
}

void writeByte(uint8_t value) { writeBytes(&value, 1); }

void outputHigh(uint gpio) {
    gpio_init(gpio);
    gpio_put(gpio, true);
    gpio_set_dir(gpio, GPIO_OUT);
}

uint8_t conversionByte(uint8_t channel, uint8_t scan) {
    return static_cast<uint8_t>(kRegConversion | (channel << 3) | scan);
}

uint8_t averagingByte() {
    switch (average_samples) {
        case 4:  return kRegAveraging | kAverageOn | (0 << 2);
        case 8:  return kRegAveraging | kAverageOn | (1 << 2);
        case 16: return kRegAveraging | kAverageOn | (2 << 2);
        case 32: return kRegAveraging | kAverageOn | (3 << 2);
        default: return kRegAveraging;  // off
    }
}

// Sets every input single-ended (unipolar and bipolar registers cleared).
void setSingleEnded() {
    const uint8_t unipolar[] = {kSetup | kUnipolarNext, 0x00};
    writeBytes(unipolar, sizeof(unipolar));
    const uint8_t bipolar[] = {kSetup | kBipolarNext, 0x00};
    writeBytes(bipolar, sizeof(bipolar));
}

// Waits for a running burst to finish, so single conversions do not interfere.
void waitIdle() {
    while (burst != Burst::Idle && !max11635::burstDone()) yield();
}

// CNVST low pulse (>= 40 ns) starts the programmed conversions; then waits for
// EOC and reads `count` results from the FIFO. false if EOC never falls.
bool convert(uint16_t* results, uint8_t count, uint8_t samples) {
    gpio_put(PHYDUINO_EXT_ADC_GPIO_CNVST, false);
    busy_wait_at_least_cycles(F_CPU / 10000000);  // 100 ns at any CPU speed
    gpio_put(PHYDUINO_EXT_ADC_GPIO_CNVST, true);

    const uint32_t timeout_us = kEocBaseUs + kEocPerSampleUs * samples * count;
    const uint32_t start = time_us_32();
    while (gpio_get(PHYDUINO_EXT_ADC_GPIO_EOC)) {
        if (time_us_32() - start > timeout_us) return false;
    }

    uint8_t data[2 * max11635::kChannels];
    select();
    spi_read_blocking(PHYDUINO_EXT_ADC_SPI, 0x00, data, 2u * count);
    deselect();
    for (uint8_t i = 0; i < count; i++) {
        results[i] = static_cast<uint16_t>(((data[2 * i] << 8) | data[2 * i + 1]) & 0x0FFF);
    }
    return true;
}

// --- Timed burst (clock mode 00, one conversion per timer tick) ----------------------
bool onBurstTimer(repeating_timer_t*) {
    uint16_t sample = 0;
    if (!convert(&sample, 1, 1)) sample = 0;
    const size_t index = burst_index;
    burst_buffer[index] = sample;
    burst_index = index + 1;
    return index + 1 < burst_count;  // false stops the timer after the last sample
}

uint32_t startTimedBurst(uint8_t channel, uint32_t sample_rate_hz) {
    writeByte(kRegAveraging);  // averaging off: one conversion per sample
    writeByte(conversionByte(channel, kNoScan));
    const uint32_t period_us = (kUsPerSecond + sample_rate_hz / 2) / sample_rate_hz;
    burst_index = 0;
    burst = Burst::Timed;
    // Negative delay: periods measured start to start, so the rate does not drift.
    if (!add_repeating_timer_us(-static_cast<int64_t>(period_us), onBurstTimer, nullptr, &burst_timer)) {
        burst = Burst::Idle;
        writeByte(averagingByte());
        return 0;
    }
    return (kUsPerSecond + period_us / 2) / period_us;
}

// --- Clocked burst (clock mode 11) ----------------------------------------------------
// Each conversion takes 16 SCLK: the conversion byte, then 8 more clocks. Sent
// as 16-bit frames [conversion byte, 0x00], the result of conversion i is the
// low byte of received frame i followed by the high byte of frame i + 1 (four
// leading zeros, then 12 bits). One extra frame of zeros clocks out the last
// result without starting another conversion.
uint32_t startClockedBurst(uint8_t channel, uint32_t sample_rate_hz) {
    uint32_t sclk = sample_rate_hz * kSclkPerSample;
    if (sclk > kMaxClockedSclkHz) sclk = kMaxClockedSclkHz;
    if (sclk < kMinClockedSclkHz) sclk = kMinClockedSclkHz;

    writeByte(kRegSetup | kClockMode11 | kRefExternal);  // no unipolar/bipolar byte follows
    const uint actual_sclk = spi_set_baudrate(PHYDUINO_EXT_ADC_SPI, sclk);
    spi_set_format(PHYDUINO_EXT_ADC_SPI, 16, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);
    clocked_tx_word = static_cast<uint16_t>(conversionByte(channel, kNoScan) << 8);

    dma_tx = dma_claim_unused_channel(true);
    dma_rx = dma_claim_unused_channel(true);
    dma_channel_config tx = dma_channel_get_default_config(static_cast<uint>(dma_tx));
    channel_config_set_transfer_data_size(&tx, DMA_SIZE_16);
    channel_config_set_read_increment(&tx, false);
    channel_config_set_write_increment(&tx, false);
    channel_config_set_dreq(&tx, spi_get_dreq(PHYDUINO_EXT_ADC_SPI, true));
    dma_channel_config rx = dma_channel_get_default_config(static_cast<uint>(dma_rx));
    channel_config_set_transfer_data_size(&rx, DMA_SIZE_16);
    channel_config_set_read_increment(&rx, false);
    channel_config_set_write_increment(&rx, true);
    channel_config_set_dreq(&rx, spi_get_dreq(PHYDUINO_EXT_ADC_SPI, false));
    dma_channel_configure(static_cast<uint>(dma_tx), &tx, &spi_get_hw(PHYDUINO_EXT_ADC_SPI)->dr,
                          &clocked_tx_word, burst_count, false);
    dma_channel_configure(static_cast<uint>(dma_rx), &rx, burst_buffer,
                          &spi_get_hw(PHYDUINO_EXT_ADC_SPI)->dr, burst_count, false);

    burst = Burst::Clocked;
    select();  // CS stays low for the whole burst
    dma_start_channel_mask((1u << dma_tx) | (1u << dma_rx));
    return actual_sclk / kSclkPerSample;
}

// Back to clock mode 00, 8-bit SPI frames, and the averaging setting.
void endClockedBurst(bool aborted) {
    if (aborted) {
        dma_channel_abort(static_cast<uint>(dma_tx));
        dma_channel_abort(static_cast<uint>(dma_rx));
    } else {
        const uint16_t zero = 0;
        uint16_t last = 0;
        spi_write16_read16_blocking(PHYDUINO_EXT_ADC_SPI, &zero, &last, 1);
        for (size_t i = 0; i < burst_count; i++) {
            const uint16_t next = i + 1 < burst_count ? burst_buffer[i + 1] : last;
            burst_buffer[i] = static_cast<uint16_t>((((burst_buffer[i] & 0xFF) << 8) | (next >> 8)) & 0x0FFF);
        }
    }
    deselect();
    while (spi_is_readable(PHYDUINO_EXT_ADC_SPI)) (void)spi_get_hw(PHYDUINO_EXT_ADC_SPI)->dr;
    dma_channel_unclaim(static_cast<uint>(dma_tx));
    dma_channel_unclaim(static_cast<uint>(dma_rx));
    dma_tx = dma_rx = -1;

    spi_set_format(PHYDUINO_EXT_ADC_SPI, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);
    spi_set_baudrate(PHYDUINO_EXT_ADC_SPI, kSpiHz);
    const uint8_t setup[] = {kSetup | kUnipolarNext, 0x00};
    writeBytes(setup, sizeof(setup));
}

}  // namespace

namespace max11635 {

void begin() {
    waitIdle();
    spi_init(PHYDUINO_EXT_ADC_SPI, kSpiHz);
    spi_set_format(PHYDUINO_EXT_ADC_SPI, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);
    gpio_set_function(PHYDUINO_EXT_ADC_GPIO_SCK,  GPIO_FUNC_SPI);
    gpio_set_function(PHYDUINO_EXT_ADC_GPIO_MOSI, GPIO_FUNC_SPI);
    gpio_set_function(PHYDUINO_EXT_ADC_GPIO_MISO, GPIO_FUNC_SPI);
    outputHigh(PHYDUINO_EXT_ADC_GPIO_CS);
    outputHigh(PHYDUINO_EXT_ADC_GPIO_CNVST);
    gpio_init(PHYDUINO_EXT_ADC_GPIO_EOC);   // input, driven by the ADC

    writeByte(kResetFifo);
    writeByte(kResetAll);
    sleep_ms(5);
    const uint8_t setup[] = {kSetup | kUnipolarNext, 0x00};  // every input single-ended
    writeBytes(setup, sizeof(setup));
    writeByte(averagingByte());
    started = true;
}

void setAveraging(uint8_t samples) {
    waitIdle();
    average_samples = samples >= 32 ? 32 : samples >= 16 ? 16 : samples >= 8 ? 8 : samples >= 4 ? 4 : 1;
    if (started) writeByte(averagingByte());
}

uint8_t averaging() { return average_samples; }

uint16_t read(uint8_t channel) {
    waitIdle();
    if (!started) begin();
    if (channel >= kChannels) return 0;

    writeByte(conversionByte(channel, kNoScan));
    uint16_t result = 0;
    return convert(&result, 1, average_samples) ? result : 0;
}

bool scan(uint8_t last_channel, uint16_t* results) {
    waitIdle();
    if (!started) begin();
    if (last_channel >= kChannels || results == nullptr) return false;

    writeByte(kResetFifo);  // only this scan's results in the FIFO
    writeByte(conversionByte(last_channel, kScanFromZero));
    return convert(results, static_cast<uint8_t>(last_channel + 1), average_samples);
}

bool readDifferential(uint8_t pair, bool bipolar, int16_t* code) {
    waitIdle();
    if (!started) begin();
    if (pair > 1 || code == nullptr) return false;

    const uint8_t mode[] = {static_cast<uint8_t>(kSetup | (bipolar ? kBipolarNext : kUnipolarNext)),
                            static_cast<uint8_t>(0x80 >> pair)};  // UCH0/1 or BCH0/1 = bit 7
    writeBytes(mode, sizeof(mode));
    writeByte(conversionByte(static_cast<uint8_t>(2 * pair), kNoScan));  // lower channel of the pair
    uint16_t raw = 0;
    const bool ok = convert(&raw, 1, average_samples);
    setSingleEnded();
    if (!ok) return false;
    // Bipolar results are 12-bit two's complement.
    *code = static_cast<int16_t>(bipolar && (raw & 0x800) ? static_cast<int>(raw) - 4096 : raw);
    return true;
}

uint32_t startBurst(uint8_t channel, uint16_t* buffer, size_t count, uint32_t sample_rate_hz) {
    waitIdle();
    if (!started) begin();
    if (channel >= kChannels || buffer == nullptr || count == 0 ||
        sample_rate_hz == 0 || sample_rate_hz > kMaxBurstHz) {
        return 0;
    }
    burst_buffer = buffer;
    burst_count = count;
    return sample_rate_hz <= kMaxTimedBurstHz ? startTimedBurst(channel, sample_rate_hz)
                                              : startClockedBurst(channel, sample_rate_hz);
}

bool burstDone() {
    switch (burst) {
        case Burst::Idle:
            return true;
        case Burst::Timed:
            if (burst_index < burst_count) return false;
            writeByte(averagingByte());
            break;
        case Burst::Clocked:
            if (dma_channel_is_busy(static_cast<uint>(dma_rx))) return false;
            endClockedBurst(false);
            writeByte(averagingByte());
            break;
    }
    burst = Burst::Idle;
    return true;
}

void stopBurst() {
    if (burst == Burst::Timed) {
        cancel_repeating_timer(&burst_timer);
    } else if (burst == Burst::Clocked) {
        endClockedBurst(true);
    }
    if (burst != Burst::Idle) writeByte(averagingByte());
    burst = Burst::Idle;
}

}  // namespace max11635
