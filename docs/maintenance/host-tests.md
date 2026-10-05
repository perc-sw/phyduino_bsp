# PC-side unit tests

The real core, variant and library sources, compiled for your PC and run
against a **fake pico-sdk** that simulates the hardware. Tests check what the
code does to the hardware ("`pinMode(OUTPUT_OPENDRAIN)` then
`digitalWrite(LOW)` drives the pin low", "`setProfiles()` writes these PDO
bits"), with no board attached. Not shipped (lives in `tests/host/`).

## Run

From the repo root:
```
cmake --workflow --preset host-tests
```
Needs a PC C++17 compiler (GCC or Clang; on Windows MSYS2 UCRT64 works), CMake ≥ 3.21,
Ninja, and the doctest submodule (`git submodule update --init extern/doctest`).
For one test or more detail, run the exe directly:
```
build/host-tests/tests/host/host_tests -tc="*tone*" -s
```
(`--help` lists doctest's options.)

## Layout

| Path | What |
|---|---|
| `tests/host/test_*.cpp` | One file per area: pins, digital, analog, PWM/tone, time/pulse/shift, interrupts, Serial1/2, SPI, Wire, variant chips, strings/random |
| `tests/host/test_support.h` | Include in every test: doctest + fake SDK + `Arduino.h`; `HostTest` fixture |
| `tests/host/fake_sdk/` | `fake_sdk.h/.cpp`: the SDK functions the code calls, backed by `fake::hw` (GPIO, ADC, PWM, UART, SPI, I2C, timer, alarms, IRQs) |
| `tests/host/models/` | MAX11635 and STUSB4500 models attached to the fake SPI0 / I2C1 |
| `tests/host/CMakeLists.txt` | Picks up the sources, generates the fake SDK header names (`FAKE_SDK_HEADERS`) |

Every `.cpp` in the core, variant and libraries is compiled automatically,
except `main.cpp`, `SerialUSB.cpp`, `usb/` and `api/PluggableUSB.cpp` (they need
TinyUSB). Arduino defines (`ARDUINO_*`, `F_CPU`, `USB_*`) come from
`platform.txt`/`boards.txt`, like the other builds.

## Writing a test

```cpp
#include "test_support.h"

TEST_CASE_FIXTURE(HostTest, "what it checks, in words") {
    pinMode(LED_BUILTIN, OUTPUT);              // code under test
    digitalWrite(LED_BUILTIN, HIGH);
    CHECK(gpioOf(LED_BUILTIN).out_level);      // simulated hardware state
}
```

- `HostTest` resets `fake::hw` to power-on state. Static state inside the code
  under test (analogRead resolution, `Serial1`/`Wire` started flags, ...) is
  not reset, as on a board: set what your test relies on, and `end()` what
  you `begin()`.
- Drive inputs through `fake::hw`: `gpio[n].input_level`, `gpio[n].waveform`
  (level over time), `adc_values[]`, `uart[n].rx`, `spi[n].device`,
  `i2c[n].on_write/on_read`; `fake::advance_us()`, `fake::raise_irq()`,
  `fake::trigger_gpio_irq()`.
- Time moves 1 µs on every timer read, so busy-waits end.
- Code that includes an SDK header not yet faked fails to compile: add the
  header to `FAKE_SDK_HEADERS` and the functions to `fake_sdk.h/.cpp`.

## Limits

The fake implements the SDK *as this code uses it*, not the real chip. Chip
register addresses (e.g. STUSB4500) are built into the models, so the tests
prove the driver logic and encoding, not the addresses; that, timing and
electrical behaviour still need `tests/hardware-checklist.md`.
