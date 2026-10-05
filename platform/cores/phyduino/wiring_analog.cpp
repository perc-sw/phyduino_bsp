/**
 * @file wiring_analog.cpp
 * @brief analogRead(), analogReadResolution(), analogReference().
 *
 * Pins wired to the RP2040 ADC (GPIO26-29) are read here. Analog pins with no
 * GPIO behind them (Phyduino Pico A3-A5) go to the board's variantAnalogRead().
 * Both sources are 12-bit and rescaled to the requested resolution.
 * analogWrite() (PWM) is in pwm.cpp.
 */
#include "Arduino.h"
#include "wiring_private.h"
#include "hardware/adc.h"

static constexpr int kAdcBits = 12;
static constexpr uint kFirstAdcGpio = 26;
static constexpr uint kLastAdcGpio  = 29;

static int read_bits = 10;  // Arduino default: 0-1023

void analogReadResolution(int bits) {
    if (bits >= 1 && bits <= kAdcBits) read_bits = bits;
}

// Fixed references on Phyduino boards: 3.3 V internal ADC, 2.048 V external ADC.
void analogReference(uint8_t) {}

int analogRead(pin_size_t pin) {
    const uint8_t gpio = digitalPinToGpio(pin);
    uint16_t raw;

    if (gpio >= kFirstAdcGpio && gpio <= kLastAdcGpio) {
        adc_gpio_init(gpio);
        adc_select_input(gpio - kFirstAdcGpio);
        raw = adc_read();
    } else if (gpio == NOT_A_GPIO && pin < PINS_COUNT) {
        raw = variantAnalogRead(pin);
    } else {
        return 0;  // not an analog pin
    }
    return static_cast<int>(rescaleBits(raw, kAdcBits, read_bits));
}
