/**
 * @file pwm.cpp
 * @brief analogWrite(), analogWriteResolution(), analogWriteFrequency().
 *
 * Every RP2040 GPIO can output PWM. Two GPIOs share each PWM "slice" (and so
 * its frequency): GPIO n and n^1, e.g. GPIO6/7 = D2/D3.
 */
#include "Arduino.h"
#include "wiring_private.h"
#include "hardware/clocks.h"
#include "hardware/gpio.h"
#include "hardware/pwm.h"

static int write_bits = 8;          // Arduino default: 0-255
static uint32_t write_hz = 1000;

void analogWriteResolution(int bits) {
    if (bits >= 1 && bits <= 16) write_bits = bits;
}

void analogWriteFrequency(uint32_t hz) {
    if (hz > 0) write_hz = hz;
}

uint16_t pwmConfigureSlice(uint slice, uint32_t hz) {
    // Pick the smallest clock divider that lets the 16-bit counter cover one period.
    const uint32_t sys_hz = clock_get_hz(clk_sys);
    uint32_t div = (sys_hz / hz + 65535u) / 65536u;
    if (div < 1)   div = 1;
    if (div > 255) div = 255;   // lowest frequency ~7.5 Hz at 125 MHz
    uint32_t top = sys_hz / (hz * div) - 1u;
    if (top > 0xFFFF) top = 0xFFFF;  // below the lowest frequency: run at the lowest

    pwm_set_clkdiv_int_frac4(slice, static_cast<uint8_t>(div), 0);
    pwm_set_wrap(slice, static_cast<uint16_t>(top));
    pwm_set_enabled(slice, true);
    return static_cast<uint16_t>(top);
}

void analogWrite(pin_size_t pin, int value) {
    const uint8_t gpio = digitalPinToGpio(pin);
    if (gpio == NOT_A_GPIO) return;

    const uint32_t max_value = (1u << write_bits) - 1u;
    if (value < 0) value = 0;
    if (static_cast<uint32_t>(value) > max_value) value = static_cast<int>(max_value);

    const uint slice = pwm_gpio_to_slice_num(gpio);
    const uint32_t top = pwmConfigureSlice(slice, write_hz);
    // level == top + 1 gives a steady high output for the maximum value.
    const uint32_t level = (static_cast<uint32_t>(value) * (top + 1u)) / max_value;

    pwm_set_gpio_level(gpio, static_cast<uint16_t>(level > 0xFFFF ? 0xFFFF : level));
    gpio_set_function(gpio, GPIO_FUNC_PWM);
}
