# 003: analogRead() covers external ADC channels transparently

**Decision.** `analogRead(A0..A5)` works on every header analog pin. Pins with
no RP2040 GPIO behind them (Phyduino Pico A3–A5 → MAX11635) are routed by the
core to the variant hook `variantAnalogRead()`.

**Why.** Students and the standard Arduino examples expect `analogRead(A3)`
to just work. The same hook will serve the Phyduino Pico2, where all of
A0–A5 go through an external ADS7038.

**Cost.** Full-scale voltage differs between pins (3.3 V vs 2.048 V on the
Pico), which is documented in `docs/boards/phyduino_pico.md`. The driver must
live in the variant (not a library) because the core calls it.
