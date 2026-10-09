# Hardware checklist

Run before every release on a real board (and after core changes in the area).
Sketches are in `tests/sketches/` and `platform/libraries/*/examples/`.

## Phyduino Pico

| # | Sketch | Setup | Expected |
|---|---|---|---|
| 1 | (any) | New board: SW1 to GND, reset, upload, SW1 back | Sketch runs; later uploads work with no buttons |
| 2 | `blink` | none | LED blinks at 1 Hz |
| 3 | `sdk_only_blink` | none | LED blinks at 2 Hz (pico-sdk calls only) |
| 4 | `serial_echo` | Serial Monitor 115200 | Banner appears; typed text echoes back |
| 5 | (any) | Upload twice from IDE | Second upload works without touching the board (1200-baud touch, then copy to RPI-RP2) |
| 5b | (any) | Board already in BOOTSEL (RPI-RP2 drive showing, no port) | Upload without a port still works |
| 6 | `analog_read` | Jumper AREF (2.048 V) to each of A0–A5 in turn | A0–A2 ≈ 2540 (3.3 V scale), A3–A5 = 4095 (2.048 V scale); only the jumpered input changes |
| 7 | `pwm_fade` | none | LED fades smoothly up and down |
| 8 | `interrupts` | Button D2 → GND | Count goes up once per press |
| 9 | `tone_test` | Piezo on D8 | Eight-note scale |
| 9b | `tone_loopback` | Wire D8 → D2 (no buzzer needed) | Measured ≈ set frequency, duty ≈ 50 %; timed tone playing at 100 ms, stopped at 300 ms |
| 10 | `uart_loopback` | Wire D1 → D0 | Counter values echo in Serial Monitor |
| 10b | `uart2_loopback` | Wire D4 → D5 | Counter values echo in Serial Monitor (Serial2) |
| 11 | `SpiLoopback` | Wire D11 → D12 | received == sent |
| 12 | `I2CScanner` | Any I2C device on SDA/SCL | Its address is listed |
| 13 | `PeripheralReceiver` | Second board running `i2c_controller` on SDA/SCL/GND (e.g. a Raspberry Pi Pico 2; 5 V boards such as the UNO R4 need a level shifter) | "hello N" lines print; controller reads a rising counter |

### USB-PD

| # | Sketch | Setup | Expected |
|---|---|---|---|
| 14 | `usbpd_status` | USB-PD charger that offers 20 V | "STUSB4500 found"; profiles 5.0V/2.0A 9.0V/1.0A 20.0V/0.5A; VBUS ≈ 20 V, POK3 lit |
| 15 | `usbpd_status` | Same charger; send `9`, then `5`, then `f` | VBUS → ≈ 9 V (POK2 lit), → ≈ 5 V, → ≈ 20 V |
| 16 | `usbpd_status` | Ordinary USB port (no PD) | contract offer 0, VBUS ≈ 5 V |

## Wiring diagrams

Tests not listed need no wiring.

### 6 · `analog_read`

AREF jumper to each analog input in turn.

```text
 Phyduino Pico
 ┌───────────────┐
 │ AREF 2.048 V ●├──┐ jumper
 │ A0           ●├──┘
 │ A1           ●│
 │ A2           ●│   move the jumper's free end
 │ A3           ●│   to A0, then A1 … A5,
 │ A4           ●│   one input at a time
 │ A5           ●│
 └───────────────┘
```

### 8 · `interrupts`

Push button from D2 to GND (internal pull-up).

```text
 Phyduino Pico
 ┌──────────────┐        button
 │ D2          ●├───────o  o──────┐
 │              │                 │
 │ GND         ●├─────────────────┘
 └──────────────┘
```

### 9 · `tone_test`

Piezo buzzer from D8 to GND.

```text
 Phyduino Pico          piezo
 ┌──────────────┐      ┌─────┐
 │ D8          ●├──────┤+    │
 │ GND         ●├──────┤-    │
 └──────────────┘      └─────┘
```

