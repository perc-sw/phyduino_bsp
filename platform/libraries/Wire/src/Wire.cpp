/**
 * @file Wire.cpp
 * @brief I2C controller (pico-sdk hardware_i2c) and peripheral (pico_i2c_slave).
 */
#include "Wire.h"

#include "hardware/gpio.h"
#include "pico/i2c_slave.h"

PhyduinoWire Wire(PHYDUINO_WIRE_INSTANCE, PIN_WIRE_SDA, PIN_WIRE_SCL);

// Arduino endTransmission() result codes.
enum : uint8_t { kOk = 0, kNack = 2, kOther = 4, kTimeout = 5 };

static constexpr uint32_t kTimeoutUs = 50 * 1000;

void PhyduinoWire::setupPins() {
    const pin_size_t pins[] = {sda_, scl_};
    for (pin_size_t pin : pins) {
        const uint gpio = digitalPinToGpio(pin);
        gpio_set_function(gpio, GPIO_FUNC_I2C);
        gpio_pull_up(gpio);
        // Schematic note: GPIO25 (SCL) needs 12 mA drive; harmless on SDA.
        gpio_set_drive_strength(gpio, GPIO_DRIVE_STRENGTH_12MA);
    }
}

void PhyduinoWire::begin() {
    i2c_init(i2c_, clock_hz_);
    setupPins();
    started_ = true;
}

static void peripheralHandler(i2c_inst_t*, i2c_slave_event_t event) {
    Wire.handlePeripheralEvent(static_cast<int>(event));
}

void PhyduinoWire::begin(uint8_t address) {
    i2c_init(i2c_, clock_hz_);
    setupPins();
    i2c_slave_init(i2c_, address, peripheralHandler);
    peripheral_mode_ = true;
    started_ = true;
}

void PhyduinoWire::end() {
    if (!started_) return;
    if (peripheral_mode_) {  // the SDK asserts if deinit runs without init
        i2c_slave_deinit(i2c_);
        peripheral_mode_ = false;
    }
    i2c_deinit(i2c_);
    gpio_init(digitalPinToGpio(sda_));
    gpio_init(digitalPinToGpio(scl_));
    started_ = false;
}

void PhyduinoWire::setClock(uint32_t hz) {
    clock_hz_ = hz;  // used by begin(); applied now if already running
    if (started_) i2c_set_baudrate(i2c_, hz);
}

// --- Controller ------------------------------------------------------------------
void PhyduinoWire::beginTransmission(uint8_t address) {
    tx_address_ = address;
    tx_length_ = 0;
}

uint8_t PhyduinoWire::endTransmission(bool sendStop) {
    int result;
    if (tx_length_ == 0) {
        // The RP2040 cannot send an address with no data (what I2C scanners do),
        // so probe with a one-byte read: the device ACKs/NACKs its address the same way.
        uint8_t dummy;
        result = i2c_read_timeout_us(i2c_, tx_address_, &dummy, 1, !sendStop, kTimeoutUs);
    } else {
        result = i2c_write_timeout_us(i2c_, tx_address_, tx_buffer_, tx_length_, !sendStop, kTimeoutUs);
    }
    tx_length_ = 0;

    if (result >= 0)                  return kOk;
    if (result == PICO_ERROR_GENERIC) return kNack;
    if (result == PICO_ERROR_TIMEOUT) return kTimeout;
    return kOther;
}

size_t PhyduinoWire::requestFrom(uint8_t address, size_t length, bool sendStop) {
    rx_buffer_.clear();
    if (length > kBufferSize) length = kBufferSize;
    uint8_t data[kBufferSize];
    const int n = i2c_read_timeout_us(i2c_, address, data, length, !sendStop, kTimeoutUs);
    if (n <= 0) return 0;
    for (int i = 0; i < n; i++) rx_buffer_.store_char(data[i]);
    return static_cast<size_t>(n);
}

// --- Buffers (both modes) -------------------------------------------------------------
size_t PhyduinoWire::write(uint8_t data) {
    return write(&data, 1);
}

size_t PhyduinoWire::write(const uint8_t* data, size_t length) {
    size_t n = 0;
    while (n < length && tx_length_ < kBufferSize) {
        tx_buffer_[tx_length_++] = data[n++];
    }
    return n;  // fewer than `length` means the buffer is full
}

int PhyduinoWire::available() { return rx_buffer_.available(); }
int PhyduinoWire::read()      { return rx_buffer_.read_char(); }
int PhyduinoWire::peek()      { return rx_buffer_.peek(); }

// --- Peripheral (runs in the I2C interrupt) --------------------------------------------
void PhyduinoWire::handlePeripheralEvent(int event) {
    switch (static_cast<i2c_slave_event_t>(event)) {
        case I2C_SLAVE_RECEIVE:  // controller wrote byte(s)
            while (i2c_get_read_available(i2c_)) {
                const uint8_t c = i2c_read_byte_raw(i2c_);
                if (!rx_buffer_.isFull()) rx_buffer_.store_char(c);
            }
            break;

        case I2C_SLAVE_REQUEST:  // controller wants a byte
            if (!request_started_) {  // first byte of this read: let the sketch fill the buffer
                request_started_ = true;
                tx_length_ = 0;
                tx_sent_ = 0;
                if (on_request_) on_request_();
            }
            i2c_write_byte_raw(i2c_, tx_sent_ < tx_length_ ? tx_buffer_[tx_sent_++] : 0xFF);
            break;

        case I2C_SLAVE_FINISH:  // stop or repeated start
            if (rx_buffer_.available() && on_receive_) on_receive_(rx_buffer_.available());
            request_started_ = false;
            tx_length_ = 0;
            break;
    }
}
