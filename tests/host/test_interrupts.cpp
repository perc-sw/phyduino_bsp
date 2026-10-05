/**
 * @file test_interrupts.cpp
 * @brief attachInterrupt(), attachInterruptParam(), detachInterrupt()
 *        (cores/phyduino/wiring_interrupts.cpp).
 */
#include "test_support.h"

namespace {
int calls = 0;
void onEvent() { calls++; }
void onEventParam(void* p) { (*static_cast<int*>(p)) += 10; }
}  // namespace

TEST_CASE_FIXTURE(HostTest, "modes map to the matching GPIO IRQ events") {
    const uint g = digitalPinToGpio(2);
    attachInterrupt(digitalPinToInterrupt(2), onEvent, FALLING);
    CHECK(fake::hw.gpio[g].irq_events == GPIO_IRQ_EDGE_FALL);
    attachInterrupt(2, onEvent, RISING);
    CHECK(fake::hw.gpio[g].irq_events == GPIO_IRQ_EDGE_RISE);  // re-attach replaces, not adds
    attachInterrupt(2, onEvent, CHANGE);
    CHECK(fake::hw.gpio[g].irq_events == (GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE));
    attachInterrupt(2, onEvent, LOW);
    CHECK(fake::hw.gpio[g].irq_events == GPIO_IRQ_LEVEL_LOW);
    attachInterrupt(2, onEvent, HIGH);
    CHECK(fake::hw.gpio[g].irq_events == GPIO_IRQ_LEVEL_HIGH);
    detachInterrupt(2);
}

TEST_CASE_FIXTURE(HostTest, "the callback runs only for enabled events, until detached") {
    calls = 0;
    const uint g = digitalPinToGpio(2);
    attachInterrupt(2, onEvent, FALLING);

    fake::trigger_gpio_irq(g, GPIO_IRQ_EDGE_FALL);
    CHECK(calls == 1);
    fake::trigger_gpio_irq(g, GPIO_IRQ_EDGE_RISE);
    CHECK(calls == 1);

    detachInterrupt(2);
    CHECK(fake::hw.gpio[g].irq_events == 0);
    fake::trigger_gpio_irq(g, GPIO_IRQ_EDGE_FALL);
    CHECK(calls == 1);
}

TEST_CASE_FIXTURE(HostTest, "each pin has its own handler; attachInterruptParam passes its argument") {
    calls = 0;
    int value = 0;
    attachInterrupt(2, onEvent, RISING);
    attachInterruptParam(3, onEventParam, RISING, &value);

    fake::trigger_gpio_irq(digitalPinToGpio(3), GPIO_IRQ_EDGE_RISE);
    CHECK(value == 10);
    CHECK(calls == 0);
    fake::trigger_gpio_irq(digitalPinToGpio(2), GPIO_IRQ_EDGE_RISE);
    CHECK(calls == 1);
    CHECK(value == 10);

    detachInterrupt(2);
    detachInterrupt(3);
}

TEST_CASE_FIXTURE(HostTest, "pins without a GPIO and null callbacks are ignored") {
    attachInterrupt(A3, onEvent, FALLING);
    attachInterrupt(2, nullptr, FALLING);
    for (const auto& g : fake::hw.gpio) CHECK(g.irq_events == 0);
}
