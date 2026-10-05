# 006: PC-side tests use a small fake pico-sdk, not the SDK's host platform

**Decision.** `tests/host/` compiles the real core, variant and library code for
the PC against a fake pico-sdk (`tests/host/fake_sdk/`) that records hardware
state, plus behavioural models of the on-board chips (`tests/host/models/`).
Tests use doctest (`extern/doctest`, pinned).

**Why not `PICO_PLATFORM=host`.** In pico-sdk 2.2.0 the host platform replaces
only gpio, irq, sync, timer, uart and divider, mostly with empty functions that
keep no state; it has no ADC, PWM, SPI, I2C, clocks or TinyUSB, and supports
Linux/macOS/Cygwin/WSL rather than plain Windows. We would still have had to
fake most of what the code touches, and tests could not observe the stubs.

**Cost.** The fake covers only the SDK functions this code calls (it fails to
compile when new ones are used, which is the cue to extend it). Chip models
encode our reading of the datasheets, so hardware tests are still needed for
register addresses, timing and electrical behaviour.
