/**
 * @file main.cpp
 * @brief Program entry: the start-up order for every sketch.
 *
 *   pico-sdk runtime (crt0, clocks)
 *     -> main()
 *        -> init()         core: ADC, USB
 *        -> initVariant()  board: on-board peripherals (variant.cpp)
 *        -> setup()        sketch
 *        -> loop()         sketch, forever
 */
#include "Arduino.h"
#include "hardware/adc.h"

void init() {
    adc_init();
    serialUsbInitStack();  // USB enumerates even if the sketch never uses Serial
}

// initVariant() is defined by every variant (variants/<board>/variant.cpp). No
// weak default here on purpose: the core is linked from an archive, where a weak
// default could silently win over the variant's version.

// Called by delay() and while waiting on Serial; a sketch may override it.
void yield() __attribute__((weak));
void yield() {}

int main() {
    init();
    initVariant();
    setup();
    for (;;) {
        loop();
    }
}
