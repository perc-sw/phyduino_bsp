/**
 * @file stusb4500.h
 * @brief Driver for the on-board STUSB4500 USB Power Delivery sink controller (U6).
 *
 * The STUSB4500 asks a USB-PD charger for the highest of up to three "sink
 * profiles" (PDOs) that the charger can supply. initVariant() (variant.cpp)
 * loads the board's default profiles at start-up; a sketch can change them:
 *
 *   #include <stusb4500.h>
 *   const stusb4500::Profile profiles[] = {{5000, 2000}, {9000, 1000}};
 *   stusb4500::setProfiles(profiles, 2);
 *
 * Profiles are written to the chip's working registers only, never to its
 * NVM, so they last until the board loses power. Profile 1 must be 5 V (USB
 * rule). Profile 3 has the highest priority, then 2, then 1.
 *
 * Bus: I2C1, address 0x28 (pins in pins_arduino.h, PHYDUINO_USBPD_*). It is
 * separate from the Wire library's I2C0, so both can be used freely.
 */
#pragma once

#include <stdint.h>

namespace stusb4500 {

struct Profile {
    uint16_t millivolts;  // 5000-20000, 50 mV steps
    uint16_t milliamps;   // 0-5000, 10 mA steps
};

constexpr uint8_t kMaxProfiles = 3;

/// Sets up I2C1 and the control pins. Returns true if the chip answers.
bool begin();

/// Loads 1-3 profiles and renegotiates if they differ from the current ones.
/// Returns false if the arguments are invalid or the chip does not answer.
bool setProfiles(const Profile* profiles, uint8_t count);

/// Copies the active profiles into `out` (room for kMaxProfiles); returns the count.
uint8_t getProfiles(Profile* out);

/// Asks the charger to negotiate again with the current profiles.
bool renegotiate();

/// Position (1-7) of the charger offer currently in use; 0 = no PD contract
/// (plain 5 V USB, or no charger attached).
uint8_t contractPosition();

/// Operating current agreed in the current contract, in mA (0 = no contract).
uint16_t contractMilliamps();

/// VBUS measured by the on-board ADC (MAX11635 channel 3), in mV.
uint32_t vbusMillivolts();

}  // namespace stusb4500
