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
| @ref core_analog | analogRead(), analogWrite(), analogReference(), resolution and PWM frequency; on every analog pin: averaging, millivolts, differential, several pins at once, fixed-rate bursts |
| @ref core_advanced | tone(), noTone(), pulseIn(), pulseInLong(), shiftOut(), shiftIn() |
| @ref core_time | millis(), micros(), delay(), delayMicroseconds() |
| @ref core_interrupts | attachInterrupt(), attachInterruptParam(), detachInterrupt(), digitalPinToInterrupt(), interrupts(), noInterrupts() |
| @ref core_math | min(), max(), constrain(), map(), sq(), radians(), degrees(), `PI`, `HALF_PI`, `TWO_PI`, `DEG_TO_RAD`, `RAD_TO_DEG`, `EULER` |
| @ref core_random | random(), randomSeed() |
| @ref core_bits | bit(), bitRead(), bitWrite(), bitSet(), bitClear(), bitToggle(), lowByte(), highByte(), makeWord(), word() |
| @ref core_sketch | setup(), loop(), yield(); start-up: init(), initVariant() |
| @ref core_multicore | setup1(), loop1(); `rp2040`: rp2040.fifo (push(), push_nb(), pop(), pop_nb(), available()), idleOtherCore(), resumeOtherCore(), restartCore1(), cpuid(), f_cpu(), getTotalHeap(), getUsedHeap(), getFreeHeap() |
| @ref core_freertos | Tools > Operating System > FreeRTOS (SMP): the FreeRTOS kernel (tasks, queues, semaphores, mutexes, event groups, software timers, stream and message buffers); its settings, and what changes in delay() and yield() |
| @ref core_cmsis_rtos2 | Tools > Operating System > FreeRTOS (SMP) + CMSIS-RTOS2: ARM's CMSIS-RTOS2 API on FreeRTOS (threads, thread flags, timers, event flags, mutexes, semaphores, memory pools, message queues); how it differs from other systems, the functions provided and those not provided |
| @ref core_types | `HIGH`, `LOW`, `CHANGE`, `FALLING`, `RISING` (PinStatus); `INPUT`, `OUTPUT`, `INPUT_PULLUP`, `INPUT_PULLDOWN`, `OUTPUT_OPENDRAIN` (PinMode); `LSBFIRST`, `MSBFIRST` (BitOrder); `boolean`, `byte`, `word`, `pin_size_t`, `voidFuncPtr`, `voidFuncPtrParam` |

## Phyduino additions

| Topic | Use | Header |
|---|---|---|
| USB serial | `Serial` (SerialUSBClass) | SerialUSB.h |
| Hardware serial | `Serial1` on D0/D1, `Serial2` on D4/D5 (SerialUARTClass) | SerialUART.h |
| Multicore | setup1(), loop1(), `rp2040` (RP2040Class, with `rp2040.fifo`, RP2040FifoClass) | RP2040.h |
| FreeRTOS | With Tools > Operating System > FreeRTOS (SMP): the FreeRTOS kernel | FreeRTOS.h, then task.h, queue.h, semphr.h, event_groups.h, timers.h, stream_buffer.h, message_buffer.h |
| CMSIS-RTOS2 | With Tools > Operating System > FreeRTOS (SMP) + CMSIS-RTOS2: ARM's CMSIS-RTOS2 API | cmsis_os2.h |
| Analog and PWM extras | analogReadResolution(), analogReadAveraging(), analogReadMillivolts(), analogFullScaleMillivolts(), analogReadDifferentialMillivolts(), analogReadMultiple(), analogReadBurst(), analogStartBurst(), analogBurstDone(), analogStopBurst(), analogWriteResolution(), analogWriteFrequency() | Arduino.h |
| pico-sdk access | digitalPinToGpio(): the RP2040 GPIO behind an Arduino pin | Arduino.h |
| Pin map | `A0`-`A5`, `LED_BUILTIN`, `SS`, `MOSI`, `MISO`, `SCK`, `SDA`, `SCL` | pins_arduino.h |
| SPI | `SPI` (PhyduinoSPI): SCK D13, MOSI D11, MISO D12 | SPI.h |
| I2C | `Wire` (PhyduinoWire): controller or peripheral on SDA/SCL | Wire.h |
| External ADC | max11635: the chip behind A3-A5, by channel (scan, true differential, bursts); sketches normally use the analog functions above | max11635.h |
| USB-PD power | stusb4500: sink profiles and the current contract | stusb4500.h |

None of these need an `#include` except SPI.h, Wire.h, max11635.h, stusb4500.h,
the FreeRTOS headers and cmsis_os2.h.

## Examples

The @ref example_sketches page lists the example sketches, also under
**File → Examples → Examples for Phyduino Pico** in the Arduino IDE once
Phyduino Pico is selected: **Phyduino** for the analog functions, USB-PD,
the second core, FreeRTOS and CMSIS-RTOS2, **SPI** and **Wire** for those libraries.
