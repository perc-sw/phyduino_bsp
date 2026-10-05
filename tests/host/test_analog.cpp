/**
 * @file test_analog.cpp
 * @brief analogRead(): internal ADC (A0-A2) and the transparent MAX11635 path
 *        (A3-A5), resolution scaling (cores/phyduino/wiring_analog.cpp,
 *        variants/phyduino_pico/variant.cpp).
 */
#include "test_support.h"

TEST_CASE_FIXTURE(HostTest, "A0-A2 read the RP2040 ADC input behind the pin") {
    analogReadResolution(12);
    fake::hw.adc_values[0] = 100;
    fake::hw.adc_values[1] = 2000;
    fake::hw.adc_values[2] = 4095;

    CHECK(analogRead(A0) == 100);
    CHECK(analogRead(A1) == 2000);
    CHECK(fake::hw.adc_selected == 1);
    CHECK(analogRead(A2) == 4095);
    CHECK(gpioOf(A2).function == GPIO_FUNC_NULL);   // pin handed to the ADC
}

TEST_CASE_FIXTURE(HostTest, "resolution rescales the 12-bit reading") {
    fake::hw.adc_values[0] = 4095;
    analogReadResolution(10);
    CHECK(analogRead(A0) == 1023);
    analogReadResolution(8);
    CHECK(analogRead(A0) == 255);
    analogReadResolution(13);           // out of range: ignored, stays 8
    CHECK(analogRead(A0) == 255);
    analogReadResolution(12);
    CHECK(analogRead(A0) == 4095);
    analogReadResolution(10);           // Arduino default, for other tests
}

TEST_CASE_FIXTURE(HostTest, "A3-A5 read MAX11635 channels 0-2") {
    Max11635Model adc;
    adc.attach();
    adc.channel_value[0] = 1234;
    adc.channel_value[1] = 42;
    adc.channel_value[2] = 4095;
    analogReadResolution(12);

    CHECK(analogRead(A3) == 1234);
    CHECK(analogRead(A4) == 42);
    CHECK(analogRead(A5) == 4095);
    analogReadResolution(10);
    CHECK(analogRead(A5) == 1023);

    // One single-channel, no-scan conversion command per read: 1 cccc 11 0.
    REQUIRE(adc.commands.size() >= 4);
    const auto n = adc.commands.size();
    CHECK(adc.commands[n - 4] == 0x86);   // A3 -> ch 0
    CHECK(adc.commands[n - 3] == 0x8E);   // A4 -> ch 1
    CHECK(adc.commands[n - 2] == 0x96);   // A5 -> ch 2
}

TEST_CASE_FIXTURE(HostTest, "A3 reads 0 when the MAX11635 does not answer") {
    Max11635Model adc;
    adc.attach();
    adc.respond = false;
    adc.channel_value[0] = 999;
    analogReadResolution(12);
    CHECK(analogRead(A3) == 0);
    analogReadResolution(10);
}

TEST_CASE_FIXTURE(HostTest, "digital-only pins read 0") {
    fake::hw.adc_values[0] = 4095;
    CHECK(analogRead(2) == 0);
    CHECK(analogRead(LED_BUILTIN) == 0);
    CHECK(analogRead(200) == 0);
}
