/**
 * @file wiring_interrupts.cpp
 * @brief attachInterrupt(), attachInterruptParam(), detachInterrupt().
 *
 * One pico-sdk GPIO IRQ callback dispatches to the per-pin user callbacks.
 * Interrupt numbers are Arduino pin numbers (digitalPinToInterrupt(p) == p).
 */
#include "Arduino.h"
#include "hardware/gpio.h"

struct Handler {
    voidFuncPtrParam callback;
    void* param;
};

static Handler handlers[NUM_BANK0_GPIOS];

// Adapts a plain void(void) callback to the void(void*) slot.
static void callPlain(void* fn) {
    reinterpret_cast<voidFuncPtr>(fn)();
}

static void dispatch(uint gpio, uint32_t) {
    const Handler& h = handlers[gpio];
    if (h.callback) h.callback(h.param);
}

static uint32_t toIrqEvents(PinStatus mode) {
    switch (mode) {
        case LOW:     return GPIO_IRQ_LEVEL_LOW;
        case HIGH:    return GPIO_IRQ_LEVEL_HIGH;
        case FALLING: return GPIO_IRQ_EDGE_FALL;
        case RISING:  return GPIO_IRQ_EDGE_RISE;
        case CHANGE:  return GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE;
    }
    return 0;
}

void attachInterruptParam(pin_size_t pin, voidFuncPtrParam callback, PinStatus mode, void* param) {
    const uint8_t gpio = digitalPinToGpio(pin);
    if (gpio == NOT_A_GPIO || callback == nullptr) return;

    // Disable first so the IRQ never sees a half-written handler.
    gpio_set_irq_enabled(gpio, 0xF, false);
    handlers[gpio] = {callback, param};
    gpio_set_irq_enabled_with_callback(gpio, toIrqEvents(mode), true, dispatch);
}

void attachInterrupt(pin_size_t pin, voidFuncPtr callback, PinStatus mode) {
    if (callback == nullptr) return;  // callPlain would otherwise call address 0
    attachInterruptParam(pin, callPlain, mode, reinterpret_cast<void*>(callback));
}

void detachInterrupt(pin_size_t pin) {
    const uint8_t gpio = digitalPinToGpio(pin);
    if (gpio == NOT_A_GPIO) return;
    gpio_set_irq_enabled(gpio, 0xF, false);
    handlers[gpio] = {nullptr, nullptr};
}
