/**
 * @file test_variant_chips.cpp
 * @brief Phyduino Pico on-board chip drivers and start-up
 *        (variants/phyduino_pico/max11635.cpp, stusb4500.cpp, variant.cpp).
 *
 * The STUSB4500 register addresses come from ST's register map and are also
 * built into the model, so these tests check the driver's logic and encoding,
 * not the addresses themselves (that needs hardware: checklist item 14).
 */
#include "test_support.h"

#include "max11635.h"
#include "stusb4500.h"

// --- MAX11635 ------------------------------------------------------------------------------
TEST_CASE_FIXTURE(HostTest, "MAX11635 begin(): reset, then setup = ext. ref, unipolar single-ended") {
    Max11635Model adc;
    adc.attach();
    max11635::begin();

    CHECK(fake::hw.spi[0].enabled);
    CHECK(adc.resets == 1);
    REQUIRE(adc.commands.size() >= 3);
    CHECK(adc.commands[0] == 0x18);              // reset FIFO
    CHECK(adc.commands[1] == 0x10);              // reset all registers
    CHECK(adc.setup_register == 0x46);           // CKSEL 00, REFSEL 01, DIFFSEL 10
    CHECK(adc.unipolar_register == 0x00);        // every input single-ended
    CHECK(fake::hw.gpio[PHYDUINO_EXT_ADC_GPIO_CS].out_level);     // CS idles high
    CHECK(fake::hw.gpio[PHYDUINO_EXT_ADC_GPIO_CNVST].out_level);  // CNVST idles high
}

TEST_CASE_FIXTURE(HostTest, "MAX11635 read(): one conversion per call, 12-bit result") {
    Max11635Model adc;
    adc.attach();
    adc.channel_value[3] = 0xABC;
    CHECK(max11635::read(3) == 0xABC);
    CHECK(adc.commands.back() == 0x9E);          // 1 0011 11 0: channel 3, no scan
    CHECK(fake::hw.gpio[PHYDUINO_EXT_ADC_GPIO_CS].out_level);     // deselected afterwards
    CHECK(max11635::read(4) == 0);               // no such channel
}

