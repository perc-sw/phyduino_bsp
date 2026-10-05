/**
 * @file test_support.h
 * @brief Included by every test file: doctest, the fake SDK, the Arduino API.
 *
 * Use TEST_CASE_FIXTURE(HostTest, "...") so each test starts from power-on
 * hardware state. Note: static state inside the code under test (e.g. the
 * analogRead resolution, Wire/Serial1 "started" flags) is NOT reset, just as
 * it would persist on the board; tests set what they rely on.
 */
#pragma once

#include "doctest.h"
#include "fake_sdk.h"
#include "chip_models.h"

#include "Arduino.h"

struct HostTest {
    HostTest() { fake::reset(); }
};

inline const fake::Gpio& gpioOf(pin_size_t arduino_pin) {
    return fake::hw.gpio[digitalPinToGpio(arduino_pin)];
}
