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

### CPU Speed

| # | Sketch | Setup | Expected |
|---|---|---|---|
| 17 | `cpu_speed` | Tools > CPU Speed at 50, 133 (default) and 200 MHz; Serial Monitor 115200 | LED 1 Hz; sys = F_CPU; peri = sys up to 133 MHz, 48 MHz above; vreg 1.15 V above 133 MHz, else 1.10 V; delay(1000) ≈ 1000000 us; loop time ∝ 1/F_CPU |
| 18 | `analog_read` | CPU Speed 200 MHz; known voltage < 2.048 V on A3 | A3 reads the applied voltage, same as at 133 MHz (MAX11635 conversion pulse long enough at full speed) |

### USB-PD

| # | Sketch | Setup | Expected |
|---|---|---|---|
| 14 | `usbpd_status` | Tools > USB-PD Power: Up to 20 V; USB-PD charger that offers 20 V | "STUSB4500 found"; profiles 5.0V/2.0A 9.0V/1.0A 20.0V/0.5A; VBUS ≈ 20 V, POK3 lit |
| 15 | `usbpd_status` | Same charger; send `9`, then `5`, then `f` | VBUS → ≈ 9 V (POK2 lit), → ≈ 5 V, → ≈ 20 V |
| 16 | `usbpd_status` | Ordinary USB port (no PD) | contract offer 0, VBUS ≈ 5 V |
| 19 | `usbpd_nvm` | New board, once: USB-PD Power "Use chip's stored settings"; send `r`, then `write5v`; unplug and replug | `r` shows the NVM (ST factory: 3 profiles 5 V/1.5 A, 15 V/1.5 A, 20 V/1.0 A); after `write5v` the read-back shows 1 profile, 5 V/2.0 A, "OK: verified". Keep the printed `r` output as the board's backup |
| 20 | `usbpd_status` | After 19: USB-PD Power "Use chip's stored settings", 20 V charger | VBUS ≈ 5 V from plug-in, profiles 5.0V/2.0A; then rebuilt with Up to 20 V: VBUS ≈ 20 V (the menu changes the working registers only; `r` in `usbpd_nvm` still shows 5 V only) |
| 27 | `UsbPdPower` | Same setup as 14, but Tools > USB-PD Power: 5 V only (default); send `20`, then `9`, then `5`, then `7` | At start: profiles 5.0V/2.00A, VBUS ≈ 5 V; `20`: "Asking for up to 20 V", profiles 5.0V/2.00A 9.0V/1.00A 20.0V/0.50A, VBUS ≈ 20 V (POK3 lit); `9`: VBUS ≈ 9 V (POK2 lit); `5`: VBUS ≈ 5 V; `7`: "Send 5, 9, 12, 15 or 20." |

### Multicore, FreeRTOS and CMSIS-RTOS2 (v0.1.1)

| # | Sketch | Setup | Expected |
|---|---|---|---|
| 28 | `MulticoreBasics` | Tools > Operating System: None; Serial Monitor 115200 | "setup() runs on core 0", then "loop() on core 0: n" once a second; LED flashes 100 ms on, 400 ms off at the same time |
| 29 | `MulticoreFifo` | As 28 | LED blinks at 2 Hz throughout; four answers in order: 168, 1229, 9592, 78498 primes, the last about 20 s after the third (133 MHz) |
| 30 | `multicore_idle` | As 28 | "PASS" once a second; echo = round + 1; count while idle +0, after resume a large number; LED blinks |
| 31 | `multicore_idle`, `MulticoreBasics` | As 28 and 30, but Tools > Operating System: FreeRTOS (SMP) | Same results as 28 and 30 |
| 32 | `FreeRtosTasks` | FreeRTOS; Serial Monitor 115200 | LED toggles every 250 ms; "countTask on core 0/1: n" once a second; every 5 s a list with loop, blink, count, IDLE0, IDLE1, Tmr Svc; Cores 0x1 for loop, 0xffffffff for blink and count; no task's Stack column near 0 |
| 33 | `FreeRtosQueue` | FreeRTOS; potentiometer wiper (or AREF) on A0 | A line every 100 ms, times 100 ms apart; A0 matches `AnalogMillivolts`; no "Queue full" |
| 34 | `serial_echo` | FreeRTOS; type, then Upload another sketch with the port selected | Echo works; the IDE's 1200-baud reset still starts the upload (no BOOTSEL button) |
| 35 | `CmsisRtos2Threads` | Tools > Operating System: FreeRTOS (SMP) + CMSIS-RTOS2; potentiometer wiper (or AREF) on A0 | "Kernel: FreeRTOS V11.1.0+"; LED flashes once a second (100 ms on); a line every 100 ms, times 100 ms apart; A0 matches `AnalogMillivolts`; no "Queue full" |
| 36 | `FreeRtosTasks`, `MulticoreBasics` | As 35 (CMSIS-RTOS2 option) | Same results as 32 and 28, except that Prio is 23 for loop and 55 for Tmr Svc |

