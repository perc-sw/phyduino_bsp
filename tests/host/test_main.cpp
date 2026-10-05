/**
 * @file test_main.cpp
 * @brief doctest entry point, plus the few sketch-side symbols the core expects.
 */
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

// Normally defined by main.cpp (not compiled for PC tests: it needs TinyUSB).
extern "C" void yield() {}  // C linkage, as declared by ArduinoCore-API (api/Common.h)
