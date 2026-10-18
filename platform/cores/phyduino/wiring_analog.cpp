/**
 * @file wiring_analog.cpp
 * @brief analogRead() and the other single-shot analog functions:
 *        analogReadResolution(), analogReadAveraging(), analogReadMillivolts(),
 *        analogReadMultiple(), analogReadDifferentialMillivolts(), analogReference().
 *
 * Pins wired to the RP2040 ADC (GPIO26-29) are read here. Analog pins with no
 * GPIO behind them (Phyduino Pico A3-A5) go to the board's variantAnalog*()
 * hooks. Both sources are 12-bit and rescaled to the requested resolution.
 * Bursts are in analog_burst.cpp; analogWrite() (PWM) is in pwm.cpp.
 */
#include "Arduino.h"
#include "wiring_private.h"
#include "hardware/adc.h"

static constexpr int kAdcBits = 12;
static constexpr uint32_t kAdcCodes = 1u << kAdcBits;
static constexpr uint kFirstAdcGpio = 26;
static constexpr uint kLastAdcGpio  = 29;
static constexpr uint8_t kVariantBatch = 16;  // variant pins per variantAnalogReadMultiple() call

static int read_bits = 10;     // Arduino default: 0-1023
static uint8_t averaging = 1;  // samples per result

AnalogSource analogSourceOf(pin_size_t pin) {
    const uint8_t gpio = digitalPinToGpio(pin);
    if (gpio >= kFirstAdcGpio && gpio <= kLastAdcGpio) return AnalogSource::Rp2040;
    if (gpio == NOT_A_GPIO && pin < PINS_COUNT && variantAnalogFullScaleMillivolts(pin) > 0) {
        return AnalogSource::Variant;
    }
    return AnalogSource::None;
}

int analogReadBits() { return read_bits; }

void analogReadResolution(int bits) {
    if (bits >= 1 && bits <= kAdcBits) read_bits = bits;
}

void analogReadAveraging(uint8_t samples) {
    // The external ADC only averages 4, 8, 16 or 32 samples; keep both ADCs alike.
    averaging = samples >= 32 ? 32 : samples >= 16 ? 16 : samples >= 8 ? 8 : samples >= 4 ? 4 : 1;
    variantAnalogReadAveraging(averaging);
}

// Fixed references on Phyduino boards: 3.3 V internal ADC, 2.048 V external ADC.
void analogReference(uint8_t) {}

// Connects `gpio` to the RP2040 ADC and selects it.
static void selectRp2040(uint8_t gpio) {
    adc_gpio_init(gpio);
    adc_select_input(gpio - kFirstAdcGpio);
}

// Averaged 12-bit reading of an RP2040 ADC pin.
static uint16_t readRp2040(uint8_t gpio) {
    analogWaitForBurst(AnalogSource::Rp2040);
    selectRp2040(gpio);
    uint32_t sum = 0;
    for (uint8_t i = 0; i < averaging; i++) sum += adc_read();
    return static_cast<uint16_t>((sum + averaging / 2) / averaging);
}

// Averaged 12-bit reading of any analog pin; false if the pin is not analog.
static bool readRaw(pin_size_t pin, uint16_t* raw) {
    switch (analogSourceOf(pin)) {
        case AnalogSource::Rp2040:
            *raw = readRp2040(digitalPinToGpio(pin));
            return true;
        case AnalogSource::Variant:
            analogWaitForBurst(AnalogSource::Variant);
            *raw = variantAnalogRead(pin);
            return true;
        default:
            *raw = 0;
            return false;
    }
}

static uint16_t toReadBits(uint16_t raw) {
    return static_cast<uint16_t>(rescaleBits(raw, kAdcBits, read_bits));
}

int analogRead(pin_size_t pin) {
    uint16_t raw;
    readRaw(pin, &raw);
    return toReadBits(raw);
}

uint32_t analogFullScaleMillivolts(pin_size_t pin) {
    switch (analogSourceOf(pin)) {
        case AnalogSource::Rp2040:  return PHYDUINO_ADC_VREF_MV;
        case AnalogSource::Variant: return variantAnalogFullScaleMillivolts(pin);
        default:                    return 0;
    }
}

static uint32_t toMillivolts(pin_size_t pin, uint32_t raw) {
    return raw * analogFullScaleMillivolts(pin) / kAdcCodes;
}

uint32_t analogReadMillivolts(pin_size_t pin) {
    uint16_t raw;
    return readRaw(pin, &raw) ? toMillivolts(pin, raw) : 0;
}

int32_t analogReadDifferentialMillivolts(pin_size_t plus, pin_size_t minus) {
    const AnalogSource plus_source = analogSourceOf(plus);
    const AnalogSource minus_source = analogSourceOf(minus);
    if (plus_source == AnalogSource::None || minus_source == AnalogSource::None) return 0;

    if (plus_source == AnalogSource::Variant && minus_source == AnalogSource::Variant) {
        analogWaitForBurst(AnalogSource::Variant);
        int32_t millivolts;
        if (variantAnalogReadDifferential(plus, minus, &millivolts)) return millivolts;
    }

    if (plus_source == AnalogSource::Rp2040 && minus_source == AnalogSource::Rp2040) {
        // Alternate the two pins so both averages are centred on the same moment.
        analogWaitForBurst(AnalogSource::Rp2040);
        const uint8_t plus_gpio = digitalPinToGpio(plus);
        const uint8_t minus_gpio = digitalPinToGpio(minus);
        int32_t sum = 0;
        for (uint8_t i = 0; i < averaging; i++) {
            selectRp2040(plus_gpio);
            sum += adc_read();
            selectRp2040(minus_gpio);
            sum -= adc_read();
        }
        return sum * static_cast<int32_t>(PHYDUINO_ADC_VREF_MV) /
               static_cast<int32_t>(kAdcCodes * averaging);
    }

    // Two separate readings (different ADCs, or out of the hardware pair's range).
    return static_cast<int32_t>(analogReadMillivolts(plus)) -
           static_cast<int32_t>(analogReadMillivolts(minus));
}

// Reads the variant pins collected so far in one go and stores their results.
static bool flushVariantPins(const pin_size_t* pins, const uint8_t* slots, uint8_t count,
                             uint16_t* results) {
    if (count == 0) return true;
    uint16_t raw[kVariantBatch];
    analogWaitForBurst(AnalogSource::Variant);
    const bool read = variantAnalogReadMultiple(pins, count, raw);
    for (uint8_t i = 0; i < count; i++) results[slots[i]] = read ? toReadBits(raw[i]) : 0;
    return read;
}

bool analogReadMultiple(const pin_size_t* pins, uint8_t count, uint16_t* results) {
    bool ok = true;
    pin_size_t variant_pins[kVariantBatch];
    uint8_t variant_slots[kVariantBatch];
    uint8_t variant_count = 0;

    // RP2040 pins back to back; the variant's pins in batches of up to 16, each in one go.
    for (uint8_t i = 0; i < count; i++) {
        switch (analogSourceOf(pins[i])) {
            case AnalogSource::Rp2040:
                results[i] = toReadBits(readRp2040(digitalPinToGpio(pins[i])));
                break;
            case AnalogSource::Variant:
                variant_pins[variant_count] = pins[i];
                variant_slots[variant_count++] = i;
                if (variant_count == kVariantBatch) {
                    ok = flushVariantPins(variant_pins, variant_slots, variant_count, results) && ok;
                    variant_count = 0;
                }
                break;
            default:
                results[i] = 0;
                ok = false;
                break;
        }
    }
    return flushVariantPins(variant_pins, variant_slots, variant_count, results) && ok;
}