### Analog acquisition (`Phyduino` examples)

| # | Sketch | Setup | Expected |
|---|---|---|---|
| 21 | `AnalogMillivolts` | AREF jumper to each of A0–A5 in turn (as in 6); Serial Monitor 115200 | Full scale 3300 mV for A0–A2, 2048 mV for A3–A5; the jumpered pin reads ≈ 2048 mV on A0–A2 and ≈ 2047 mV on A3–A5 |
| 22 | `AnalogAveraging` | Potentiometer across 3.3 V and GND, wiper on A0; a second potentiometer across AREF and GND, wiper on A3; Serial Monitor 115200. Not AREF on A0 or a 3.3 V potentiometer on A3: each then follows the other ADC's reference and carries the slow ≈ 1 % drift of the 3.3 V rail, which averaging cannot remove | Spread shrinks as averaging rises from 1 to 32: clearly on A0 (≈ 10 → 1), only slightly on A3 (the MAX11635 is already quiet, ≈ 5 → 3) |
| 23 | `AnalogDifferential` | Potentiometer wiper on A3 and A0 (one potentiometer for both, or one each; keep A3 under 2.048 V, e.g. a potentiometer across AREF and GND), A4 and A1 to GND; turn from 0 V up | A3 − A4 matches A3 in `AnalogMillivolts`, below and above 1024 mV (above it, two readings); A0 − A1 matches A0 |
| 24 | `AnalogReadMultiple` | Serial Plotter 115200; AREF jumper to one analog pin at a time, or known, different voltages on several pins at once (e.g. the wiring of 23) | Six traces, ≈ 20 lines a second; only the jumpered pin's trace rises to ≈ 2048 mV (or each trace matches that pin in `AnalogMillivolts`) |
| 25 | `AnalogBurst` | Wire D2 → A0 (sketch plays 500 Hz on D2) | Plot shows a clean square wave, 40 samples per period; "Rate 20000 Hz"; mean ≈ 2048 (1950–2050: 500 samples are 12.5 periods, so it depends on where the burst starts) |
| 25b | `AnalogBurst` | As 25, but `PIN = A3`, `RATE_HZ = 200000` (MAX11635 clock mode 11); D2 → A3 through 10 kΩ, 10 kΩ from A3 to GND (1.65 V) | Square wave between ≈ 0 and ≈ 3200–3300 counts (1.60–1.65 V, resistor tolerance); edges settle within ~8 samples (divider and the input filter); flat parts steady, with no isolated spikes or jumps by powers of two (those would mean misaligned result bytes); "Rate 197916 Hz" (nearest SPI clock to 200000); every burst the same. Rate / 500 ≈ 396 samples per period (edges ≈ 198 samples apart; 181 would mean the SPI is back in mode 0, which adds a gap between frames) |
| 26 | `AnalogBurstBackground` | Steady voltage under 2.048 V on A3 (e.g. potentiometer); Serial Monitor 115200 | LED blinks steadily at 2 Hz, from when the Serial Monitor opens (`setup()` waits for it); ten lines a second; mean matches A3 in `AnalogMillivolts`, RMS of a few tenths of a mV |

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
