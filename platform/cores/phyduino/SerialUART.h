/**
 * @file SerialUART.h
 * @brief `Serial1`, `Serial2`: the RP2040 hardware UARTs on the header pins.
 *
 * Pins come from pins_arduino.h (PIN_SERIAL1_TX/RX, PIN_SERIAL2_TX/RX).
 * Received bytes are buffered by an interrupt; writes block until queued in
 * the 32-byte hardware FIFO.
 */
#pragma once

#include "api/HardwareSerial.h"
#include "api/RingBuffer.h"
#include "hardware/uart.h"

class SerialUARTClass : public arduino::HardwareSerial {
public:
    SerialUARTClass(uart_inst_t* uart, pin_size_t tx_pin, pin_size_t rx_pin)
        : uart_(uart), tx_pin_(tx_pin), rx_pin_(rx_pin) {}

    void begin(unsigned long baud) override { begin(baud, SERIAL_8N1); }
    void begin(unsigned long baud, uint16_t config) override;
    void end() override;

    int available() override;
    int peek() override;
    int read() override;
    void flush() override;
    size_t write(uint8_t c) override;
    using arduino::Print::write;

    operator bool() override { return started_; }

    void handleRxInterrupt();  // internal: called from the UART IRQ

private:
    uart_inst_t* uart_;
    pin_size_t tx_pin_;
    pin_size_t rx_pin_;
    bool started_ = false;
    arduino::RingBufferN<256> rx_buffer_;
};

extern SerialUARTClass Serial1;  // UART0
extern SerialUARTClass Serial2;  // UART1
