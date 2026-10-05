/**
 * @file test_pins.cpp
 * @brief The Phyduino Pico pin map (pins_arduino.h) against the documented
 *        numbering and the RP2040 GPIO function table, so a typo in the pin
 *        map shows up here rather than as a dead peripheral on a board.
 */
#include "test_support.h"

#include <set>

namespace {

// RP2040 datasheet, GPIO function select table: which GPIOs can carry each signal.
const std::set<int> kUart0Tx = {0, 12, 16, 28}, kUart0Rx = {1, 13, 17, 29};
const std::set<int> kUart1Tx = {4, 8, 20, 24},  kUart1Rx = {5, 9, 21, 25};
const std::set<int> kSpi0Rx = {0, 4, 16, 20}, kSpi0Cs = {1, 5, 17, 21}, kSpi0Sck = {2, 6, 18, 22}, kSpi0Tx = {3, 7, 19, 23};
const std::set<int> kSpi1Rx = {8, 12, 24, 28}, kSpi1Cs = {9, 13, 25, 29}, kSpi1Sck = {10, 14, 26}, kSpi1Tx = {11, 15, 27};
const std::set<int> kI2c0Sda = {0, 4, 8, 12, 16, 20, 24, 28}, kI2c0Scl = {1, 5, 9, 13, 17, 21, 25, 29};
const std::set<int> kI2c1Sda = {2, 6, 10, 14, 18, 22, 26},    kI2c1Scl = {3, 7, 11, 15, 19, 23, 27};

int gpio(pin_size_t pin) { return digitalPinToGpio(pin); }

}  // namespace

TEST_CASE("pin numbers follow the Uno layout (docs/decisions/004)") {
    CHECK(A0 == 14); CHECK(A1 == 15); CHECK(A2 == 16);
    CHECK(A3 == 17); CHECK(A4 == 18); CHECK(A5 == 19);
    CHECK(SDA == 20); CHECK(SCL == 21);
    CHECK(LED_BUILTIN == 22);
    CHECK(PINS_COUNT == 23);
}

TEST_CASE("Arduino pin -> GPIO matches the table in docs/boards/phyduino_pico.md") {
    const int expected[PINS_COUNT] = {
        1, 0, 6, 7, 8, 9, 10, 15, 16, 17, 13, 11, 12, 14,  // D0-D13
        26, 27, 28,                                        // A0-A2
        NOT_A_GPIO, NOT_A_GPIO, NOT_A_GPIO,                // A3-A5 (MAX11635)
        24, 25,                                            // SDA, SCL
        5,                                                 // LED
    };
    for (pin_size_t pin = 0; pin < PINS_COUNT; pin++) {
        CAPTURE(pin);
        CHECK(gpio(pin) == expected[pin]);
    }
    CHECK(digitalPinToGpio(PINS_COUNT) == NOT_A_GPIO);
    CHECK(digitalPinToGpio(255) == NOT_A_GPIO);
}

TEST_CASE("no GPIO is used twice, and header pins avoid on-board chip pins") {
    const std::set<int> on_board = {
        PHYDUINO_EXT_ADC_GPIO_SCK, PHYDUINO_EXT_ADC_GPIO_MOSI, PHYDUINO_EXT_ADC_GPIO_MISO,
        PHYDUINO_EXT_ADC_GPIO_CS, PHYDUINO_EXT_ADC_GPIO_EOC, PHYDUINO_EXT_ADC_GPIO_CNVST,
        PHYDUINO_USBPD_GPIO_SDA, PHYDUINO_USBPD_GPIO_SCL, PHYDUINO_USBPD_GPIO_ALERT,
        PHYDUINO_USBPD_GPIO_RESET};
    CHECK(on_board.size() == 10);  // the chips' own pins are distinct too

    std::set<int> used;
    for (pin_size_t pin = 0; pin < PINS_COUNT; pin++) {
        const int g = gpio(pin);
        if (g == NOT_A_GPIO) continue;
        CAPTURE(pin);
        CHECK(used.insert(g).second);
        CHECK(on_board.count(g) == 0);
    }
}

TEST_CASE("Serial1/Serial2 pins are UART0/UART1 TX/RX pins") {
    CHECK(kUart0Tx.count(gpio(PIN_SERIAL1_TX)));
    CHECK(kUart0Rx.count(gpio(PIN_SERIAL1_RX)));
    CHECK(kUart1Tx.count(gpio(PIN_SERIAL2_TX)));
    CHECK(kUart1Rx.count(gpio(PIN_SERIAL2_RX)));
}

TEST_CASE("SPI pins belong to PHYDUINO_SPI_INSTANCE (SPI1)") {
    CHECK(PHYDUINO_SPI_INSTANCE == spi1);
    CHECK(kSpi1Tx.count(gpio(PIN_SPI_MOSI)));
    CHECK(kSpi1Rx.count(gpio(PIN_SPI_MISO)));
    CHECK(kSpi1Sck.count(gpio(PIN_SPI_SCK)));
    CHECK(kSpi1Cs.count(gpio(PIN_SPI_SS)));  // SS is driven as GPIO, but matches the hardware CSn
}

TEST_CASE("Wire pins belong to PHYDUINO_WIRE_INSTANCE (I2C0)") {
    CHECK(PHYDUINO_WIRE_INSTANCE == i2c0);
    CHECK(kI2c0Sda.count(gpio(PIN_WIRE_SDA)));
    CHECK(kI2c0Scl.count(gpio(PIN_WIRE_SCL)));
}

TEST_CASE("on-board MAX11635 is on SPI0 and STUSB4500 on I2C1") {
    CHECK(PHYDUINO_EXT_ADC_SPI == spi0);
    CHECK(kSpi0Sck.count(PHYDUINO_EXT_ADC_GPIO_SCK));
    CHECK(kSpi0Tx.count(PHYDUINO_EXT_ADC_GPIO_MOSI));
    CHECK(kSpi0Rx.count(PHYDUINO_EXT_ADC_GPIO_MISO));

    CHECK(PHYDUINO_USBPD_I2C == i2c1);
    CHECK(kI2c1Sda.count(PHYDUINO_USBPD_GPIO_SDA));
    CHECK(kI2c1Scl.count(PHYDUINO_USBPD_GPIO_SCL));
    CHECK(PHYDUINO_USBPD_I2C_ADDR == 0x28);  // ADDR0 = ADDR1 = GND
}

TEST_CASE("A0-A2 are on RP2040 ADC inputs") {
    for (pin_size_t pin = A0; pin <= A2; pin++) {
        CAPTURE(pin);
        CHECK(gpio(pin) >= 26);
        CHECK(gpio(pin) <= 29);
    }
}
