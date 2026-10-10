/**
 * @file stusb4500.cpp
 * @brief STUSB4500 register access over I2C1.
 *
 * Register addresses are not in the STUSB4500 datasheet; they come from ST's
 * published register map (as used by ST's reference driver). Checked against
 * hardware: see tests/hardware-checklist.md, "USB-PD".
 */
#include "stusb4500.h"

#include "Arduino.h"
#include "hardware/gpio.h"
#include "hardware/i2c.h"
#include "max11635.h"

namespace {

constexpr uint32_t kI2cHz      = 400 * 1000;
constexpr uint32_t kTimeoutUs  = 10 * 1000;
constexpr uint32_t kStartupWaitMs = 50;  // > TLOAD (30 ms, datasheet)

// Registers
constexpr uint8_t kRegPdCommandCtrl = 0x1A;
constexpr uint8_t kRegDeviceId      = 0x2F;
constexpr uint8_t kRegTxHeaderLow   = 0x51;
constexpr uint8_t kRegPdoCount      = 0x70;  // DPM_PDO_NUMB, bits 2:0
constexpr uint8_t kRegSinkPdo1      = 0x85;  // DPM_SNK_PDO1..3, 4 bytes each, little-endian
constexpr uint8_t kRegRdoStatus     = 0x91;  // RDO_REG_STATUS, 4 bytes, little-endian

// Soft reset = "send a Soft_Reset message", which makes the charger renegotiate.
constexpr uint8_t kSoftResetMessage = 0x0D;
constexpr uint8_t kSendCommand      = 0x26;

// Known DEVICE_ID values (chip revisions).
constexpr uint8_t kDeviceIds[] = {0x21, 0x25};

// Fixed-supply sink PDO fields.
constexpr uint32_t kVoltageShift = 10;     // bits 19:10, 50 mV units
constexpr uint32_t kVoltageMask  = 0x3FFu << kVoltageShift;
constexpr uint32_t kCurrentMask  = 0x3FFu; // bits 9:0, 10 mA units

// VBUS divider on MAX11635 AIN3: 100 kOhm over 10.2 kOhm.
constexpr uint32_t kVbusDividerNum = 1102;
constexpr uint32_t kVbusDividerDen = 102;

bool readRegs(uint8_t reg, uint8_t* data, size_t length) {
    if (i2c_write_timeout_us(PHYDUINO_USBPD_I2C, PHYDUINO_USBPD_I2C_ADDR, &reg, 1, true, kTimeoutUs) != 1) {
        return false;
    }
    return i2c_read_timeout_us(PHYDUINO_USBPD_I2C, PHYDUINO_USBPD_I2C_ADDR, data, length, false,
                               kTimeoutUs) == static_cast<int>(length);
}

bool writeRegs(uint8_t reg, const uint8_t* data, size_t length) {
    uint8_t buffer[1 + 4];
    if (length > 4) return false;
    buffer[0] = reg;
    for (size_t i = 0; i < length; i++) buffer[1 + i] = data[i];
    return i2c_write_timeout_us(PHYDUINO_USBPD_I2C, PHYDUINO_USBPD_I2C_ADDR, buffer, length + 1, false,
                                kTimeoutUs) == static_cast<int>(length + 1);
}

bool readU32(uint8_t reg, uint32_t& value) {
    uint8_t b[4];
    if (!readRegs(reg, b, 4)) return false;
    value = b[0] | (b[1] << 8) | (b[2] << 16) | (static_cast<uint32_t>(b[3]) << 24);
    return true;
}

bool writeU32(uint8_t reg, uint32_t value) {
    const uint8_t b[4] = {static_cast<uint8_t>(value), static_cast<uint8_t>(value >> 8),
                          static_cast<uint8_t>(value >> 16), static_cast<uint8_t>(value >> 24)};
    return writeRegs(reg, b, 4);
}

uint8_t pdoRegister(uint8_t index) {  // index 0-2
    return static_cast<uint8_t>(kRegSinkPdo1 + 4 * index);
}

bool started = false;

}  // namespace

