/**
 * @file test_wire.cpp
 * @brief The Wire library (platform/libraries/Wire): controller and peripheral.
 */
#include "test_support.h"

#include <Wire.h>

#include <vector>

namespace {
struct Transfer {
    uint8_t address;
    std::vector<uint8_t> data;
    bool nostop;
};
std::vector<Transfer> writes, reads;

// A device at 0x50 that ACKs everything and reads back 0xA0, 0xA1, ...
void attachDevice(int result_override = 0) {
    writes.clear();
    reads.clear();
    fake::hw.i2c[0].on_write = [result_override](uint8_t a, const uint8_t* d, size_t n, bool nostop) -> int {
        writes.push_back({a, std::vector<uint8_t>(d, d + n), nostop});
        if (result_override) return result_override;
        return a == 0x50 ? static_cast<int>(n) : PICO_ERROR_GENERIC;
    };
    fake::hw.i2c[0].on_read = [](uint8_t a, uint8_t* d, size_t n, bool nostop) -> int {
        reads.push_back({a, {}, nostop});
        if (a != 0x50) return PICO_ERROR_GENERIC;
        for (size_t i = 0; i < n; i++) d[i] = static_cast<uint8_t>(0xA0 + i);
        return static_cast<int>(n);
    };
}

int received_count = -1;
int request_calls = 0;
std::vector<int> received;
void onReceive(int count) {
    received_count = count;
    while (Wire.available()) received.push_back(Wire.read());
}
void onRequest() {
    request_calls++;
    Wire.write(0x11);
    Wire.write(0x22);
}
}  // namespace

TEST_CASE_FIXTURE(HostTest, "begin() sets up I2C0 on SDA/SCL with pull-ups and 12 mA drive") {
    Wire.end();
    Wire.setClock(100000);
    Wire.begin();
    CHECK(fake::hw.i2c[0].enabled);
    CHECK(fake::hw.i2c[0].baud == 100000);
    for (pin_size_t pin : {SDA, SCL}) {
        CAPTURE(pin);
        CHECK(gpioOf(pin).function == GPIO_FUNC_I2C);
        CHECK(gpioOf(pin).pull_up);
        CHECK(gpioOf(pin).drive == GPIO_DRIVE_STRENGTH_12MA);
    }
    Wire.end();
    CHECK_FALSE(fake::hw.i2c[0].enabled);
}

TEST_CASE_FIXTURE(HostTest, "setClock before begin() is kept; after begin() it applies at once") {
    Wire.end();
    Wire.setClock(400000);
    CHECK_FALSE(fake::hw.i2c[0].enabled);       // not touched while stopped
    Wire.begin();
    CHECK(fake::hw.i2c[0].baud == 400000);
    Wire.setClock(100000);
    CHECK(fake::hw.i2c[0].baud == 100000);
    Wire.end();
}

TEST_CASE_FIXTURE(HostTest, "a write transaction sends the buffered bytes") {
    attachDevice();
    Wire.begin();
    Wire.beginTransmission(0x50);
    Wire.write(0x01);
    const uint8_t more[] = {0x02, 0x03};
    Wire.write(more, 2);
    CHECK(Wire.endTransmission() == 0);
    REQUIRE(writes.size() == 1);
    CHECK(writes[0].address == 0x50);
    CHECK(writes[0].data == std::vector<uint8_t>{1, 2, 3});
    CHECK_FALSE(writes[0].nostop);

    Wire.beginTransmission(0x50);
    Wire.write(0x09);
    Wire.endTransmission(false);                 // repeated start
    CHECK(writes.back().nostop);
    Wire.end();
}

TEST_CASE_FIXTURE(HostTest, "endTransmission result codes") {
    attachDevice();
    Wire.begin();
    Wire.beginTransmission(0x51);                // nobody there
    Wire.write(uint8_t{0});
    CHECK(Wire.endTransmission() == 2);          // NACK

    attachDevice(PICO_ERROR_TIMEOUT);
    Wire.beginTransmission(0x50);
    Wire.write(uint8_t{0});
    CHECK(Wire.endTransmission() == 5);          // timeout
    Wire.end();
}

TEST_CASE_FIXTURE(HostTest, "an empty transmission (I2C scanner) probes with a 1-byte read") {
    attachDevice();
    Wire.begin();
    Wire.beginTransmission(0x50);
    CHECK(Wire.endTransmission() == 0);
    Wire.beginTransmission(0x51);
    CHECK(Wire.endTransmission() == 2);
    CHECK(writes.empty());                       // the RP2040 cannot send a 0-byte write
    CHECK(reads.size() == 2);
    Wire.end();
}

TEST_CASE_FIXTURE(HostTest, "requestFrom fills the receive buffer") {
    attachDevice();
    Wire.begin();
    CHECK(Wire.requestFrom(0x50, 3) == 3);
    CHECK(Wire.available() == 3);
    CHECK(Wire.peek() == 0xA0);
    CHECK(Wire.read() == 0xA0);
    CHECK(Wire.read() == 0xA1);
    CHECK(Wire.read() == 0xA2);
    CHECK(Wire.read() == -1);
    CHECK(Wire.requestFrom(0x51, 3) == 0);       // NACK
    CHECK(Wire.available() == 0);
    Wire.end();
}

TEST_CASE_FIXTURE(HostTest, "peripheral mode: onReceive gets what the controller wrote") {
    Wire.end();
    received.clear();
    received_count = -1;
    Wire.onReceive(onReceive);
    Wire.begin(0x42);
    auto& bus = fake::hw.i2c[0];
    REQUIRE(bus.slave_handler != nullptr);
    CHECK(bus.slave_address == 0x42);

    bus.slave_rx = {7, 8, 9};
    bus.slave_handler(i2c0, I2C_SLAVE_RECEIVE);
    bus.slave_handler(i2c0, I2C_SLAVE_FINISH);
    CHECK(received_count == 3);
    CHECK(received == std::vector<int>{7, 8, 9});
    Wire.end();
    CHECK(bus.slave_handler == nullptr);
}

TEST_CASE_FIXTURE(HostTest, "peripheral mode: onRequest fills the reply once per read") {
    Wire.end();
    request_calls = 0;
    Wire.onRequest(onRequest);
    Wire.begin(0x42);
    auto& bus = fake::hw.i2c[0];

    for (int i = 0; i < 3; i++) bus.slave_handler(i2c0, I2C_SLAVE_REQUEST);
    bus.slave_handler(i2c0, I2C_SLAVE_FINISH);
    CHECK(request_calls == 1);
    CHECK(bus.slave_tx == std::vector<uint8_t>{0x11, 0x22, 0xFF});  // 0xFF once the reply runs out

    bus.slave_tx.clear();
    bus.slave_handler(i2c0, I2C_SLAVE_REQUEST);                    // next read transaction
    CHECK(request_calls == 2);
    CHECK(bus.slave_tx == std::vector<uint8_t>{0x11});
    Wire.end();
}
