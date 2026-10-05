/**
 * @file tone.cpp
 * @brief tone(), noTone(): 50 % square wave from the pin's PWM slice.
 *
 * Only one tone plays at a time (Arduino convention). The paired GPIO on the
 * same PWM slice changes frequency too (see pwm.cpp).
 */
#include "Arduino.h"
#include "wiring_private.h"
#include "hardware/gpio.h"
#include "hardware/pwm.h"
#include "pico/time.h"

static uint8_t tone_gpio = NOT_A_GPIO;
static alarm_id_t stop_alarm = 0;

static void stopTone() {
    if (stop_alarm > 0) {
        cancel_alarm(stop_alarm);
        stop_alarm = 0;
    }
    if (tone_gpio != NOT_A_GPIO) {
        pwm_set_gpio_level(tone_gpio, 0);
        gpio_init(tone_gpio);
        tone_gpio = NOT_A_GPIO;
    }
}

static int64_t onToneTimeout(alarm_id_t, void*) {
    stop_alarm = 0;
    stopTone();
    return 0;  // do not repeat
}

void tone(uint8_t pin, unsigned int frequency, unsigned long duration) {
    const uint8_t gpio = digitalPinToGpio(pin);
    if (gpio == NOT_A_GPIO || frequency == 0) return;

    stopTone();
    const uint32_t top = pwmConfigureSlice(pwm_gpio_to_slice_num(gpio), frequency);
    pwm_set_gpio_level(gpio, static_cast<uint16_t>((top + 1u) / 2u));
    gpio_set_function(gpio, GPIO_FUNC_PWM);
    tone_gpio = gpio;

    if (duration > 0) {
        stop_alarm = add_alarm_in_ms(duration, onToneTimeout, nullptr, true);
    }
}

void noTone(uint8_t pin) {
    if (digitalPinToGpio(pin) == tone_gpio) stopTone();
}
