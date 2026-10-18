/**
 * @file test_analog_extras.cpp
 * @brief Averaging, millivolts, differential, multi-pin reads and bursts on
 *        both ADCs (cores/phyduino/wiring_analog.cpp, analog_burst.cpp,
 *        variants/phyduino_pico/variant.cpp, max11635.cpp).
 */
#include "test_support.h"

#include "max11635.h"

namespace {

// Each test starts at 12 bits, no averaging; puts the Arduino defaults back after.
struct AnalogTest : HostTest {
    Max11635Model adc;
    AnalogTest() {
        adc.attach();
        analogReadResolution(12);
        analogReadAveraging(1);
    }
    ~AnalogTest() {
        analogStopBurst();
        analogReadAveraging(1);
        analogReadResolution(10);
    }
};

}  // namespace

// --- Averaging ----------------------------------------------------------------------------
TEST_CASE_FIXTURE(AnalogTest, "averaging: RP2040 pins average N conversions") {
    uint16_t next = 100;
    fake::hw.adc_sample = [&](uint) { return next++; };   // 100, 101, 102, ...
    analogReadAveraging(4);
    const uint32_t before = fake::hw.adc_conversions;
    CHECK(analogRead(A0) == 102);                          // (100+101+102+103)/4 = 101.5, rounded
    CHECK(fake::hw.adc_conversions - before == 4);
}

TEST_CASE_FIXTURE(AnalogTest, "averaging: rounded down to 1/4/8/16/32 and sent to the MAX11635") {
    analogReadAveraging(10);
    CHECK(max11635::averaging() == 8);
    CHECK(adc.averaging_register == 0x34);                 // 001 1 01 00: AVGON, NAVG = 8
    analogReadAveraging(100);
    CHECK(max11635::averaging() == 32);
    CHECK(adc.averaging_register == 0x3C);
    analogReadAveraging(0);
    CHECK(max11635::averaging() == 1);
    CHECK(adc.averaging_register == 0x20);                 // averaging off
}

// --- Millivolts -----------------------------------------------------------------------------
TEST_CASE_FIXTURE(AnalogTest, "full scale and millivolts follow each pin's ADC") {
    CHECK(analogFullScaleMillivolts(A0) == 3300);
    CHECK(analogFullScaleMillivolts(A5) == 2048);
    CHECK(analogFullScaleMillivolts(2) == 0);

    fake::hw.adc_values[0] = 2048;
    adc.channel_value[0] = 1024;
    CHECK(analogReadMillivolts(A0) == 1650);
    CHECK(analogReadMillivolts(A3) == 512);
    CHECK(analogReadMillivolts(2) == 0);

    analogReadResolution(8);                               // millivolts always use 12 bits
    CHECK(analogReadMillivolts(A3) == 512);
}

// --- Differential -----------------------------------------------------------------------------
TEST_CASE_FIXTURE(AnalogTest, "differential A3-A4 is a bipolar hardware conversion, then single-ended again") {
    adc.channel_value[0] = 1500;
    adc.channel_value[1] = 1000;
    CHECK(analogReadDifferentialMillivolts(A3, A4) == 250);   // 500 codes x 0.5 mV
    CHECK(analogReadDifferentialMillivolts(A4, A3) == -250);
    CHECK(adc.bipolar_register == 0);                      // pair back to single-ended
    CHECK(adc.unipolar_register == 0);
    bool bipolar_set = false;
    for (uint8_t c : adc.commands) bipolar_set = bipolar_set || c == 0x47;
    CHECK(bipolar_set);                                    // setup with DIFFSEL = 11
}

TEST_CASE_FIXTURE(AnalogTest, "differential beyond ±1.024 V falls back to two readings") {
    adc.channel_value[0] = 4000;
    adc.channel_value[1] = 0;
    CHECK(analogReadDifferentialMillivolts(A3, A4) == 2000);
}

TEST_CASE_FIXTURE(AnalogTest, "differential on RP2040 pins and mixed pins is computed") {
    fake::hw.adc_values[0] = 2048;                         // 1650 mV
    fake::hw.adc_values[1] = 1024;                         //  825 mV
    CHECK(analogReadDifferentialMillivolts(A0, A1) == 825);
    adc.channel_value[2] = 2048;                           // A5: 1024 mV
    CHECK(analogReadDifferentialMillivolts(A0, A5) == 626);
    CHECK(analogReadDifferentialMillivolts(A0, 2) == 0);   // not analog
}

// --- Several pins ---------------------------------------------------------------------------------
TEST_CASE_FIXTURE(AnalogTest, "analogReadMultiple keeps the order and scans the MAX11635 once") {
    fake::hw.adc_values[0] = 111;
    adc.channel_value[0] = 300;
    adc.channel_value[2] = 500;
    const pin_size_t pins[] = {A5, A0, A3, A3};
    uint16_t results[4] = {};
    const int pulses = adc.cnvst_pulses;
    CHECK(analogReadMultiple(pins, 4, results));
    CHECK(results[0] == 500);
    CHECK(results[1] == 111);
    CHECK(results[2] == 300);
    CHECK(results[3] == 300);
    CHECK(adc.cnvst_pulses - pulses == 1);                 // one scan for all three
    CHECK(adc.commands.back() == 0x90);                    // 1 010 00 0: channels 0..2

    const pin_size_t bad[] = {A0, 2};
    CHECK_FALSE(analogReadMultiple(bad, 2, results));
    CHECK(results[1] == 0);
}

