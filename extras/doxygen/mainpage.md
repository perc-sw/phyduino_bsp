# API reference

Arduino IDE support for the University of Toronto Department of Physics
student-workshop boards: a standalone Arduino core built directly on the
Raspberry Pi pico-sdk. Install steps and the supported boards are on the
[home page](../index.html); the pin map and board notes are on the
[Phyduino Pico page](../boards/phyduino_pico.html).

This reference covers the Arduino functions and what is different about them
on Phyduino boards, plus what the Phyduino package adds. Other parts of the
Arduino API (`String`, `Serial.print()`, `isDigit()`, ...) work as described
in the [Arduino language reference](https://docs.arduino.cc/language-reference/).

## Arduino functions

| Topic | Functions |
|---|---|
| @ref core_digital | pinMode(), digitalWrite(), digitalRead() |
| @ref core_analog | analogRead(), analogWrite(), resolution and PWM frequency; on every analog pin: averaging, millivolts, differential, several pins at once, fixed-rate bursts |
| @ref core_advanced | tone(), noTone(), pulseIn(), shiftOut(), shiftIn() |
| @ref core_time | millis(), micros(), delay(), delayMicroseconds() |
| @ref core_interrupts | attachInterrupt(), detachInterrupt(), interrupts(), noInterrupts() |
| @ref core_math | min(), max(), constrain(), map(), sq() |
| @ref core_random | random(), randomSeed() |
| @ref core_bits | bitRead(), bitSet(), lowByte(), highByte(), ... |
| @ref core_sketch | setup(), loop(), yield() |
| @ref core_types | HIGH, LOW, INPUT, OUTPUT, ..., `byte`, `word` |

## Phyduino additions

| Topic | Use | Header |
|---|---|---|
| USB serial | `Serial` (SerialUSBClass) | SerialUSB.h |
| Hardware serial | `Serial1` on D0/D1, `Serial2` on D4/D5 (SerialUARTClass) | SerialUART.h |
| Analog and PWM extras | analogReadResolution(), analogReadAveraging(), analogReadMillivolts(), analogReadDifferentialMillivolts(), analogReadMultiple(), analogReadBurst(), analogStartBurst(), analogWriteResolution(), analogWriteFrequency() | Arduino.h |
| pico-sdk access | digitalPinToGpio(): the RP2040 GPIO behind an Arduino pin | Arduino.h |
| Pin map | `A0`-`A5`, `LED_BUILTIN`, `SS`, `MOSI`, `MISO`, `SCK`, `SDA`, `SCL` | pins_arduino.h |
| SPI | `SPI` (PhyduinoSPI): SCK D13, MOSI D11, MISO D12 | SPI.h |
| I2C | `Wire` (PhyduinoWire): controller or peripheral on SDA/SCL | Wire.h |
| External ADC | max11635: the chip behind A3-A5, by channel (scan, true differential, bursts); sketches normally use the analog functions above | max11635.h |
| USB-PD power | stusb4500: sink profiles and the current contract | stusb4500.h |

None of these need an `#include` except SPI.h, Wire.h, max11635.h and stusb4500.h.

## Examples

The [Examples](examples.html) page lists the example sketches, also under
**File → Examples → Examples for Phyduino Pico** in the Arduino IDE once
Phyduino Pico is selected: **Phyduino** for the analog functions, **SPI** and
**Wire** for those libraries.
