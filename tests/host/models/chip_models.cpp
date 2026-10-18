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

uint16_t Max11635Model::convertChannel(uint8_t channel) {
    auto value = [this](uint8_t ch) -> int { return sample ? sample(ch) : channel_value[ch]; };
    if (channel > 3) return 0;
    const uint8_t pair_bit = static_cast<uint8_t>(0x80 >> (channel / 2));
    if (channel % 2 == 0 && (unipolar_register & pair_bit)) {          // unipolar wins
        const int diff = value(channel) - value(channel + 1);
        return static_cast<uint16_t>(diff < 0 ? 0 : diff > 4095 ? 4095 : diff);
    }
    if (channel % 2 == 0 && (bipolar_register & pair_bit)) {           // two's complement
        int diff = value(channel) - value(channel + 1);
        diff = diff < -2048 ? -2048 : diff > 2047 ? 2047 : diff;
        return static_cast<uint16_t>(diff & 0x0FFF);
    }
    return static_cast<uint16_t>(value(channel) & 0x0FFF);
}

void Max11635Model::pushResult(uint16_t result) {
    out_.push_back(static_cast<uint8_t>(result >> 8));
    out_.push_back(static_cast<uint8_t>(result));
    conversions++;
}

uint8_t Max11635Model::onByte(uint8_t out) {
    if (fake::hw.gpio[kAdcCs].out_level) return 0;  // not selected

    // DOUT shifts out the oldest waiting byte while DIN shifts in `out`.
    uint8_t reply = 0;
    if (!out_.empty()) {
        reply = out_.front();
        out_.pop_front();
        if (out_.empty() && !clockMode11()) fake::hw.gpio[kAdcEoc].input_level = true;
    }

    if (expect_ != Expect::Command) {                // data byte after a setup byte
        (expect_ == Expect::Unipolar ? unipolar_register : bipolar_register) = out;
        expect_ = Expect::Command;
        return reply;
    }
    if (out == 0) return reply;                      // read clocks, not a command

    commands.push_back(out);
    if (out & 0x80) {                                // conversion register
        channel_ = (out >> 3) & 0x07;
        scan_ = (out >> 1) & 0x03;
        if (clockMode11() && respond) pushResult(convertChannel(channel_));
    } else if ((out & 0xC0) == 0x40) {               // setup register
        setup_register = out;
        if ((out & 0x03) == 0x02) expect_ = Expect::Unipolar;
        if ((out & 0x03) == 0x03) expect_ = Expect::Bipolar;
    } else if ((out & 0xE0) == 0x20) {               // averaging register
        averaging_register = out;
    } else if ((out & 0xF0) == 0x10) {               // reset register
        out_.clear();
        if (out & 0x08) fifo_resets++;
        else            resets++;
    }
    return reply;
}

void Max11635Model::onGpioPut(unsigned gpio, bool value) {
    if (gpio != kAdcCnvst) return;
    if (!value) {
        cnvst_low_ = true;
    } else if (cnvst_low_) {                         // rising edge ends the CNVST pulse
        cnvst_low_ = false;
        cnvst_pulses++;
        if (!respond || clockMode11()) return;
        if (scan_ == 0) {                            // channels 0..N
            for (uint8_t ch = 0; ch <= channel_; ch++) pushResult(convertChannel(ch));
        } else if (scan_ == 1) {                     // channels N..3
            for (uint8_t ch = channel_; ch <= 3; ch++) pushResult(convertChannel(ch));
        } else {                                     // N once (scan 11; 10 not modelled)
            pushResult(convertChannel(channel_));
        }
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
