# Changelog

Format: one section per version, newest first. Stage plan: README.md.

## Unreleased: v0.1.1 (Stage 2: FreeRTOS and multi-core)

Design: ADR 009 (docs/decisions/009-multicore-freertos.html) and ADR 010
(docs/decisions/010-cmsis-rtos2.html). Built and host-tested. Hardware
checklist (2026-10-19, one Phyduino Pico): 28-32, 34 and 36 passed
(MulticoreBasics with all three Operating System options, MulticoreFifo
without FreeRTOS, `multicore_idle` with None and FreeRTOS, FreeRtosTasks with
FreeRTOS and CMSIS-RTOS2, `serial_echo` and the IDE's 1200-baud upload reset
with FreeRTOS); 33 and 35 not yet run.

### Added
- Second core, arduino-pico compatible: a sketch that defines `setup1()` and
  `loop1()` runs them on core 1 at the same time as `setup()`/`loop()`.
- `rp2040` object (`RP2040.h`): `rp2040.fifo` (`push()`, `push_nb()`, `pop()`,
  `pop_nb()`, `available()`; 16 values each way, in RAM), `idleOtherCore()`,
  `resumeOtherCore()`, `restartCore1()`, `cpuid()`, `f_cpu()`,
  `getTotalHeap()`, `getUsedHeap()`, `getFreeHeap()`.
- Tools > Operating System menu: None (default) or FreeRTOS (SMP). FreeRTOS
  links a second SDK bundle, `platform/system/phyduino_pico_freertos/`,
  with the SDK built for the FreeRTOS RP2040 port (SDK mutexes and
  `sleep_ms()` block the task) and the kernel (heap_3). `setup()`/`loop()`
  and `setup1()`/`loop1()` run as tasks pinned to cores 0 and 1 (8 KB
  stacks, priority 1). Configuration: `sdk-builder/freertos/FreeRTOSConfig.h`.
- FreeRTOS kernel: submodule `extern/FreeRTOS-Kernel` (Raspberry Pi fork,
  4f7299d, V11.1.0+); licences in `platform/licenses/`.
- With FreeRTOS, `delay()` blocks the calling task and the default `yield()`
  lets tasks of the same priority run. Stack overflow, out of memory and
  failed kernel checks stop the board (weak hooks a sketch can replace).
- Including `FreeRTOS.h` without the menu option stops the build with
  "FreeRTOS is off. Choose Tools > Operating System > FreeRTOS (SMP)".
- Phyduino library examples MulticoreBasics, MulticoreFifo, FreeRtosTasks
  and FreeRtosQueue; test sketch `tests/sketches/multicore_idle`.
- `sdk-builder/`: `PHYDUINO_OS=freertos` and preset `phyduino_pico_freertos`.
  CMake verification build: preset `phyduino_pico_freertos` and
  `PHYDUINO_MENUS` (any Tools menu option). `tests/compile_all.py` adds
  `os=freertos` for sketches that include `FreeRTOS.h`. CI builds both.
- Host tests for the FIFO, core 1 start-up, `idleOtherCore()` and
  `restartCore1()`; 95 cases.
- CMSIS-RTOS2: Tools > Operating System > FreeRTOS (SMP) + CMSIS-RTOS2 adds
  ARM's CMSIS-RTOS2 API (`#include <cmsis_os2.h>`) on the same FreeRTOS
  bundle, through ARM's CMSIS-FreeRTOS wrapper (v11.3.0, with CMSIS_6 v6.4.0
  headers) in `lib/libcmsis_os2.a`, linked only with this option. `loop()`
  and `loop1()` run at `osPriorityNormal`. Sources copied into
  `sdk-builder/cmsis-rtos2/`; licences in `platform/licenses/`. Including
  `cmsis_os2.h` without the option stops the build with a message naming it.
- Phyduino library example CmsisRtos2Threads. CMake verification preset
  `phyduino_pico_cmsis`; `tests/compile_all.py` adds `os=cmsis` for sketches
  that include `cmsis_os2.h`; CI builds it.

### Changed
- FreeRTOS: 56 priorities (0-55, was 0-7), as CMSIS-RTOS2 requires; the
  timer task is at 55. `configCPU_CLOCK_HZ` follows Tools > CPU Speed.
