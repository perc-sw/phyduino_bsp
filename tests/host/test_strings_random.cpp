/**
 * @file test_strings_random.cpp
 * @brief itoa family, dtostrf, String formatting, random(), map()
 *        (string_conversions.cpp, random.cpp, ArduinoCore-API).
 */
#include "test_support.h"

#include <cstring>

TEST_CASE("itoa family: radix and sign handling (AVR convention)") {
    char buf[40];
    CHECK(std::strcmp(itoa(-42, buf, 10), "-42") == 0);
    CHECK(std::strcmp(itoa(-1, buf, 16), "ffffffff") == 0);  // minus sign only in base 10
    CHECK(std::strcmp(itoa(0, buf, 10), "0") == 0);
    CHECK(std::strcmp(utoa(255, buf, 2), "11111111") == 0);
    CHECK(std::strcmp(ltoa(-2147483647L - 1, buf, 10), "-2147483648") == 0);
    CHECK(std::strcmp(ultoa(4294967295UL, buf, 36), "1z141z3") == 0);
    CHECK(std::strcmp(itoa(10, buf, 1), "") == 0);           // invalid radix
}

TEST_CASE("dtostrf pads to the width and rounds to the precision") {
    char buf[20];
    CHECK(std::strcmp(dtostrf(3.14159, 6, 2, buf), "  3.14") == 0);
    CHECK(std::strcmp(dtostrf(-0.5, 1, 1, buf), "-0.5") == 0);
}

TEST_CASE("String uses the core's conversions") {
    CHECK(String(-12) == "-12");
    CHECK(String(255, HEX) == "ff");
    CHECK(String(3.14159, 3) == "3.142");
    CHECK(String("pulse=") + 7 == "pulse=7");
}

TEST_CASE("random stays in range and is repeatable with the same seed") {
    randomSeed(1234);
    long first[5];
    for (long& v : first) {
        v = random(10);
        CHECK(v >= 0);
        CHECK(v < 10);
    }
    randomSeed(1234);
    for (long v : first) CHECK(random(10) == v);

    for (int i = 0; i < 100; i++) {
        const long v = random(5, 8);
        CHECK(v >= 5);
        CHECK(v < 8);
    }
    CHECK(random(0) == 0);
    CHECK(random(-3) == 0);
    CHECK(random(5, 5) == 5);
}

TEST_CASE("map (from ArduinoCore-API) behaves as on other boards") {
    CHECK(map(512, 0, 1023, 0, 255) == 127);
    CHECK(map(0, 0, 1023, 255, 0) == 255);
}
