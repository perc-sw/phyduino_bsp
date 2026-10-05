/**
 * @file variant.cpp
 * @brief Phyduino Pico board start-up and the analog hook for A3-A5.
 */
#include "Arduino.h"
#include "max11635.h"
#include "stusb4500.h"

// Board USB-PD profiles (schematic: 10 W max). The STUSB4500 picks the highest
// one the charger offers; POK2/POK3 LEDs show which (2: 9 V, 3: 20 V).
static const stusb4500::Profile kDefaultPowerProfiles[] = {
    {5000, 2000},   // 1: 5.0 V, 2.0 A
    {9000, 1000},   // 2: 9.0 V, 1.0 A
    {20000, 500},   // 3: 20.0 V, 0.5 A
};

void initVariant() {
    max11635::begin();
    // Only renegotiates if the chip's profiles differ; does nothing if it is absent.
    if (stusb4500::begin()) {
        stusb4500::setProfiles(kDefaultPowerProfiles, 3);
    }
}

// A3-A5 have no RP2040 GPIO; they are MAX11635 channels 0-2.
uint16_t variantAnalogRead(pin_size_t pin) {
    if (pin < PIN_A3 || pin > PIN_A5) return 0;
    return max11635::read(static_cast<uint8_t>(pin - PIN_A3));
}
