/**
 * @file test_digital.cpp
 * @brief pinMode(), digitalWrite(), digitalRead() (cores/phyduino/wiring_digital.cpp).
 */
#include "test_support.h"

TEST_CASE_FIXTURE(HostTest, "OUTPUT drives the pin") {
    pinMode(LED_BUILTIN, OUTPUT);
    const auto& led = gpioOf(LED_BUILTIN);
    CHECK(led.function == GPIO_FUNC_SIO);
    CHECK(led.output);

    digitalWrite(LED_BUILTIN, HIGH);
    CHECK(led.out_level);
    CHECK(digitalRead(LED_BUILTIN) == HIGH);

    digitalWrite(LED_BUILTIN, LOW);
    CHECK_FALSE(led.out_level);
    CHECK(digitalRead(LED_BUILTIN) == LOW);
}

TEST_CASE_FIXTURE(HostTest, "input modes set the pulls") {
    pinMode(2, INPUT_PULLUP);
    CHECK(gpioOf(2).pull_up);
    CHECK_FALSE(gpioOf(2).pull_down);
    CHECK_FALSE(gpioOf(2).output);

    pinMode(2, INPUT_PULLDOWN);
    CHECK_FALSE(gpioOf(2).pull_up);
    CHECK(gpioOf(2).pull_down);

    pinMode(2, INPUT);
    CHECK_FALSE(gpioOf(2).pull_up);
    CHECK_FALSE(gpioOf(2).pull_down);
}

TEST_CASE_FIXTURE(HostTest, "digitalRead reads the input level") {
    pinMode(7, INPUT);
    fake::hw.gpio[digitalPinToGpio(7)].input_level = true;
    CHECK(digitalRead(7) == HIGH);
    fake::hw.gpio[digitalPinToGpio(7)].input_level = false;
    CHECK(digitalRead(7) == LOW);
}

TEST_CASE_FIXTURE(HostTest, "writing HIGH to an INPUT enables the pull-up (Arduino convention)") {
    pinMode(3, INPUT);
    digitalWrite(3, HIGH);
    CHECK(gpioOf(3).pull_up);
    CHECK_FALSE(gpioOf(3).output);
    digitalWrite(3, LOW);
    CHECK_FALSE(gpioOf(3).pull_up);
}

TEST_CASE_FIXTURE(HostTest, "OUTPUT_OPENDRAIN drives low, releases high") {
    pinMode(4, OUTPUT_OPENDRAIN);
    const auto& pin = gpioOf(4);
    CHECK_FALSE(pin.output);            // released after pinMode
    CHECK_FALSE(pin.out_level);         // latch low, so "driven" always means low

    digitalWrite(4, LOW);
    CHECK(pin.output);
    CHECK_FALSE(pin.out_level);

    digitalWrite(4, HIGH);
    CHECK_FALSE(pin.output);
}

TEST_CASE_FIXTURE(HostTest, "pins without a GPIO are ignored safely") {
    pinMode(A3, OUTPUT);
    digitalWrite(A3, HIGH);
    CHECK(digitalRead(A3) == LOW);
    pinMode(200, OUTPUT);               // out of range
    digitalWrite(200, HIGH);
    CHECK(digitalRead(200) == LOW);
    for (const auto& g : fake::hw.gpio) CHECK_FALSE(g.initialised);
}
