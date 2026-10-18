/**
 * @file variant.cpp
 * @brief Phyduino Pico board start-up and the analog hooks for A3-A5 (MAX11635).
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

// --- Analog hooks: A3-A5 have no RP2040 GPIO; they are MAX11635 channels 0-2 ----------
static bool isExternalAnalog(pin_size_t pin) { return pin >= PIN_A3 && pin <= PIN_A5; }
static uint8_t channelOf(pin_size_t pin) { return static_cast<uint8_t>(pin - PIN_A3); }

uint16_t variantAnalogRead(pin_size_t pin) {
    return isExternalAnalog(pin) ? max11635::read(channelOf(pin)) : 0;
}

uint32_t variantAnalogFullScaleMillivolts(pin_size_t pin) {
    return isExternalAnalog(pin) ? PHYDUINO_EXT_ADC_VREF_MV : 0;
}

void variantAnalogReadAveraging(uint8_t samples) {
    max11635::setAveraging(samples);
}

bool variantAnalogReadMultiple(const pin_size_t* pins, uint8_t count, uint16_t* raw) {
    uint8_t last_channel = 0;
    for (uint8_t i = 0; i < count; i++) {
        if (!isExternalAnalog(pins[i])) return false;
        if (channelOf(pins[i]) > last_channel) last_channel = channelOf(pins[i]);
    }
    // One scan of channels 0..last, so every pin is sampled within a few µs.
    uint16_t channels[max11635::kChannels];
    if (!max11635::scan(last_channel, channels)) return false;
    for (uint8_t i = 0; i < count; i++) raw[i] = channels[channelOf(pins[i])];
    return true;
}

bool variantAnalogReadDifferential(pin_size_t plus, pin_size_t minus, int32_t* millivolts) {
    // The only hardware pair on the header is AIN0/AIN1 = A3/A4.
    int sign;
    if (plus == PIN_A3 && minus == PIN_A4) {
        sign = 1;
    } else if (plus == PIN_A4 && minus == PIN_A3) {
        sign = -1;
    } else {
        return false;
    }
    int16_t code;
    if (!max11635::readDifferential(0, true, &code)) return false;
    if (code <= -2048 || code >= 2047) return false;  // beyond ±1.024 V: let the core read each pin
    *millivolts = sign * static_cast<int32_t>(code) * PHYDUINO_EXT_ADC_VREF_MV / 4096;
    return true;
}

uint32_t variantAnalogStartBurst(pin_size_t pin, uint16_t* buffer, size_t count, uint32_t sample_rate_hz) {
    if (!isExternalAnalog(pin)) return 0;
    return max11635::startBurst(channelOf(pin), buffer, count, sample_rate_hz);
}

bool variantAnalogBurstDone() { return max11635::burstDone(); }

void variantAnalogStopBurst() { max11635::stopBurst(); }
