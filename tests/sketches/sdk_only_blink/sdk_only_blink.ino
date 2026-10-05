// Milestone-2 proof: blinks the on-board LED using only pico-sdk calls,
// no Arduino API. Confirms the SDK bundle compiles, links and boots.
#include "hardware/gpio.h"
#include "pico/time.h"

void setup() {
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
}

void loop() {
    gpio_xor_mask(1u << PICO_DEFAULT_LED_PIN);
    sleep_ms(250);
}