// --- Bursts: RP2040 ADC ---------------------------------------------------------------------------
TEST_CASE_FIXTURE(AnalogTest, "RP2040 burst: ADC divider and DMA") {
    uint16_t next = 0;
    fake::hw.adc_sample = [&](uint) { return next++; };
    uint16_t buffer[8] = {};
    CHECK(analogReadBurst(A1, buffer, 8, 100000) == 100000);
    CHECK(fake::hw.adc_clkdiv_at_run == doctest::Approx(479.0));  // 480 ADC clocks of 48 MHz
    CHECK(fake::hw.adc_selected == 1);
    for (uint16_t i = 0; i < 8; i++) CHECK(buffer[i] == i);
    CHECK_FALSE(fake::hw.adc_running);                     // stopped afterwards
    CHECK_FALSE(fake::hw.adc_fifo_enabled);

    CHECK(analogReadBurst(A1, buffer, 8, 500000) == 500000);
    CHECK(fake::hw.adc_clkdiv_at_run == doctest::Approx(95.0));   // fastest: 96 clocks
    CHECK(analogReadBurst(A1, buffer, 8, 500001) == 0);
}

TEST_CASE_FIXTURE(AnalogTest, "RP2040 burst below 733 Hz uses a timer, evenly spaced") {
    std::vector<uint64_t> times;
    fake::hw.adc_sample = [&](uint) { times.push_back(fake::hw.now_us); return uint16_t{7}; };
    uint16_t buffer[5] = {};
    CHECK(analogReadBurst(A2, buffer, 5, 100) == 100);
    REQUIRE(times.size() == 5);
    for (size_t i = 1; i < times.size(); i++) CHECK(times[i] - times[i - 1] == 10000);
    CHECK(buffer[4] == 7);
}

TEST_CASE_FIXTURE(AnalogTest, "burst results use analogReadResolution()") {
    fake::hw.adc_values[0] = 4095;
    analogReadResolution(10);
    uint16_t buffer[3] = {};
    CHECK(analogReadBurst(A0, buffer, 3, 10000) > 0);
    CHECK(buffer[0] == 1023);
}

TEST_CASE_FIXTURE(AnalogTest, "background burst: done later; one burst at a time; reads wait") {
    fake::hw.dma_busy_polls = 3;
    fake::hw.adc_values[0] = 42;
    uint16_t buffer[4] = {};
    CHECK(analogStartBurst(A0, buffer, 4, 50000) == 50000);
    CHECK_FALSE(analogBurstDone());
    CHECK(analogStartBurst(A3, buffer, 4, 50000) == 0);    // already running
    CHECK(analogRead(A0) == 42);                           // waited for the burst to end
    CHECK(analogBurstDone());
    CHECK(buffer[3] == 42);
}

TEST_CASE_FIXTURE(AnalogTest, "invalid bursts are refused") {
    uint16_t buffer[2];
    CHECK(analogReadBurst(2, buffer, 2, 1000) == 0);       // not analog
    CHECK(analogReadBurst(A0, nullptr, 2, 1000) == 0);
    CHECK(analogReadBurst(A0, buffer, 0, 1000) == 0);
    CHECK(analogReadBurst(A0, buffer, 2, 0) == 0);
    CHECK(analogBurstDone());
}

// --- Bursts: MAX11635 -----------------------------------------------------------------------------
TEST_CASE_FIXTURE(AnalogTest, "MAX11635 burst up to 20 kHz: timer, averaging off, then restored") {
    analogReadAveraging(8);
    uint16_t next = 1000;
    adc.sample = [&](uint8_t) { return next++; };
    uint16_t buffer[4] = {};
    CHECK(analogReadBurst(A4, buffer, 4, 1000) == 1000);
    for (uint16_t i = 0; i < 4; i++) CHECK(buffer[i] == 1000 + i);
    CHECK(adc.averaging_register == 0x34);                 // averaging 8 again afterwards
    bool averaging_off = false;
    for (uint8_t c : adc.commands) averaging_off = averaging_off || c == 0x20;
    CHECK(averaging_off);
}

TEST_CASE_FIXTURE(AnalogTest, "MAX11635 burst above 20 kHz: clock mode 11, SCLK = 16 x rate, results realigned") {
    uint16_t next = 1;
    adc.sample = [&](uint8_t channel) { return static_cast<uint16_t>(channel * 1000 + next++); };
    uint16_t buffer[6] = {};
    CHECK(analogReadBurst(A5, buffer, 6, 100000) == 100000);
    for (uint16_t i = 0; i < 6; i++) CHECK(buffer[i] == 2000 + 1 + i);
    CHECK(adc.conversions == 6);                           // no extra conversion at the end
    CHECK(fake::hw.spi[0].baud == 4000000);                // back to 4 MHz, 8-bit frames
    CHECK(fake::hw.spi[0].data_bits == 8);
    CHECK(adc.setup_register == 0x46);                     // back to clock mode 00
    CHECK(fake::hw.gpio[PHYDUINO_EXT_ADC_GPIO_CS].out_level);
    bool mode_11 = false;
    for (uint8_t c : adc.commands) mode_11 = mode_11 || c == 0x74;
    CHECK(mode_11);

    CHECK(analogReadBurst(A5, buffer, 6, 300001) == 0);    // above the chip's limit
}

TEST_CASE_FIXTURE(AnalogTest, "stopping a MAX11635 burst puts the chip back for single reads") {
    fake::hw.dma_busy_polls = 100;
    uint16_t buffer[16] = {};
    CHECK(analogStartBurst(A3, buffer, 16, 200000) > 0);
    CHECK_FALSE(analogBurstDone());
    analogStopBurst();
    CHECK(analogBurstDone());
    CHECK(adc.setup_register == 0x46);
    adc.channel_value[0] = 321;
    CHECK(analogRead(A3) == 321);
}
