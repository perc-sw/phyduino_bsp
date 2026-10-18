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

/**
 * @brief I2C controller or peripheral on one RP2040 I2C peripheral; sketches use Wire.
 *
 * Every transfer gives up after 50 ms if the bus is stuck.
 */
class PhyduinoWire : public arduino::HardwareI2C {
public:
    /// Largest number of bytes in one transmission or request.
    static constexpr size_t kBufferSize = 256;

    /**
     * @brief Binds Wire to an I2C peripheral and its pins; nothing is set up until begin().
     * @param i2c pico-sdk I2C (i2c0 or i2c1).
     * @param sda Arduino pin for SDA.
     * @param scl Arduino pin for SCL.
     */
    PhyduinoWire(i2c_inst_t* i2c, pin_size_t sda, pin_size_t scl)
        : i2c_(i2c), sda_(sda), scl_(scl) {}

    /**
     * @brief Starts as a controller, at 100 kHz unless setClock() was called.
     */
    void begin() override;

    /**
     * @brief Starts as a peripheral that answers at `address`.
     *
     * Set onReceive() and onRequest() first.
     * @param address 7-bit address, 0x08-0x77.
     */
    void begin(uint8_t address) override;

    /**
     * @brief Stops I2C and frees the pins.
     */
    void end() override;

    /**
     * @brief Sets the bus clock; applies at once if already started.
     * @param hz Clock in Hz, usually 100000 or 400000.
     */
    void setClock(uint32_t hz) override;

    /// @name Controller
    /// @{

    /**
     * @brief Starts a write to a device; queue the bytes with write().
     * @param address 7-bit device address.
     */
    void beginTransmission(uint8_t address) override;

    /**
     * @brief Sends the queued bytes.
     *
     * With no bytes queued it only checks that the device answers, which is
     * what I2C scanners use.
     * @param sendStop true to end with a stop; false to keep the bus for a
     *                 following requestFrom() (repeated start).
     * @return 0 on success, 2 if the device did not answer, 5 on timeout,
     *         4 on any other error.
     */
    uint8_t endTransmission(bool sendStop) override;

    /**
     * @brief Sends the queued bytes and ends with a stop.
     * @return As endTransmission(bool).
     */
    uint8_t endTransmission() override { return endTransmission(true); }

    /**
     * @brief Reads bytes from a device into the receive buffer; take them with read().
     * @param address 7-bit device address.
     * @param length Number of bytes, at most kBufferSize.
     * @param sendStop true to end with a stop; false to keep the bus.
     * @return Number of bytes received, 0 on error.
     */
    size_t requestFrom(uint8_t address, size_t length, bool sendStop) override;

    /**
     * @brief Reads bytes from a device and ends with a stop.
     * @param address 7-bit device address.
     * @param length Number of bytes, at most kBufferSize.
     * @return Number of bytes received, 0 on error.
     */
    size_t requestFrom(uint8_t address, size_t length) override { return requestFrom(address, length, true); }

    /// @}

    /// @name Data (controller and peripheral)
    /// @{

    /**
     * @brief Queues one byte: for endTransmission(), or as the reply in onRequest().
     * @param data Byte to queue.
     * @return 1, or 0 if the buffer is full.
     */
    size_t write(uint8_t data) override;

    /**
     * @brief Queues several bytes: for endTransmission(), or as the reply in onRequest().
     * @param data Bytes to queue.
     * @param length Number of bytes.
     * @return Number of bytes queued; fewer than `length` if the buffer is full.
     */
    size_t write(const uint8_t* data, size_t length) override;
    using arduino::Print::write;

    /**
     * @brief Number of received bytes waiting to be read.
     * @return Byte count.
     */
    int available() override;

    /**
     * @brief Takes the next received byte.
     * @return 0-255, or -1 if there is none.
     */
    int read() override;

    /**
     * @brief Next received byte, left in the buffer.
     * @return 0-255, or -1 if there is none.
     */
    int peek() override;

    /**
     * @brief Does nothing; kept for Arduino compatibility.
     */
    void flush() override {}

    /// @}

    /// @name Peripheral
    /// The callbacks run inside the I2C interrupt: keep them short.
    /// @{

    /**
     * @brief Sets the function called after a controller has written to this board.
     * @param callback Function taking the number of bytes received; read them with read().
     */
    void onReceive(void (*callback)(int)) override { on_receive_ = callback; }

    /**
     * @brief Sets the function called when a controller reads from this board.
     * @param callback Function that queues the reply with write().
     */
    void onRequest(void (*callback)()) override    { on_request_ = callback; }

    /// @}

    /// @private Called from the I2C interrupt; not for sketches.
    void handlePeripheralEvent(int event);

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

/// I2C on the header SDA / SCL pins.
extern PhyduinoWire Wire;
