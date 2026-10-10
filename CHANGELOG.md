# Changelog

Format: one section per version, newest first. Stage plan: README.md.

## Unreleased: v0.1.0 (Stage 1: Phyduino Pico board package)

### Added
- Own Arduino core on pico-sdk 2.2.0 + ArduinoCore-API 1.5.2.
- Phyduino Pico (RP2040) variant: Uno-style pin map, `LED_BUILTIN` on GPIO5.
- Digital I/O, `analogRead` (A0–A2 RP2040 ADC, A3–A5 on-board MAX11635),
  `analogWrite` (PWM), `tone`, interrupts, `pulseIn`, `shiftIn/Out`, `random`.
- USB `Serial` (TinyUSB CDC) with 1200-baud upload touch; `Serial1`/`Serial2` UARTs.
- Upload by copying the UF2 to the BOOTSEL drive, no USB driver needed; picotool as
  fallback (`platform/tools/uf2_upload.*`, ADR 007).
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
- Tools > C++ Standard menu: C++14, 17 (default), 20, 23 (`gnu++NN`). CI
  compiles every sketch at each standard (and at the CPU Speed / Optimize
  extremes); the CMake build stays on C++17.
- Tools > CPU Speed menu: 50, 75, 100, 125, 133 (default), 150, 176, 200 MHz.
  Set in `init()`; above 133 MHz the core voltage goes to 1.15 V first. 175 MHz
  is not possible from the 12 MHz crystal, so 176 MHz. Peripheral clock follows
  the CPU up to 133 MHz (SPI up to F_CPU / 2), 48 MHz above. Hardware-tested at
  200, 133 and 50 MHz (`tests/sketches/cpu_speed`); other speeds not yet.
- Tools > Optimize menu: `-Os` (default), `-O2`, `-O3`, `-Og`.
- Pre-release reminder: a post-commit hook (`.githooks/`) runs
  `package/prerelease_reminder.py`, which lists the CHANGELOG entries since the
  last tag once there are 10 or more.

### Changed
- Stage plan: the Analog Shield and Analog Shield2 libraries move to their own
  repository (ADR 008). Old Stages 2 and 6 removed; FreeRTOS/multi-core is now
  v0.1.1 and the Pico2 stages are v0.2.0 and v0.2.1.

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
- STUSB4500 register addresses (from ST's register map) confirmed on hardware
  2026-10-09 (checklist 14-15: profiles, contract and renegotiation all work).
- Hardware checklist (2026-10-05, one Phyduino Pico): items 1-8, 9b, 10, 10b, 11, 16
  passed. 2026-10-08: 12 (BMP280 at 0x77, seesaw at 0x36) and 13 (Pico 2 as
  controller) passed. 2026-10-09: 9 (piezo), 14 and 15 (USB-PD charger, Pico 2
  running serial_bridge) passed: 20.2 V with POK3 lit, then 9.1 V, 5.15 V and back
  to 20.2 V on command. 5b passed: Arduino IDE 2 Upload button, no port selected,
  board already in BOOTSEL, with the IDE's built-in Blink example. All items passed.
  New items for the CPU Speed menu: 17 (`cpu_speed` at 50/133/200 MHz) passed
  2026-10-09; 18 (MAX11635 A3 at 200 MHz, Analog Shield temperature front end,
  ≈ 1.39 V) passed 2026-10-09: same reading as at 133 MHz within sensor drift.
- The first contract after plugging into a charger asks 1000 mA at 20 V, although
  the chip's profiles (loaded from its NVM) already read 20 V/0.5 A; after a
  renegotiation (`setProfiles()`) it asks 500 mA. Cause not known yet (probably an
  STUSB4500 NVM setting); `begin()` does not touch the profiles.
- `usbpd_status` read one command character per second, so a burst of input was
  acted on seconds late; it now handles all waiting input each loop (fix not yet
  run on hardware).
- Board seen with A3-A5 100 kOhm pull-downs (R47-R49) apparently not connected:
  unconnected inputs float. Hardware, not firmware.
