/**
 * @file chip_models.h
 * @brief Behavioural models of the Phyduino Pico's on-board chips, attached to
 *        the fake SDK's SPI / I2C / GPIO so the variant drivers can be tested.
 *
 * Each model follows the datasheet behaviour the driver relies on, and records
 * what the driver did so tests can check it.
 */
#pragma once

#include <cstdint>
#include <vector>

/// MAX11635 12-bit ADC on SPI0 (CS GPIO21, CNVST GPIO18, EOC GPIO19).
/// Conversion: command byte 1cccc11x selects channel; a CNVST low pulse
/// converts; EOC goes low; the next two bytes read clock out the 12-bit result.
struct Max11635Model {
    uint16_t channel_value[4] = {0, 0, 0, 0};   // 12-bit result per channel
    bool respond = true;                         // false: chip missing (EOC never falls)
    std::vector<uint8_t> commands;               // command bytes received (not read clocks)
    uint8_t setup_register = 0;
    uint8_t unipolar_register = 0xFF;
    int resets = 0;

    void attach();   // hooks into fake::hw; call after fake::reset()

private:
    uint8_t channel_ = 0;
    bool expect_unipolar_byte_ = false;
    int result_bytes_left_ = 0;
    uint16_t result_ = 0;
    bool cnvst_low_ = false;
    uint8_t onByte(uint8_t out);
    void onGpioPut(unsigned gpio, bool value);
};

/// STUSB4500 USB-PD controller on I2C1, address 0x28: a 256-byte register
/// file with the chip's power-on defaults (ST NVM defaults: 5 V/1.5 A,
/// 15 V/1.5 A, 20 V/1 A) and soft-reset detection.
struct Stusb4500Model {
    uint8_t regs[256] = {};
    bool present = true;
    int soft_resets = 0;      // TX_HEADER=Soft_Reset followed by "send command"
    int register_writes = 0;  // write transactions with data

    void attach();
    uint32_t pdo(int index) const;             // 0-2, little-endian 32-bit
    void setPdo(int index, uint32_t value);
    void setContract(uint8_t position, uint16_t operating_10ma);  // RDO_REG_STATUS

private:
    uint8_t pointer_ = 0;
};
