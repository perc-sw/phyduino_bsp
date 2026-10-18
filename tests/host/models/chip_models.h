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
#include <deque>
#include <functional>
#include <vector>

/// MAX11635 12-bit ADC on SPI0 (CS GPIO21, CNVST GPIO18, EOC GPIO19).
///
/// Clock mode 00: the conversion register (1 cccc ss x) selects channel N and
/// scan mode (11 = N once, 00 = channels 0..N); a CNVST low pulse converts;
/// EOC goes low; the following bytes read clock the results out of the FIFO,
/// 2 bytes each. Unipolar/bipolar registers (after a setup byte with DIFFSEL
/// 10/11) make AIN0/AIN1 or AIN2/AIN3 a differential pair.
///
/// Clock mode 11 (setup CKSEL = 11): each conversion byte starts a
/// conversion whose 16-bit result comes out on the next two bytes clocked,
/// while CS stays low.
struct Max11635Model {
    uint16_t channel_value[4] = {0, 0, 0, 0};   // 12-bit single-ended result per channel
    /// Optional: value of each conversion (overrides channel_value), e.g. a ramp.
    std::function<uint16_t(uint8_t channel)> sample;
    bool respond = true;                         // false: chip missing (EOC never falls)
    std::vector<uint8_t> commands;               // command bytes received (not data or read clocks)
    uint8_t setup_register = 0;
    uint8_t unipolar_register = 0;              // power-up state: every register 0
    uint8_t bipolar_register = 0;
    uint8_t averaging_register = 0;
    int resets = 0;
    int fifo_resets = 0;
    int cnvst_pulses = 0;
    int conversions = 0;                         // results produced (each averaged result counts once)

    void attach();   // hooks into fake::hw; call after fake::reset()

private:
    uint8_t channel_ = 0;
    uint8_t scan_ = 3;
    enum class Expect { Command, Unipolar, Bipolar } expect_ = Expect::Command;
    std::deque<uint8_t> out_;                    // bytes waiting on DOUT
    bool cnvst_low_ = false;
    bool clockMode11() const { return (setup_register & 0x30) == 0x30; }
    uint16_t convertChannel(uint8_t channel);
    void pushResult(uint16_t result);
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
