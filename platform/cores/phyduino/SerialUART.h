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

/**
 * @brief Serial port on one RP2040 UART; sketches use Serial1 and Serial2.
 *
 * Received bytes go into a 256-byte buffer from an interrupt; bytes that
 * arrive while it is full are dropped.
 */
class SerialUARTClass : public arduino::HardwareSerial {
public:
    /**
     * @brief Binds the port to a UART and its pins; nothing is set up until begin().
     * @param uart pico-sdk UART (uart0 or uart1).
     * @param tx_pin Arduino pin for TX.
     * @param rx_pin Arduino pin for RX.
     */
    SerialUARTClass(uart_inst_t* uart, pin_size_t tx_pin, pin_size_t rx_pin)
        : uart_(uart), tx_pin_(tx_pin), rx_pin_(rx_pin) {}

    /**
     * @brief Starts the port with 8 data bits, no parity and 1 stop bit.
     * @param baud Baud rate, e.g. 115200.
     */
    void begin(unsigned long baud) override { begin(baud, SERIAL_8N1); }

    /**
     * @brief Starts the port with the given frame format.
     * @param baud Baud rate, e.g. 115200.
     * @param config Frame format, e.g. SERIAL_8N1 or SERIAL_7E2. Mark and
     *               space parity are not supported and give no parity.
     */
    void begin(unsigned long baud, uint16_t config) override;

    /**
     * @brief Waits for queued bytes to go out, then stops the port and frees its pins.
     *
     * Unread received bytes are discarded.
     */
    void end() override;

    /**
     * @brief Number of received bytes waiting to be read.
     * @return 0-256.
     */
    int available() override;

    /**
     * @brief Next received byte, left in the buffer.
     * @return 0-255, or -1 if nothing has been received.
     */
    int peek() override;

    /**
     * @brief Takes the next received byte from the buffer.
     * @return 0-255, or -1 if nothing has been received.
     */
    int read() override;

    /**
     * @brief Waits until every queued byte has been sent.
     */
    void flush() override;

    /**
     * @brief Sends one byte, waiting while the hardware FIFO is full.
     * @param c Byte to send.
     * @return 1, or 0 if the port has not been started.
     */
    size_t write(uint8_t c) override;
    using arduino::Print::write;

    /**
     * @brief Whether the port is running.
     * @return true between begin() and end().
     */
    operator bool() override { return started_; }

    /// @private Called from the UART interrupt; not for sketches.
    void handleRxInterrupt();

private:
    uart_inst_t* uart_;
    pin_size_t tx_pin_;
    pin_size_t rx_pin_;
    bool started_ = false;
    arduino::RingBufferN<256> rx_buffer_;
};

/// UART0 on D1 (TX) and D0 (RX).
extern SerialUARTClass Serial1;
/// UART1 on D4 (TX) and D5 (RX).
extern SerialUARTClass Serial2;
