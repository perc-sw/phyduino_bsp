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

/**
 * @brief SPI controller on one RP2040 SPI peripheral; sketches use SPI.
 *
 * Transfers 8 bits at a time. LSBFIRST is supported (done in software).
 */
class PhyduinoSPI : public arduino::HardwareSPI {
public:
    /**
     * @brief Binds the controller to an SPI peripheral and its pins; nothing is set up until begin().
     * @param spi pico-sdk SPI (spi0 or spi1).
     * @param sck Arduino pin for SCK.
     * @param mosi Arduino pin for MOSI.
     * @param miso Arduino pin for MISO.
     */
    PhyduinoSPI(spi_inst_t* spi, pin_size_t sck, pin_size_t mosi, pin_size_t miso)
        : spi_(spi), sck_(sck), mosi_(mosi), miso_(miso) {}

    /**
     * @brief Starts the controller and connects SCK, MOSI and MISO.
     *
     * Uses the settings of the last beginTransaction(), or 4 MHz, MSBFIRST,
     * SPI_MODE0 if there was none.
     */
    void begin() override;

    /**
     * @brief Stops the controller and frees its pins.
     */
    void end() override;

    /**
     * @brief Applies the clock, bit order and mode for the next transfers.
     * @param settings E.g. `SPISettings(1000000, MSBFIRST, SPI_MODE0)`.
     */
    void beginTransaction(arduino::SPISettings settings) override;

    /**
     * @brief Ends a transaction. Does nothing; kept for Arduino compatibility.
     */
    void endTransaction() override {}

    /**
     * @brief Sends one byte and returns the byte received at the same time.
     * @param data Byte to send.
     * @return Byte received.
     */
    uint8_t transfer(uint8_t data) override;

    /**
     * @brief Sends 16 bits as two bytes, in the bit order of the transaction.
     * @param data Value to send.
     * @return Value received.
     */
    uint16_t transfer16(uint16_t data) override;

    /**
     * @brief Sends a buffer and replaces its contents with the bytes received.
     * @param buffer Bytes to send; overwritten with the received bytes.
     * @param count Number of bytes.
     */
    void transfer(void* buffer, size_t count) override;

    /// @name Interrupt-safe transactions
    /// Not needed in v0.1: these do nothing.
    /// @{
    void usingInterrupt(int) override {}     ///< Does nothing.
    void notUsingInterrupt(int) override {}  ///< Does nothing.
    void attachInterrupt() override {}       ///< Does nothing.
    void detachInterrupt() override {}       ///< Does nothing.
    /// @}

private:
    spi_inst_t* spi_;
    pin_size_t sck_, mosi_, miso_;
    arduino::SPISettings settings_;
    bool lsb_first_ = false;
};

/// SPI on the header: SCK D13, MOSI D11, MISO D12.
extern PhyduinoSPI SPI;
