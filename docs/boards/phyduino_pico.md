# Phyduino Pico (RP2040)

FQBN `phyduino:rp2:phyduino_pico`. Hardware design: `../phyduino_pico` (KiCad, rev D/E).

## Core parts

| Part | Details |
|---|---|
| MCU | RP2040 (U4), 125 MHz |
| Flash | W25Q128JV (U9), 16 MB; boot stage 2 = SDK `W25Q080` (works for the whole W25Q family) |
| Crystal | 12 MHz (Y1) |
| LED | GPIO5, active high (`LED_BUILTIN`, Arduino pin 22) |
| Boot mode | Slide switch SW1: 3.3 V = normal, GND = BOOTSEL (UF2 drive). No BOOTSEL button. |
| Reset | SW2 (also header power pin 3) |
| Debug | J1, 3-pin JST-SH SWD |

## Pin map

The authoritative table is the comment at the top of
`platform/variants/phyduino_pico/pins_arduino.h`. Summary:

| Arduino pin | Header | RP2040 | Notes |
|---|---|---|---|
| 0 / 1 | D0 RX / D1 TX | GPIO1 / GPIO0 | `Serial1` (UART0) |
| 2–9 | D2–D9 | GPIO6,7,8,9,10,15,16,17 | D4/D5 = `Serial2` (UART1) |
| 10–13 | D10 SS, D11 MOSI, D12 MISO, D13 SCK | GPIO13, 11, 12, 14 | `SPI` (SPI1) |
| 14–16 | A0–A2 | GPIO26–28 | RP2040 ADC, 3.3 V full scale; also usable as digital pins |
| 17–19 | A3–A5 | — | MAX11635 ADC, **2.048 V full scale**, analog only |
| 20 / 21 | SDA / SCL | GPIO24 / GPIO25 | `Wire` (I2C0); silkscreen D18/D19 |
| 22 | (LED) | GPIO5 | `LED_BUILTIN` |

Numbering follows the Uno (A0–A5 = 14–19, so `for (p = A0; p <= A5; p++)`
works). The silkscreen's D18/D19 names for SDA/SCL are not Arduino pin
numbers here; use `SDA`/`SCL` (ADR 004).

## On-board peripherals (not on the header)

| Chip | Bus / pins | Used by |
|---|---|---|
| MAX11635 12-bit ADC (U5) | SPI0: SCK 22, MOSI 23, MISO 20, CS 21, EOC 19, CNVST 18 | `analogRead(A3..A5)` (ch 0–2); ch 3 = VBUS ÷ (100k/10.2k) |
| LM4040 2.048 V reference (U7) | AREF header pin | MAX11635 reference only |
| STUSB4500 USB-PD sink (U6) | I2C1: SDA 2, SCL 3, ALERT 4, RESET 29; address 0x28 | power profiles at start-up; `#include <stusb4500.h>` |

## USB Power Delivery

At start-up `initVariant()` loads the board's default sink profiles into the
STUSB4500 (`platform/variants/phyduino_pico/variant.cpp`):

| Profile | Voltage | Current | LED |
|---|---|---|---|
| 1 | 5.0 V | 2.0 A | POK2 off, POK3 off |
| 2 | 9.0 V | 1.0 A | POK2 on |
| 3 | 20.0 V | 0.5 A | POK3 on |

A USB-PD charger supplies the highest profile it can; an ordinary USB port gives
5 V. Profiles go to the chip's working registers, never its NVM, and the board
renegotiates only if they differ from what the chip already holds. A sketch can
change them with `stusb4500::setProfiles()` and read the contract and measured
VBUS (see `platform/variants/phyduino_pico/stusb4500.h` and
`tests/sketches/usbpd_status`). The register addresses come from ST's register
map, not the datasheet; `tests/hardware-checklist.md` item 14 checks them.

## Things to know

- **Analog full scale differs**: A0–A2 = 3.3 V, A3–A5 = 2.048 V. With the
  default 10-bit `analogRead()`, volts = `reading * 3.3 / 1023` on A0–A2 but
  `reading * 2.048 / 1023` on A3–A5. `analogReference()` does nothing.
- **No I2C pull-ups on the board.** `Wire` enables the RP2040's weak internal
  pull-ups; use 4.7 kΩ to 3.3 V for longer wires or 400 kHz.
- **GPIO25 (SCL) needs 12 mA drive** (schematic note); `Wire` sets it.
- **PWM pairs**: GPIO n and n^1 share a PWM slice and so a frequency
  (e.g. D2/D3, D4/D5). `tone()` on one changes the other's frequency.
- **USB-PD bus**: the STUSB4500 is on I2C1 (GPIO2/3). `phyduino_pico_bios`
  sets it to `i2c0` by mistake; this core uses I2C1. Wire (I2C0) is separate.