namespace stusb4500 {

bool begin() {
    if (!started) {
        i2c_init(PHYDUINO_USBPD_I2C, kI2cHz);  // board has 4.7 kOhm pull-ups
        gpio_set_function(PHYDUINO_USBPD_GPIO_SDA, GPIO_FUNC_I2C);
        gpio_set_function(PHYDUINO_USBPD_GPIO_SCL, GPIO_FUNC_I2C);
        // RESET is active high; hold it low (the board also pulls it down).
        gpio_init(PHYDUINO_USBPD_GPIO_RESET);
        gpio_put(PHYDUINO_USBPD_GPIO_RESET, false);
        gpio_set_dir(PHYDUINO_USBPD_GPIO_RESET, GPIO_OUT);
        gpio_init(PHYDUINO_USBPD_GPIO_ALERT);    // open-drain alert, unused for now
        gpio_pull_up(PHYDUINO_USBPD_GPIO_ALERT);
        started = true;
    }
    // After power-up the chip needs up to 30 ms (TLOAD) to load its registers
    // from NVM before it answers; the RP2040 can get here sooner.
    const uint32_t start = millis();
    do {
        uint8_t id;
        if (readRegs(kRegDeviceId, &id, 1)) {
            for (uint8_t known : kDeviceIds) {
                if (id == known) return true;
            }
            return false;  // something answered, but not an STUSB4500
        }
        delay(2);
    } while (millis() - start < kStartupWaitMs);
    return false;
}

uint8_t getProfiles(Profile* out) {
    uint8_t count;
    if (!started || !readRegs(kRegPdoCount, &count, 1)) return 0;
    count &= 0x07;
    if (count > kMaxProfiles) count = kMaxProfiles;
    for (uint8_t i = 0; i < count; i++) {
        uint32_t pdo;
        if (!readU32(pdoRegister(i), pdo)) return 0;
        out[i].millivolts = static_cast<uint16_t>(((pdo & kVoltageMask) >> kVoltageShift) * 50);
        out[i].milliamps  = static_cast<uint16_t>((pdo & kCurrentMask) * 10);
    }
    return count;
}

bool setProfiles(const Profile* profiles, uint8_t count) {
    if (!started || count < 1 || count > kMaxProfiles || profiles[0].millivolts != 5000) return false;
    for (uint8_t i = 0; i < count; i++) {
        if (profiles[i].millivolts < 5000 || profiles[i].millivolts > 20000 ||
            profiles[i].milliamps > 5000) {
            return false;
        }
    }

    // Nothing to do if the chip already has these profiles (avoids a needless renegotiation).
    Profile current[kMaxProfiles];
    if (getProfiles(current) == count) {
        bool same = true;
        for (uint8_t i = 0; i < count; i++) {
            same = same && current[i].millivolts == profiles[i].millivolts
                        && current[i].milliamps == profiles[i].milliamps;
        }
        if (same) return true;
    }

    for (uint8_t i = 0; i < count; i++) {
        uint32_t pdo;
        if (!readU32(pdoRegister(i), pdo)) return false;
        // Keep the flag bits (31:20); replace voltage and current.
        pdo &= ~(kVoltageMask | kCurrentMask);
        pdo |= (static_cast<uint32_t>(profiles[i].millivolts / 50) << kVoltageShift) & kVoltageMask;
        pdo |= static_cast<uint32_t>(profiles[i].milliamps / 10) & kCurrentMask;
        if (!writeU32(pdoRegister(i), pdo)) return false;
    }
    if (!writeRegs(kRegPdoCount, &count, 1)) return false;
    return renegotiate();
}

uint8_t boardProfiles(uint16_t max_millivolts, Profile* out) {
    constexpr uint32_t kBoardMaxMilliwatts = 10000;
    if (max_millivolts == 0) return 0;
    uint8_t count = 0;
    out[count++] = {5000, 2000};
    if (max_millivolts >= 9000) out[count++] = {9000, 1000};
    if (max_millivolts > 9000) {
        const uint32_t ma = kBoardMaxMilliwatts * 1000 / max_millivolts / 10 * 10;  // 10 mA steps
        out[count++] = {max_millivolts, static_cast<uint16_t>(ma)};
    }
    return count;
}

bool renegotiate() {
    if (!started) return false;
    return writeRegs(kRegTxHeaderLow, &kSoftResetMessage, 1) &&
           writeRegs(kRegPdCommandCtrl, &kSendCommand, 1);
}

uint8_t contractPosition() {
    uint32_t rdo;
    if (!started || !readU32(kRegRdoStatus, rdo)) return 0;
    return static_cast<uint8_t>((rdo >> 28) & 0x07);
}

uint16_t contractMilliamps() {
    uint32_t rdo;
    if (!started || !readU32(kRegRdoStatus, rdo) || ((rdo >> 28) & 0x07) == 0) return 0;
    return static_cast<uint16_t>(((rdo >> 10) & 0x3FF) * 10);  // operating current, 10 mA units
}

uint32_t vbusMillivolts() {
    const uint32_t raw = max11635::read(PHYDUINO_EXT_ADC_VBUS_CH);
    const uint32_t adc_mv = raw * PHYDUINO_EXT_ADC_VREF_MV / 4096u;
    return adc_mv * kVbusDividerNum / kVbusDividerDen;
}

}  // namespace stusb4500