// --- STUSB4500 -----------------------------------------------------------------------------
TEST_CASE_FIXTURE(HostTest, "STUSB4500 begin() finds the chip, or gives up after ~50 ms") {
    Stusb4500Model pd;
    pd.attach();
    CHECK(stusb4500::begin());

    pd.present = false;
    const uint64_t start = fake::hw.now_us;
    CHECK_FALSE(stusb4500::begin());
    CHECK(fake::hw.now_us - start >= 50'000);    // waited out TLOAD (30 ms) and more
    CHECK(fake::hw.now_us - start < 70'000);

    pd.present = true;
    pd.regs[0x2F] = 0x99;                        // something else answers at 0x28
    CHECK_FALSE(stusb4500::begin());
}

TEST_CASE_FIXTURE(HostTest, "STUSB4500 getProfiles decodes the PDO registers") {
    Stusb4500Model pd;
    pd.attach();
    REQUIRE(stusb4500::begin());
    stusb4500::Profile p[stusb4500::kMaxProfiles];
    REQUIRE(stusb4500::getProfiles(p) == 3);
    CHECK(p[0].millivolts == 5000);  CHECK(p[0].milliamps == 1500);
    CHECK(p[1].millivolts == 15000); CHECK(p[1].milliamps == 1500);
    CHECK(p[2].millivolts == 20000); CHECK(p[2].milliamps == 1000);
}

TEST_CASE_FIXTURE(HostTest, "STUSB4500 setProfiles encodes PDOs, keeps flag bits, renegotiates once") {
    Stusb4500Model pd;
    pd.attach();
    REQUIRE(stusb4500::begin());
    const stusb4500::Profile board[] = {{5000, 2000}, {9000, 1000}, {20000, 500}};
    CHECK(stusb4500::setProfiles(board, 3));

    // Fixed PDO: voltage in 50 mV units at bits 19:10, current in 10 mA units at 9:0.
    CHECK(pd.pdo(0) == ((0x26u << 24) | (100u << 10) | 200u));   // flags 31:20 untouched
    CHECK(pd.pdo(1) == ((180u << 10) | 100u));
    CHECK(pd.pdo(2) == ((400u << 10) | 50u));
    CHECK(pd.regs[0x70] == 3);
    CHECK(pd.soft_resets == 1);

    // Same profiles again: nothing written, no second renegotiation.
    const int writes = pd.register_writes;
    CHECK(stusb4500::setProfiles(board, 3));
    CHECK(pd.register_writes == writes);
    CHECK(pd.soft_resets == 1);

    // Fewer profiles: count register follows.
    CHECK(stusb4500::setProfiles(board, 2));
    CHECK(pd.regs[0x70] == 2);
    CHECK(pd.soft_resets == 2);
}

TEST_CASE_FIXTURE(HostTest, "STUSB4500 setProfiles rejects invalid profiles without writing") {
    Stusb4500Model pd;
    pd.attach();
    REQUIRE(stusb4500::begin());
    const int writes = pd.register_writes;
    const stusb4500::Profile ok[] = {{5000, 1000}, {9000, 1000}, {12000, 1000}, {15000, 1000}};
    const stusb4500::Profile not_5v_first[] = {{9000, 1000}};
    const stusb4500::Profile too_high[] = {{5000, 1000}, {21000, 1000}};
    const stusb4500::Profile too_much_current[] = {{5000, 5010}};

    CHECK_FALSE(stusb4500::setProfiles(ok, 0));
    CHECK_FALSE(stusb4500::setProfiles(ok, 4));
    CHECK_FALSE(stusb4500::setProfiles(not_5v_first, 1));
    CHECK_FALSE(stusb4500::setProfiles(too_high, 2));
    CHECK_FALSE(stusb4500::setProfiles(too_much_current, 1));
    CHECK(pd.register_writes == writes);
    CHECK(pd.soft_resets == 0);
}

TEST_CASE_FIXTURE(HostTest, "STUSB4500 contract and VBUS readings") {
    Stusb4500Model pd;
    pd.attach();
    REQUIRE(stusb4500::begin());
    CHECK(stusb4500::contractPosition() == 0);   // no PD contract
    CHECK(stusb4500::contractMilliamps() == 0);

    pd.setContract(3, 50);                       // offer #3, 500 mA
    CHECK(stusb4500::contractPosition() == 3);
    CHECK(stusb4500::contractMilliamps() == 500);

    Max11635Model adc;
    adc.attach();
    adc.channel_value[PHYDUINO_EXT_ADC_VBUS_CH] = 1896;  // 948 mV at the ADC pin
    // Divider 100k / 10.2k: VBUS = 948 mV * 110.2 / 10.2
    CHECK(stusb4500::vbusMillivolts() == 948u * 1102u / 102u);
}

TEST_CASE_FIXTURE(HostTest, "initVariant() loads the default power profile (5 V only)") {
    Max11635Model adc;
    adc.attach();
    Stusb4500Model pd;
    pd.attach();                                 // starts with ST's factory 5/15/20 V profiles
    initVariant();

    stusb4500::Profile p[stusb4500::kMaxProfiles];
    REQUIRE(stusb4500::getProfiles(p) == 1);
    CHECK(p[0].millivolts == 5000);  CHECK(p[0].milliamps == 2000);
    CHECK(pd.regs[0x70] == 1);
    CHECK(pd.soft_resets == 1);
    CHECK(adc.setup_register == 0x46);           // MAX11635 configured too
}

TEST_CASE("boardProfiles() gives each Tools > USB-PD Power option, within 10 W") {
    stusb4500::Profile p[stusb4500::kMaxProfiles];

    CHECK(stusb4500::boardProfiles(0, p) == 0);  // "use the chip's stored settings"

    REQUIRE(stusb4500::boardProfiles(5000, p) == 1);
    CHECK(p[0].millivolts == 5000);  CHECK(p[0].milliamps == 2000);

    REQUIRE(stusb4500::boardProfiles(9000, p) == 2);
    CHECK(p[1].millivolts == 9000);  CHECK(p[1].milliamps == 1000);

    const uint16_t top[][2] = {{12000, 830}, {15000, 660}, {20000, 500}};
    for (const auto& t : top) {
        CAPTURE(t[0]);
        REQUIRE(stusb4500::boardProfiles(t[0], p) == 3);
        CHECK(p[0].millivolts == 5000);
        CHECK(p[1].millivolts == 9000);
        CHECK(p[2].millivolts == t[0]);  CHECK(p[2].milliamps == t[1]);
        CHECK(uint32_t(p[2].millivolts) * p[2].milliamps <= 10000u * 1000u);
    }
}

TEST_CASE_FIXTURE(HostTest, "initVariant() carries on when the STUSB4500 is missing") {
    Max11635Model adc;
    adc.attach();
    Stusb4500Model pd;
    pd.attach();
    pd.present = false;
    initVariant();                               // must not hang or crash
    CHECK(pd.soft_resets == 0);
}
