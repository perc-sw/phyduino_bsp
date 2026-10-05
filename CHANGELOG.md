# Changelog

Format: one section per version, newest first. Stage plan: README.md.

## Unreleased: v0.1.0 (Stage 1: Phyduino Pico board package)

### Added
- Own Arduino core on pico-sdk 2.2.0 + ArduinoCore-API 1.5.2.
- Phyduino Pico (RP2040) variant: Uno-style pin map, `LED_BUILTIN` on GPIO5.
- Digital I/O, `analogRead` (A0–A2 RP2040 ADC, A3–A5 on-board MAX11635),
  `analogWrite` (PWM), `tone`, interrupts, `pulseIn`, `shiftIn/Out`, `random`.
- USB `Serial` (TinyUSB CDC) with 1200-baud upload touch; `Serial1`/`Serial2` UARTs.
- SPI and Wire (controller + peripheral) libraries.
- STUSB4500 USB-PD driver (`stusb4500.h`); default profiles 5 V/2 A, 9 V/1 A,
  20 V/0.5 A loaded at start-up (working registers only, NVM untouched).
- `sdk-builder/` generating the committed SDK bundle.
- Release tooling (`package/`) and CI (`.github/`).
- CMake verification build (`CMakeLists.txt`, `cmake/`): same flags as the
  Arduino build, read from platform.txt/boards.txt, with `-Werror`. Not shipped.
- PC-side unit tests (`tests/host/`, doctest): core, variant drivers and
  libraries against a fake pico-sdk with MAX11635/STUSB4500 models; 71 cases.
- GPL-3.0 licence (`LICENSE`, also in the release zip); bundled upstream
  licences in `platform/licenses/`.
- `Arduino.h` declares `itoa()`/`utoa()`/`ltoa()`/`ultoa()` and `dtostrf()`.

### Fixed (found by the PC-side tests)
- PWM/`tone()` below ~7.5 Hz: counter top overflowed 16 bits and gave a wrong
  frequency; now clamps to the lowest achievable frequency.
- `attachInterrupt(pin, nullptr, mode)` enabled the interrupt with a null
  handler (crash on first edge); now ignored.
- `itoa()` of a negative value in base 2/8/16 printed a 64-bit pattern where
  `long` is 64-bit; now always the width of `int`.

### Open items
- **USB VID/PID**: uses Raspberry Pi VID 0x2E8A with the pico-sdk generic PID
  0x000A. Request a dedicated PID from Raspberry Pi before wide release.
- STUSB4500 register addresses (from ST's register map) still to be confirmed on hardware.
- Hardware checklist not yet run on a real board.