- `Serial1`/`Serial2` receive buffers are guarded by a critical section
  (spin lock) instead of masking interrupts, so either core can read a port
  whichever core called `begin()`.

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
- STUSB4500 USB-PD driver (`stusb4500.h`); profiles from Tools > USB-PD Power
  (default 5 V only) loaded at start-up (working registers only, NVM untouched).
  `UsbPdPower` example in the `Phyduino` library (File > Examples > Phyduino);
  not yet run on hardware (checklist 27).
- `sdk-builder/` generating the committed SDK bundle.
- Release tooling (`package/`) and CI (`.github/`).
- CMake verification build (`CMakeLists.txt`, `cmake/`): same flags as the
  Arduino build, read from platform.txt/boards.txt, with `-Werror`. Not shipped.
- PC-side unit tests (`tests/host/`, doctest): core, variant drivers and
  libraries against a fake pico-sdk with MAX11635/STUSB4500 models; 87 cases.
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
- Tools > USB-PD Power (VIN) menu: 5 V only (default), up to 9, 12, 15 or 20 V
  (top profile at 10 W), or the chip's stored settings. VBUS feeds VIN, so VIN
  follows the contract. Working registers only (`stusb4500::boardProfiles()`).
- `tests/sketches/usbpd_nvm`: one-off tool that sets the STUSB4500 NVM to 5 V only
  (sector 3 only, skipped if already set, read back to verify; `factory` undoes
  it), so VIN is 5 V from plug-in until a sketch asks for more.
- GitHub release notes come from the annotated tag's message (`git tag -a`);
  a lightweight tag still gets "See CHANGELOG.md".
- Pre-release reminder: a post-commit hook (`.githooks/`) runs
  `package/prerelease_reminder.py`, which lists the CHANGELOG entries since the
  last tag once there are 10 or more.
- Analog acquisition on every analog pin (A0-A5, whichever ADC):
  `analogReadAveraging()` (1/4/8/16/32; in hardware on the MAX11635),
  `analogReadMillivolts()`, `analogFullScaleMillivolts()`,
  `analogReadDifferentialMillivolts()` (A3-A4 true simultaneous differential,
  ±1.024 V; other pairs from two readings), `analogReadMultiple()` (one MAX11635
  scan for A3-A5), and fixed-rate bursts `analogReadBurst()` /
  `analogStartBurst()` / `analogBurstDone()` / `analogStopBurst()` (A0-A2 up to
  500 ksps with ADC divider + DMA; A3-A5 timer-paced up to 20 ksps, clock mode
  11 + DMA up to 300 ksps). Variants provide eight `variantAnalog*()` hooks (ADR
  003 extended); `max11635.h` gains averaging, scan, differential and bursts.
  New bundled `Phyduino` library with six analog examples (File > Examples > Phyduino).
  Host-tested; not yet run on hardware (checklist 21-26, 25b is the clock
  mode 11 byte alignment).

### Changed
- Default USB-PD request is now 5 V only (was up to 20 V): VIN is 5 V unless
  Tools > USB-PD Power asks for more.
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

### Fixed
- Release workflow merged the package index of the newest full release only
  (`releases/latest` skips pre-releases), so each pre-release index listed only
  itself. It now merges the index of the newest `v*` release, pre-releases
  included.

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
- Solved 2026-10-09: the first contract after plug-in asked 1000 mA at 20 V
  because the NVM held ST's factory profiles (5 V/1.5 A, 15 V/1.5 A, 20 V/1.0 A);
  the "20 V/0.5 A" seen earlier was the working registers after `initVariant()`.
  The test board's NVM is now 5 V only (checklist 19, passed); its original
  contents (factory, except sector 0 byte 3 = AB) are recorded in
  docs/development/hardware-checklist.html. Item 20 (20 V charger: 5 V at plug-in,
  menu still raises it) still to do.
- `usbpd_status` read one command character per second, so a burst of input was
  acted on seconds late; it now handles all waiting input each loop (fix not yet
  run on hardware).
- Board seen with A3-A5 100 kOhm pull-downs (R47-R49) apparently not connected:
  unconnected inputs float. Hardware, not firmware.
