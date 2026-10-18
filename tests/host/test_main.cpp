/**
 * @file test_main.cpp
 * @brief doctest entry point, plus the few sketch-side symbols the core expects.
 */
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "fake_sdk.h"

// Normally defined by main.cpp (not compiled for PC tests: it needs TinyUSB).
// C linkage, as declared by ArduinoCore-API (api/Common.h). Time moves on while
// the sketch waits, so loops such as analogReadBurst() see background work finish.
extern "C" void yield() { fake::advance_us(1); }
