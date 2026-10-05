# 001: Own Arduino core built directly on the pico-sdk

**Decision.** Phyduino boards use their own core (`platform/cores/phyduino`),
written against the pico-sdk and ArduinoCore-API. It does not depend on
another RP2040 Arduino core such as arduino-pico or ArduinoCore-mbed.

**Why.** The board package is a teaching tool that must stay small and
understandable for one maintainer coming back after months away. A thin core
over the SDK is easy to read end to end, and students can use the SDK
directly from sketches. Depending on a large third-party core would tie
releases to its schedule and internals.

**Cost.** Every Arduino API feature has to be implemented here. The scope is
kept to what the course needs; the SDK covers the rest.
