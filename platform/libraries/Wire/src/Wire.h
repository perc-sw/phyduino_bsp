/**
 * @file Wire.h
 * @brief Arduino Wire (I2C) on the SDA/SCL header pins.
 *
 * - Controller: begin(), beginTransmission(), write(), endTransmission(),
 *   requestFrom(), read().
 * - Peripheral: begin(address) with onReceive() / onRequest().
 *
 * The board has no I2C pull-up resistors; the RP2040's weak internal pull-ups
 * are enabled, which is enough for short wires at 100 kHz. Add 4.7 kOhm
 * resistors to 3.3 V for longer buses or 400 kHz.
 *
 * Peripheral and pins: pins_arduino.h (PHYDUINO_WIRE_INSTANCE, PIN_WIRE_*).
 */
#pragma once

#include "Arduino.h"
#include "api/HardwareI2C.h"
#include "api/RingBuffer.h"
#include "hardware/i2c.h"

class PhyduinoWire : public arduino::HardwareI2C {
public:
    static constexpr size_t kBufferSize = 256;

    PhyduinoWire(i2c_inst_t* i2c, pin_size_t sda, pin_size_t scl)
        : i2c_(i2c), sda_(sda), scl_(scl) {}

    void begin() override;                 // controller
    void begin(uint8_t address) override;  // peripheral at `address`
    void end() override;
    void setClock(uint32_t hz) override;

    void beginTransmission(uint8_t address) override;
    uint8_t endTransmission(bool sendStop) override;
    uint8_t endTransmission() override { return endTransmission(true); }

    size_t requestFrom(uint8_t address, size_t length, bool sendStop) override;
    size_t requestFrom(uint8_t address, size_t length) override { return requestFrom(address, length, true); }

    size_t write(uint8_t data) override;
    size_t write(const uint8_t* data, size_t length) override;
    using arduino::Print::write;
    int available() override;
    int read() override;
    int peek() override;
    void flush() override {}

    void onReceive(void (*callback)(int)) override { on_receive_ = callback; }
    void onRequest(void (*callback)()) override    { on_request_ = callback; }

    void handlePeripheralEvent(int event);  // internal: called from the I2C IRQ

private:
    void setupPins();

    i2c_inst_t* i2c_;
    pin_size_t sda_, scl_;
    uint32_t clock_hz_ = 100000;
    bool started_ = false;
    uint8_t tx_address_ = 0;
    uint8_t tx_buffer_[kBufferSize];
    size_t tx_length_ = 0;
    arduino::RingBufferN<kBufferSize> rx_buffer_;

    // Peripheral mode
    bool peripheral_mode_ = false;
    void (*on_receive_)(int) = nullptr;
    void (*on_request_)() = nullptr;
    size_t tx_sent_ = 0;          // bytes of tx_buffer_ already sent to the controller
    bool request_started_ = false;
};

extern PhyduinoWire Wire;
