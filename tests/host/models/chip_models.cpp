/**
 * @file chip_models.cpp
 * @brief MAX11635 and STUSB4500 models (see chip_models.h).
 */
#include "chip_models.h"

#include "fake_sdk.h"

namespace {
constexpr unsigned kAdcCs = 21, kAdcCnvst = 18, kAdcEoc = 19;
constexpr uint8_t kPdAddress = 0x28;
}

// --- MAX11635 -------------------------------------------------------------------------------
void Max11635Model::attach() {
    fake::hw.gpio[kAdcEoc].input_level = true;  // EOC idles high
    fake::hw.spi[0].device = [this](uint8_t out) { return onByte(out); };
    fake::hw.on_gpio_put = [this](unsigned gpio, bool value) { onGpioPut(gpio, value); };
}

uint8_t Max11635Model::onByte(uint8_t out) {
    if (fake::hw.gpio[kAdcCs].out_level) return 0;  // not selected

    if (result_bytes_left_ > 0) {                   // clocking out a result
        result_bytes_left_--;
        const uint8_t b = result_bytes_left_ == 1 ? static_cast<uint8_t>(result_ >> 8)
                                                  : static_cast<uint8_t>(result_);
        if (result_bytes_left_ == 0) fake::hw.gpio[kAdcEoc].input_level = true;
        return b;
    }
    if (expect_unipolar_byte_) {
        expect_unipolar_byte_ = false;
        unipolar_register = out;
        return 0;
    }
    commands.push_back(out);
    if (out & 0x80) {                                // conversion register
        channel_ = (out >> 3) & 0x0F;
    } else if ((out & 0xC0) == 0x40) {               // setup register
        setup_register = out;
        expect_unipolar_byte_ = (out & 0x03) == 0x02;
    } else if ((out & 0xF0) == 0x10) {               // reset register
        if ((out & 0x08) == 0) resets++;
    }
    return 0;
}

void Max11635Model::onGpioPut(unsigned gpio, bool value) {
    if (gpio != kAdcCnvst) return;
    if (!value) {
        cnvst_low_ = true;
    } else if (cnvst_low_) {                         // rising edge ends the CNVST pulse
        cnvst_low_ = false;
        if (!respond) return;
        result_ = channel_ < 4 ? (channel_value[channel_] & 0x0FFF) : 0;
        result_bytes_left_ = 2;
        fake::hw.gpio[kAdcEoc].input_level = false;  // conversion done
    }
}

// --- STUSB4500 ------------------------------------------------------------------------------
void Stusb4500Model::attach() {
    regs[0x2F] = 0x21;    // DEVICE_ID
    regs[0x70] = 3;       // DPM_PDO_NUMB
    setPdo(0, (0x26u << 24) | (100u << 10) | 150u);          // flag bits 0x26, 5 V, 1.5 A
    setPdo(1, (300u << 10) | 150u);                          // 15 V, 1.5 A
    setPdo(2, (400u << 10) | 100u);                          // 20 V, 1.0 A

    auto& bus = fake::hw.i2c[1];
    bus.on_write = [this](uint8_t addr, const uint8_t* data, size_t len, bool) -> int {
        if (!present || addr != kPdAddress) return PICO_ERROR_GENERIC;
        if (len == 0) return 0;
        pointer_ = data[0];
        if (len > 1) register_writes++;
        for (size_t i = 1; i < len; i++) {
            const uint8_t reg = pointer_++;
            regs[reg] = data[i];
            if (reg == 0x1A && data[i] == 0x26 && regs[0x51] == 0x0D) soft_resets++;
        }
        return static_cast<int>(len);
    };
    bus.on_read = [this](uint8_t addr, uint8_t* data, size_t len, bool) -> int {
        if (!present || addr != kPdAddress) return PICO_ERROR_GENERIC;
        for (size_t i = 0; i < len; i++) data[i] = regs[pointer_++];
        return static_cast<int>(len);
    };
}

uint32_t Stusb4500Model::pdo(int index) const {
    const int base = 0x85 + 4 * index;
    return regs[base] | (regs[base + 1] << 8) | (regs[base + 2] << 16) |
           (static_cast<uint32_t>(regs[base + 3]) << 24);
}

void Stusb4500Model::setPdo(int index, uint32_t value) {
    const int base = 0x85 + 4 * index;
    for (int i = 0; i < 4; i++) regs[base + i] = static_cast<uint8_t>(value >> (8 * i));
}

void Stusb4500Model::setContract(uint8_t position, uint16_t operating_10ma) {
    const uint32_t rdo = (static_cast<uint32_t>(position & 7) << 28) |
                         (static_cast<uint32_t>(operating_10ma & 0x3FF) << 10);
    for (int i = 0; i < 4; i++) regs[0x91 + i] = static_cast<uint8_t>(rdo >> (8 * i));
}
