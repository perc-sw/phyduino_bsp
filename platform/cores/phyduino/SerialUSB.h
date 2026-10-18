/**
 * @file SerialUSB.h
 * @brief `Serial`: USB CDC serial port (TinyUSB).
 *
 * The USB stack starts in init() (main.cpp), before setup(), and is serviced in
 * the background every 1 ms, so the board always enumerates and accepts the
 * IDE's 1200-baud "touch" (reboot to BOOTSEL for upload).
 */
#pragma once

#include "api/HardwareSerial.h"

/**
 * @brief Serial port over the board's USB connection; sketches use Serial.
 *
 * The port is always running, so begin() and end() do nothing. Data written
 * while no terminal has the port open is dropped.
 */
class SerialUSBClass : public arduino::HardwareSerial {
public:
    /**
     * @brief Does nothing: the port starts before setup().
     * @param baud Ignored; the baud rate means nothing over USB.
     */
    void begin(unsigned long baud) override { (void)baud; }

    /**
     * @brief Does nothing: the port starts before setup().
     * @param baud Ignored.
     * @param config Ignored.
     */
    void begin(unsigned long baud, uint16_t config) override { (void)baud; (void)config; }

    /**
     * @brief Does nothing: the port stays up so the IDE can still upload.
     */
    void end() override {}

    /**
     * @brief Number of received bytes waiting to be read.
     * @return Byte count.
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
     * @brief Sends any bytes still held in the USB buffer.
     */
    void flush() override;

    /**
     * @brief Sends one byte.
     * @param c Byte to send.
     * @return 1, or 0 if it was dropped (see write(const uint8_t*, size_t)).
     */
    size_t write(uint8_t c) override { return write(&c, 1); }

    /**
     * @brief Sends a block of bytes.
     *
     * Waits while the USB buffer is full. Stops early if no terminal has the
     * port open, or if the computer reads nothing for 0.5 s.
     * @param buffer Bytes to send.
     * @param size Number of bytes.
     * @return Number of bytes sent.
     */
    size_t write(const uint8_t* buffer, size_t size) override;
    using arduino::Print::write;

    /**
     * @brief Whether a terminal has the port open.
     *
     * Use `while (!Serial) {}` in setup() to wait for the Serial Monitor.
     * @return true while a terminal has the port open (DTR set).
     */
    operator bool() override;
};

/// USB serial port, the one the Serial Monitor uses.
extern SerialUSBClass Serial;

/// Starts TinyUSB and its 1 ms background task. Called once from init().
void serialUsbInitStack();