### 9b · `tone_loopback`

D8 looped back to D2.

```text
 Phyduino Pico
 ┌──────────────┐
 │ D8          ●├──┐
 │              │  │ jumper
 │ D2          ●├──┘
 └──────────────┘
```

### 10 · `uart_loopback`

Serial1: D1 (TX) looped back to D0 (RX).

```text
 Phyduino Pico
 ┌──────────────┐
 │ D1 TX       ●├──┐
 │              │  │ jumper
 │ D0 RX       ●├──┘
 └──────────────┘
```

### 10b · `uart2_loopback`

Serial2: D4 (TX) looped back to D5 (RX).

```text
 Phyduino Pico
 ┌──────────────┐
 │ D4 TX       ●├──┐
 │              │  │ jumper
 │ D5 RX       ●├──┘
 └──────────────┘
```

### 11 · `SpiLoopback`

SPI: D11 (MOSI) looped back to D12 (MISO).

```text
 Phyduino Pico
 ┌──────────────┐
 │ D11 MOSI    ●├──┐
 │              │  │ jumper
 │ D12 MISO    ●├──┘
 └──────────────┘
```

### 12 · `I2CScanner`

Any 3.3 V I2C device (tested: BMP280 and seesaw encoder).

```text
 Phyduino Pico              I2C device (3.3 V)
 ┌──────────────────┐       ┌──────────┐
 │ 3.3V            ●├───────┤ VIN      │
 │ GND             ●├───────┤ GND      │
 │ SDA (D18,GPIO24)●├───────┤ SDA      │
 │ SCL (D19,GPIO25)●├───────┤ SCL      │
 └──────────────────┘       └──────────┘
   No pull-ups on the Phyduino; most breakouts have their own.
```

### 13 · `PeripheralReceiver + i2c_controller`

Raspberry Pi Pico 2 as controller.

```text
 Raspberry Pi Pico 2                Phyduino Pico
 (i2c_controller)                   (PeripheralReceiver, 0x42)
 ┌────────────────┐                 ┌───────────────────┐
 │ pin 6  GP4 SDA●├─────────────────┤● SDA (D18, GPIO24)│
 │ pin 7  GP5 SCL●├─────────────────┤● SCL (D19, GPIO25)│
 │ pin 8  GND    ●├─────────────────┤● GND              │
 └────────────────┘                 └───────────────────┘

 Pull-ups (were needed), anywhere on the bus:

   3.3 V ──┬────────┐
         [4.7k]   [4.7k]
           │        │
          SDA      SCL

   Don't join the 3V3 pins of the two boards.
   5 V controllers (e.g. UNO R4) need a level shifter.
```

### 14 / 15 · `usbpd_status`

Charger powers the board; a Pico 2 running `serial_bridge` passes Serial1 to the PC
(`usbpd_status` mirrors its output to Serial1). Open the Pico 2's Serial Monitor
before plugging in the charger, or the "STUSB4500 found" line is missed.

```text
 USB-PD charger (20 V)                          PC
        │ USB-C                                  │ USB
        ▼                                        ▼
 ┌────────────────────┐              ┌─────────────────────┐
 │ Phyduino Pico      │              │ Raspberry Pi Pico 2 │
 │ (usbpd_status)     │              │ (serial bridge)     │
 │ D1 TX (GPIO0)     ●├──────────────┤● GP1 RX  (pin 2)    │
 │ D0 RX (GPIO1)     ●├──────────────┤● GP0 TX  (pin 1)    │
 │ GND               ●├──────────────┤● GND     (pin 3)    │
 │ POK2 / POK3 LEDs   │              └─────────────────────┘
 └────────────────────┘
   Only these three wires: VBUS is 20 V on a 20 V contract.
```

### 16 · `usbpd_status`

Ordinary USB port on the PC.

```text
 PC USB port ──── USB-C ────► Phyduino Pico (usbpd_status)
   5 V only; Serial Monitor at 115200
```
