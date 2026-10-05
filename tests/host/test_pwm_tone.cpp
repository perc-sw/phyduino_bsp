/**
 * @file test_pwm_tone.cpp
 * @brief analogWrite() and tone() PWM set-up (cores/phyduino/pwm.cpp, tone.cpp).
 *
 * At 125 MHz the slice divider is the smallest integer that keeps the 16-bit
 * counter period in range: div = ceil(f_sys / f / 65536), wrap = f_sys / (f * div) - 1.
 */
#include "test_support.h"
#include "wiring_private.h"

namespace {
const fake::PwmSlice& sliceOf(pin_size_t pin) {
    return fake::hw.pwm[(digitalPinToGpio(pin) >> 1) & 7];
}
uint16_t levelOf(pin_size_t pin) {
    return sliceOf(pin).level[digitalPinToGpio(pin) & 1];
}
}  // namespace

TEST_CASE_FIXTURE(HostTest, "analogWrite at the defaults: 8-bit, 1 kHz") {
    analogWriteResolution(8);
    analogWriteFrequency(1000);
    analogWrite(3, 128);                       // D3 = GPIO7, slice 3 channel B

    CHECK(gpioOf(3).function == GPIO_FUNC_PWM);
    CHECK(sliceOf(3).enabled);
    CHECK(sliceOf(3).div_int == 2);
    CHECK(sliceOf(3).wrap == 62499);           // 125 MHz / (1 kHz * 2) - 1
    CHECK(levelOf(3) == 128u * 62500u / 255u);
}

TEST_CASE_FIXTURE(HostTest, "analogWrite limits: 0 is off, max is fully on, out of range clamps") {
    analogWriteResolution(8);
    analogWriteFrequency(1000);
    analogWrite(5, 0);
    CHECK(levelOf(5) == 0);
    analogWrite(5, 255);
    CHECK(levelOf(5) == 62500);                // wrap + 1: high for the whole period
    analogWrite(5, 1000);
    CHECK(levelOf(5) == 62500);
    analogWrite(5, -5);
    CHECK(levelOf(5) == 0);
}

TEST_CASE_FIXTURE(HostTest, "analogWriteResolution and analogWriteFrequency") {
    analogWriteResolution(12);
    analogWriteFrequency(50);
    analogWrite(6, 4095);
    CHECK(sliceOf(6).div_int == 39);           // ceil(125e6 / 50 / 65536)
    CHECK(sliceOf(6).wrap == 64101);           // 125e6 / (50 * 39) - 1
    CHECK(levelOf(6) == 64102);

    analogWriteResolution(0);                  // invalid: ignored
    analogWrite(6, 4095);
    CHECK(levelOf(6) == 64102);

    analogWriteResolution(8);                  // back to defaults for other tests
    analogWriteFrequency(1000);
}

TEST_CASE_FIXTURE(HostTest, "very low frequencies clamp the divider to 255") {
    CHECK(pwmConfigureSlice(0, 1) == 65535);   // 125e6 / (1 * 255) - 1 would overflow; clamps
    CHECK(fake::hw.pwm[0].div_int == 255);
}

TEST_CASE_FIXTURE(HostTest, "analogWrite ignores pins without a GPIO") {
    analogWrite(A3, 100);
    for (const auto& s : fake::hw.pwm) CHECK_FALSE(s.enabled);
}

TEST_CASE_FIXTURE(HostTest, "tone plays a 50 % square wave and stops after the duration") {
    tone(8, 440, 100);                         // D8 = GPIO16, slice 0 channel A
    CHECK(gpioOf(8).function == GPIO_FUNC_PWM);
    CHECK(sliceOf(8).div_int == 5);            // ceil(125e6 / 440 / 65536)
    CHECK(sliceOf(8).wrap == 56817);           // 125e6 / (440 * 5) - 1
    CHECK(levelOf(8) == 56818 / 2);

    fake::advance_us(99 * 1000);
    CHECK(gpioOf(8).function == GPIO_FUNC_PWM);
    fake::advance_us(2 * 1000);
    CHECK(gpioOf(8).function == GPIO_FUNC_SIO);  // released back to a plain GPIO
    CHECK(levelOf(8) == 0);
}

TEST_CASE_FIXTURE(HostTest, "a new tone stops the previous one; noTone only stops its own pin") {
    tone(8, 1000);
    tone(9, 2000);
    CHECK(gpioOf(8).function == GPIO_FUNC_SIO);
    CHECK(gpioOf(9).function == GPIO_FUNC_PWM);

    noTone(8);                                 // not playing: no effect
    CHECK(gpioOf(9).function == GPIO_FUNC_PWM);
    noTone(9);
    CHECK(gpioOf(9).function == GPIO_FUNC_SIO);
    CHECK(fake::hw.alarms.empty());
}

TEST_CASE_FIXTURE(HostTest, "tone ignores frequency 0 and pins without a GPIO") {
    tone(8, 0);
    tone(A4, 440);
    for (const auto& s : fake::hw.pwm) CHECK_FALSE(s.enabled);
}
