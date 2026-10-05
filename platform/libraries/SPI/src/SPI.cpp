/**
 * @file SPI.cpp
 * @brief SPI controller. The RP2040 SPI only shifts MSB first, so LSBFIRST
 *        is done by reversing each byte in software.
 */
#include "SPI.h"

#include "hardware/gpio.h"

PhyduinoSPI SPI(PHYDUINO_SPI_INSTANCE, PIN_SPI_SCK, PIN_SPI_MOSI, PIN_SPI_MISO);

static uint8_t reverseBits(uint8_t b) {
    b = static_cast<uint8_t>((b & 0xF0) >> 4 | (b & 0x0F) << 4);
    b = static_cast<uint8_t>((b & 0xCC) >> 2 | (b & 0x33) << 2);
    b = static_cast<uint8_t>((b & 0xAA) >> 1 | (b & 0x55) << 1);
    return b;
}

void PhyduinoSPI::begin() {
    spi_init(spi_, settings_.getClockFreq());
    beginTransaction(settings_);
    gpio_set_function(digitalPinToGpio(sck_),  GPIO_FUNC_SPI);
    gpio_set_function(digitalPinToGpio(mosi_), GPIO_FUNC_SPI);
    gpio_set_function(digitalPinToGpio(miso_), GPIO_FUNC_SPI);
}

void PhyduinoSPI::end() {
    spi_deinit(spi_);
    gpio_init(digitalPinToGpio(sck_));
    gpio_init(digitalPinToGpio(mosi_));
    gpio_init(digitalPinToGpio(miso_));
}

void PhyduinoSPI::beginTransaction(arduino::SPISettings settings) {
    settings_ = settings;
    const SPIMode mode = settings.getDataMode();
    const spi_cpol_t cpol = (mode == SPI_MODE2 || mode == SPI_MODE3) ? SPI_CPOL_1 : SPI_CPOL_0;
    const spi_cpha_t cpha = (mode == SPI_MODE1 || mode == SPI_MODE3) ? SPI_CPHA_1 : SPI_CPHA_0;
    spi_set_baudrate(spi_, settings.getClockFreq());
    spi_set_format(spi_, 8, cpol, cpha, SPI_MSB_FIRST);
    lsb_first_ = settings.getBitOrder() == LSBFIRST;
}

uint8_t PhyduinoSPI::transfer(uint8_t data) {
    uint8_t out = lsb_first_ ? reverseBits(data) : data;
    uint8_t in = 0;
    spi_write_read_blocking(spi_, &out, &in, 1);
    return lsb_first_ ? reverseBits(in) : in;
}

uint16_t PhyduinoSPI::transfer16(uint16_t data) {
    // Send the byte that goes on the wire first, first.
    if (lsb_first_) {
        const uint8_t lo = transfer(static_cast<uint8_t>(data));
        const uint8_t hi = transfer(static_cast<uint8_t>(data >> 8));
        return static_cast<uint16_t>(hi << 8 | lo);
    }
    const uint8_t hi = transfer(static_cast<uint8_t>(data >> 8));
    const uint8_t lo = transfer(static_cast<uint8_t>(data));
    return static_cast<uint16_t>(hi << 8 | lo);
}

void PhyduinoSPI::transfer(void* buffer, size_t count) {
    uint8_t* bytes = static_cast<uint8_t*>(buffer);
    if (lsb_first_) {
        for (size_t i = 0; i < count; i++) bytes[i] = transfer(bytes[i]);
    } else {
        spi_write_read_blocking(spi_, bytes, bytes, count);  // in-place is allowed
    }
}
