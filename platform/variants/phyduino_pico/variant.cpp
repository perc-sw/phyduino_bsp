/**
 * @file variant.cpp
 * @brief Phyduino Pico board start-up and the analog hook for A3-A5.
 */
#include "Arduino.h"
#include "max11635.h"
#include "stusb4500.h"

// Highest USB-PD voltage to ask for, from Tools > USB-PD Power (boards.txt);
// 0 = leave the chip on its NVM settings. VIN follows the contract.
#ifndef PHYDUINO_USBPD_MAX_MV
#define PHYDUINO_USBPD_MAX_MV 5000
#endif

void initVariant() {
    max11635::begin();
    // Working registers only, never the NVM. Only renegotiates if the chip's
    // profiles differ; does nothing if the chip is absent.
    stusb4500::Profile profiles[stusb4500::kMaxProfiles];
    const uint8_t count = stusb4500::boardProfiles(PHYDUINO_USBPD_MAX_MV, profiles);
    if (stusb4500::begin() && count > 0) {
        stusb4500::setProfiles(profiles, count);
    }
}

// A3-A5 have no RP2040 GPIO; they are MAX11635 channels 0-2.
uint16_t variantAnalogRead(pin_size_t pin) {
    if (pin < PIN_A3 || pin > PIN_A5) return 0;
    return max11635::read(static_cast<uint8_t>(pin - PIN_A3));
}
