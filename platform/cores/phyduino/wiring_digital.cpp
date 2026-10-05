/**
 * @file wiring_digital.cpp
 * @brief pinMode(), digitalWrite(), digitalRead().
 *
 * For RP2040-specific pad settings (drive strength, slew rate, Schmitt
 * trigger) call the pico-sdk directly, e.g.
 *   gpio_set_drive_strength(digitalPinToGpio(pin), GPIO_DRIVE_STRENGTH_12MA);
 */
#include "Arduino.h"
#include "hardware/gpio.h"

// Last mode set per pin; digitalWrite() needs it for INPUT and OUTPUT_OPENDRAIN.
static PinMode pin_modes[PINS_COUNT];

void pinMode(pin_size_t pin, PinMode mode) {
    const uint8_t gpio = digitalPinToGpio(pin);
    if (gpio == NOT_A_GPIO) return;

    gpio_init(gpio);  // back to plain SIO GPIO, input, output latch low
    pin_modes[pin] = mode;

    switch (mode) {
        case INPUT:            gpio_disable_pulls(gpio);        break;
        case INPUT_PULLUP:     gpio_pull_up(gpio);              break;
        case INPUT_PULLDOWN:   gpio_pull_down(gpio);            break;
        case OUTPUT:           gpio_set_dir(gpio, GPIO_OUT);    break;
        // Open drain: latch stays low; "HIGH" = released (input), "LOW" = driven low.
        case OUTPUT_OPENDRAIN: gpio_disable_pulls(gpio);        break;
    }
}

void digitalWrite(pin_size_t pin, PinStatus value) {
    const uint8_t gpio = digitalPinToGpio(pin);
    if (gpio == NOT_A_GPIO) return;

    switch (pin_modes[pin]) {
        case OUTPUT_OPENDRAIN:
            gpio_set_dir(gpio, value == LOW ? GPIO_OUT : GPIO_IN);
            break;
        case INPUT:  // Arduino convention: writing HIGH to an input enables the pull-up
            if (value == LOW) gpio_disable_pulls(gpio);
            else              gpio_pull_up(gpio);
            break;
        default:
            gpio_put(gpio, value != LOW);
            break;
    }
}

PinStatus digitalRead(pin_size_t pin) {
    const uint8_t gpio = digitalPinToGpio(pin);
    if (gpio == NOT_A_GPIO) return LOW;
    return gpio_get(gpio) ? HIGH : LOW;
}
