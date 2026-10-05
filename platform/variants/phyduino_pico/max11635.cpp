/**
 * @file max11635.cpp
 * @brief MAX11635 register sequences (see datasheet "Programming" tables).
 */
#include "max11635.h"

#include "Arduino.h"
#include "hardware/gpio.h"
#include "hardware/spi.h"
#include "pico/time.h"

namespace {

constexpr uint32_t kSpiHz        = 4 * 1000 * 1000;
constexpr uint32_t kEocTimeoutUs = 100;    // a conversion takes a few microseconds

// Register select bits (top bits of each command byte).
constexpr uint8_t kRegConversion = 0x80;   // 1xxx xxxx
constexpr uint8_t kRegSetup      = 0x40;   // 01xx xxxx
constexpr uint8_t kRegReset      = 0x10;   // 0001 xxxx

// Conversion: channel in bits 6:3, SCAN = 11 (single channel, no scan).
constexpr uint8_t kNoScan        = 0x06;
// Setup: CKSEL = 00 (internal timing, CNVST start), REFSEL = 01 (external
// reference), DIFFSEL = 10 (a unipolar-mode byte follows; 0 = all single-ended).
constexpr uint8_t kSetup         = kRegSetup | 0x04 | 0x02;
constexpr uint8_t kUnipolarAll   = 0x00;
// Reset: bit 3 = 1 clears the FIFO only; 0 resets every register.
constexpr uint8_t kResetFifo     = kRegReset | 0x08;
constexpr uint8_t kResetAll      = kRegReset;

bool started = false;

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

}  // namespace

namespace max11635 {

void begin() {
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
    const uint8_t setup[] = {kSetup, kUnipolarAll};
    writeBytes(setup, sizeof(setup));
    started = true;
}

uint16_t read(uint8_t channel) {
    if (!started) begin();
    if (channel > 3) return 0;

    writeByte(kRegConversion | static_cast<uint8_t>(channel << 3) | kNoScan);

    // CNVST low pulse (>= 40 ns) starts the conversion.
    gpio_put(PHYDUINO_EXT_ADC_GPIO_CNVST, false);
    busy_wait_at_least_cycles(8);
    gpio_put(PHYDUINO_EXT_ADC_GPIO_CNVST, true);

    const uint32_t start = time_us_32();
    while (gpio_get(PHYDUINO_EXT_ADC_GPIO_EOC)) {
        if (time_us_32() - start > kEocTimeoutUs) return 0;
    }

    uint8_t data[2] = {0, 0};
    select();
    spi_read_blocking(PHYDUINO_EXT_ADC_SPI, 0x00, data, sizeof(data));
    deselect();
    return static_cast<uint16_t>(((data[0] << 8) | data[1]) & 0x0FFF);
}

}  // namespace max11635
