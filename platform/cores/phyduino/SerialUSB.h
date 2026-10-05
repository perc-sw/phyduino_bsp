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

class SerialUSBClass : public arduino::HardwareSerial {
public:
    void begin(unsigned long baud) override { (void)baud; }  // baud is meaningless over USB
    void begin(unsigned long baud, uint16_t config) override { (void)baud; (void)config; }
    void end() override {}

    int available() override;
    int peek() override;
    int read() override;
    void flush() override;
    size_t write(uint8_t c) override { return write(&c, 1); }
    size_t write(const uint8_t* buffer, size_t size) override;
    using arduino::Print::write;

    /// True while a terminal has the port open (DTR set).
    operator bool() override;
};

extern SerialUSBClass Serial;

/// Starts TinyUSB and its 1 ms background task. Called once from init().
void serialUsbInitStack();
