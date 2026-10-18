/**
 * @file stusb4500.h
 * @brief Driver for the on-board STUSB4500 USB Power Delivery sink controller (U6).
 *
 * The STUSB4500 asks a USB-PD charger for the highest of up to three "sink
 * profiles" (PDOs) that the charger can supply. initVariant() (variant.cpp)
 * loads the board's default profiles at start-up; a sketch can change them:
 *
 * @code
 * #include <stusb4500.h>
 * const stusb4500::Profile profiles[] = {{5000, 2000}, {9000, 1000}};
 * stusb4500::setProfiles(profiles, 2);
 * @endcode
 *
 * Profiles are written to the chip's working registers only, never to its
 * NVM, so they last until the board loses power. Profile 1 must be 5 V (USB
 * rule). Profile 3 has the highest priority, then 2, then 1.
 *
 * VBUS also feeds the header's VIN pin, so VIN follows the contract. The chip
 * negotiates from its NVM at plug-in, before any sketch runs; Phyduino boards
 * have the NVM set to 5 V only (tests/sketches/usbpd_nvm, run once per board).
 *
 * Bus: I2C1, address 0x28 (pins in pins_arduino.h, PHYDUINO_USBPD_*). It is
 * separate from the Wire library's I2C0, so both can be used freely.
 */
#pragma once

#include <stdint.h>

namespace stusb4500 {

/// @addtogroup usbpd
/// @{

/**
 * @brief One sink profile (PDO): a voltage the board accepts and the current it draws.
 */
struct Profile {
    uint16_t millivolts;  ///< 5000-20000, 50 mV steps
    uint16_t milliamps;   ///< 0-5000, 10 mA steps
};

/// Number of profiles the chip holds.
constexpr uint8_t kMaxProfiles = 3;

/**
 * @brief Sets up I2C1 and the control pins.
 * @return true if the chip answers.
 */
bool begin();

/**
 * @brief Loads 1-3 profiles and renegotiates if they differ from the current ones.
 * @param profiles Profiles in priority order, lowest first; the first must be 5 V.
 * @param count Number of profiles, 1 to kMaxProfiles.
 * @return false if the arguments are invalid or the chip does not answer.
 */
bool setProfiles(const Profile* profiles, uint8_t count);

/**
 * @brief Reads the active profiles.
 * @param out Array with room for kMaxProfiles.
 * @return Number of profiles written to `out`.
 */
uint8_t getProfiles(Profile* out);

/**
 * @brief The board's profiles for Tools > USB-PD Power.
 *
 * 5 V/2 A, then 9 V/1 A if max_millivolts >= 9000, then max_millivolts at
 * 10 W (schematic limit) if it is above 9 V.
 * @param max_millivolts Highest voltage to ask for, or 0 for "use the chip's
 *                       stored settings".
 * @param out Array with room for kMaxProfiles.
 * @return Number of profiles written to `out`; 0 for max_millivolts 0.
 */
uint8_t boardProfiles(uint16_t max_millivolts, Profile* out);

/**
 * @brief Asks the charger to negotiate again with the current profiles.
 * @return false if begin() has not run or the chip does not answer.
 */
bool renegotiate();

/**
 * @brief Which charger offer is in use.
 * @return Position 1-7 of the offer; 0 = no PD contract (plain 5 V USB, or
 *         no charger attached).
 */
uint8_t contractPosition();

/**
 * @brief Operating current agreed in the current contract.
 * @return Current in mA; 0 = no contract.
 */
uint16_t contractMilliamps();

/**
 * @brief VBUS measured by the on-board ADC (MAX11635 channel 3).
 * @return Voltage in mV.
 */
uint32_t vbusMillivolts();

/** @} */

}  // namespace stusb4500
