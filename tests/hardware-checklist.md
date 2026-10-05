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
| 5 | (any) | Upload twice from IDE | Second upload works without touching the board (1200-baud touch) |
| 6 | `analog_read` | Jumper AREF (2.048 V) to each of A0–A5 in turn | A0–A2 ≈ 2540 (3.3 V scale), A3–A5 = 4095 (2.048 V scale); only the jumpered input changes |
| 7 | `pwm_fade` | none | LED fades smoothly up and down |
| 8 | `interrupts` | Button D2 → GND | Count goes up once per press |
| 9 | `tone_test` | Piezo on D8 | Eight-note scale |
| 9b | `tone_loopback` | Wire D8 → D2 (no buzzer needed) | Measured ≈ set frequency, duty ≈ 50 %; timed tone playing at 100 ms, stopped at 300 ms |
| 10 | `uart_loopback` | Wire D1 → D0 | Counter values echo in Serial Monitor |
| 10b | `uart2_loopback` | Wire D4 → D5 | Counter values echo in Serial Monitor (Serial2) |
| 11 | `SpiLoopback` | Wire D11 → D12 | received == sent |
| 12 | `I2CScanner` | Any I2C device on SDA/SCL | Its address is listed |
| 13 | `PeripheralReceiver` | Second board as controller | Bytes print; reads return a counter |

### USB-PD

| # | Sketch | Setup | Expected |
|---|---|---|---|
| 14 | `usbpd_status` | USB-PD charger that offers 20 V | "STUSB4500 found"; profiles 5.0V/2.0A 9.0V/1.0A 20.0V/0.5A; VBUS ≈ 20 V, POK3 lit |
| 15 | `usbpd_status` | Same charger; send `9`, then `5`, then `f` | VBUS → ≈ 9 V (POK2 lit), → ≈ 5 V, → ≈ 20 V |
| 16 | `usbpd_status` | Ordinary USB port (no PD) | contract offer 0, VBUS ≈ 5 V |
