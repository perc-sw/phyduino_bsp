/**
 * @file SPI.h
 * @brief Arduino SPI on the header pins: SCK = D13, MOSI = D11, MISO = D12.
 *
 * SS (D10) is not driven automatically; use digitalWrite() like on an Uno.
 * The peripheral and pins are set in the variant's pins_arduino.h
 * (PHYDUINO_SPI_INSTANCE, PIN_SPI_*).
 */
#pragma once

#include "Arduino.h"
#include "api/HardwareSPI.h"
#include "hardware/spi.h"

class PhyduinoSPI : public arduino::HardwareSPI {
public:
    PhyduinoSPI(spi_inst_t* spi, pin_size_t sck, pin_size_t mosi, pin_size_t miso)
        : spi_(spi), sck_(sck), mosi_(mosi), miso_(miso) {}

    void begin() override;
    void end() override;

    void beginTransaction(arduino::SPISettings settings) override;
    void endTransaction() override {}

    uint8_t transfer(uint8_t data) override;
    uint16_t transfer16(uint16_t data) override;
    void transfer(void* buffer, size_t count) override;

    // Interrupt-safe transactions are not needed in v0.1: no-ops.
    void usingInterrupt(int) override {}
    void notUsingInterrupt(int) override {}
    void attachInterrupt() override {}
    void detachInterrupt() override {}

private:
    spi_inst_t* spi_;
    pin_size_t sck_, mosi_, miso_;
    arduino::SPISettings settings_;
    bool lsb_first_ = false;
};

extern PhyduinoSPI SPI;
