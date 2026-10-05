/**
 * @file test_time_pulse_shift.cpp
 * @brief millis/micros/delay, pulseIn, shiftOut/shiftIn
 *        (wiring_time.cpp, wiring_pulse.cpp, wiring_shift.cpp).
 *
 * Fake time moves 1 us on every timer read, so busy-waits always finish.
 */
#include "test_support.h"

#include <vector>

TEST_CASE_FIXTURE(HostTest, "millis and micros follow the microsecond timer") {
    fake::hw.now_us = 5'000'000;
    CHECK(millis() == 5000);
    const unsigned long t0 = micros();
    fake::advance_us(1234);
    CHECK(micros() - t0 >= 1234);
    CHECK(micros() - t0 < 1240);
}

TEST_CASE_FIXTURE(HostTest, "delay and delayMicroseconds wait at least as long as asked") {
    const uint64_t start = fake::hw.now_us;
    delay(10);
    CHECK(fake::hw.now_us - start >= 10'000);

    const uint64_t mid = fake::hw.now_us;
    delayMicroseconds(250);
    CHECK(fake::hw.now_us - mid == 250);
}

TEST_CASE_FIXTURE(HostTest, "pulseIn measures a HIGH pulse") {
    pinMode(3, INPUT);
    // HIGH from 1000 us to 1500 us.
    fake::hw.gpio[digitalPinToGpio(3)].waveform = [](uint64_t t) { return t >= 1000 && t < 1500; };
    const unsigned long width = pulseIn(3, HIGH, 100000);
    CHECK(width >= 498);
    CHECK(width <= 502);
}

TEST_CASE_FIXTURE(HostTest, "pulseIn waits out a pulse already in progress") {
    pinMode(3, INPUT);
    // Already HIGH at t=0 until 200 us, then a 300 us pulse from 1000 us.
    fake::hw.gpio[digitalPinToGpio(3)].waveform = [](uint64_t t) {
        return t < 200 || (t >= 1000 && t < 1300);
    };
    const unsigned long width = pulseIn(3, HIGH, 100000);
    CHECK(width >= 298);
    CHECK(width <= 302);
}

TEST_CASE_FIXTURE(HostTest, "pulseIn returns 0 on timeout or for a pin without a GPIO") {
    pinMode(3, INPUT);
    CHECK(pulseIn(3, HIGH, 1000) == 0);         // line never goes HIGH
    CHECK(pulseIn(A3, HIGH, 1000) == 0);
}

TEST_CASE_FIXTURE(HostTest, "shiftOut sends 8 bits, one per rising clock edge") {
    pinMode(4, OUTPUT);   // data
    pinMode(5, OUTPUT);   // clock
    const uint data_gpio = digitalPinToGpio(4), clock_gpio = digitalPinToGpio(5);
    std::vector<int> bits;
    fake::hw.on_gpio_put = [&](uint gpio, bool value) {
        if (gpio == clock_gpio && value) bits.push_back(fake::hw.gpio[data_gpio].out_level);
    };

    shiftOut(4, 5, MSBFIRST, 0xA5);
    CHECK(bits == std::vector<int>{1, 0, 1, 0, 0, 1, 0, 1});

    bits.clear();
    shiftOut(4, 5, LSBFIRST, 0xA5);
    CHECK(bits == std::vector<int>{1, 0, 1, 0, 0, 1, 0, 1});  // 0xA5 is a bit palindrome...

    bits.clear();
    shiftOut(4, 5, LSBFIRST, 0x01);
    CHECK(bits == std::vector<int>{1, 0, 0, 0, 0, 0, 0, 0});  // ...0x01 is not
}

TEST_CASE_FIXTURE(HostTest, "shiftIn samples the data pin after each rising clock edge") {
    pinMode(3, INPUT);    // data
    pinMode(5, OUTPUT);   // clock
    const uint data_gpio = digitalPinToGpio(3), clock_gpio = digitalPinToGpio(5);
    const int pattern[8] = {1, 1, 0, 0, 1, 0, 1, 0};   // 0xCA MSB first
    int edge = 0;
    fake::hw.on_gpio_put = [&](uint gpio, bool value) {
        if (gpio == clock_gpio && value && edge < 8) fake::hw.gpio[data_gpio].input_level = pattern[edge++];
    };

    CHECK(shiftIn(3, 5, MSBFIRST) == 0xCA);
    edge = 0;
    CHECK(shiftIn(3, 5, LSBFIRST) == 0x53);           // same wire bits, read LSB first
}
