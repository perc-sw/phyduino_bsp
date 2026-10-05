# 004: Uno-style pin numbers; SDA/SCL get 20/21

**Decision.** D0–D13 = 0–13, A0–A5 = 14–19 (as on an Uno), SDA = 20, SCL = 21,
LED_BUILTIN = 22.

**Why.** On an Uno, 18/19 are A4/A5 *and* SDA/SCL. On the Phyduino Pico,
SDA/SCL (GPIO24/25) are separate from A4/A5 (MAX11635), so they can't share
numbers. Keeping A0–A5 contiguous at 14–19 keeps `for (p = A0; p <= A5; p++)`
and Uno examples working. Sketches use SDA/SCL by name, so their numbers
rarely matter.

**Cost.** The silkscreen labels D18/D19 don't match pin numbers 18/19 (those
are A4/A5). This is documented on the board page.
