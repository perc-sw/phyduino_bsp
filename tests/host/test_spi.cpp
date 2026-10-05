/**
 * @file test_spi.cpp
 * @brief The SPI library (platform/libraries/SPI).
 */
#include "test_support.h"

#include <SPI.h>

TEST_CASE_FIXTURE(HostTest, "begin() sets up SPI1 on D11/D12/D13") {
    SPI.begin();
    CHECK(fake::hw.spi[1].enabled);
    CHECK(gpioOf(PIN_SPI_MOSI).function == GPIO_FUNC_SPI);
    CHECK(gpioOf(PIN_SPI_MISO).function == GPIO_FUNC_SPI);
    CHECK(gpioOf(PIN_SPI_SCK).function == GPIO_FUNC_SPI);
    CHECK(gpioOf(PIN_SPI_SS).function != GPIO_FUNC_SPI);  // SS stays a sketch-driven GPIO
    SPI.end();
}

TEST_CASE_FIXTURE(HostTest, "beginTransaction applies clock and SPI mode") {
    SPI.begin();
    SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0));
    CHECK(fake::hw.spi[1].baud == 1000000);
    CHECK(fake::hw.spi[1].cpol == SPI_CPOL_0);
    CHECK(fake::hw.spi[1].cpha == SPI_CPHA_0);

    SPI.beginTransaction(SPISettings(500000, MSBFIRST, SPI_MODE1));
    CHECK(fake::hw.spi[1].cpol == SPI_CPOL_0);
    CHECK(fake::hw.spi[1].cpha == SPI_CPHA_1);
    SPI.beginTransaction(SPISettings(500000, MSBFIRST, SPI_MODE2));
    CHECK(fake::hw.spi[1].cpol == SPI_CPOL_1);
    CHECK(fake::hw.spi[1].cpha == SPI_CPHA_0);
    SPI.beginTransaction(SPISettings(500000, MSBFIRST, SPI_MODE3));
    CHECK(fake::hw.spi[1].cpol == SPI_CPOL_1);
    CHECK(fake::hw.spi[1].cpha == SPI_CPHA_1);
    SPI.endTransaction();
    SPI.end();
}

TEST_CASE_FIXTURE(HostTest, "transfer() exchanges bytes (MOSI looped to MISO)") {
    fake::hw.spi[1].device = [](uint8_t out) { return out; };
    SPI.begin();
    SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0));
    CHECK(SPI.transfer(0x5A) == 0x5A);
    CHECK(SPI.transfer16(0x1234) == 0x1234);
    CHECK(fake::hw.spi[1].sent == std::vector<uint8_t>{0x5A, 0x12, 0x34});  // MSB byte first

    uint8_t buffer[3] = {1, 2, 3};
    SPI.transfer(buffer, 3);
    CHECK(buffer[0] == 1);
    CHECK(buffer[2] == 3);
    SPI.end();
}

TEST_CASE_FIXTURE(HostTest, "LSBFIRST reverses each byte on the wire") {
    fake::hw.spi[1].device = [](uint8_t) { return 0x01; };   // device always sends 0000 0001
    SPI.begin();
    SPI.beginTransaction(SPISettings(1000000, LSBFIRST, SPI_MODE0));
    CHECK(SPI.transfer(0x01) == 0x80);                        // received bits, read LSB first
    CHECK(fake::hw.spi[1].sent.back() == 0x80);               // 0x01 sent LSB first

    fake::hw.spi[1].sent.clear();
    SPI.transfer16(0x1234);
    // LSB-first 16-bit: low byte first, each byte bit-reversed.
    CHECK(fake::hw.spi[1].sent == std::vector<uint8_t>{0x2C, 0x48});
    SPI.end();
}
