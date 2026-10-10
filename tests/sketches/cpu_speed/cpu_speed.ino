// Tools > CPU Speed check: blinks LED_BUILTIN once per second and, every second,
// prints the clocks the chip is really running at, the core voltage, and a
// timing check (delay(1000) measured with micros(), plus a fixed busy loop).
// Expect: sys == F_CPU, peri == sys, vreg 1.15 V above 133 MHz (else 1.10 V),
// delay ~1000000 us at every speed, loop time scaling as 1/F_CPU.
#include "hardware/clocks.h"
#include "hardware/vreg.h"

static uint32_t busyLoopUs() {
    const uint32_t start = micros();
    for (volatile uint32_t i = 0; i < 1000000; i = i + 1) {
    }
    return micros() - start;
}

void setup() {
    pinMode(LED_BUILTIN, OUTPUT);
    Serial.begin(115200);
}

void loop() {
    const uint32_t start = micros();
    digitalWrite(LED_BUILTIN, HIGH);
    delay(500);
    digitalWrite(LED_BUILTIN, LOW);
    delay(500);
    const uint32_t delayUs = micros() - start;

    Serial.print("F_CPU=");
    Serial.print(F_CPU / 1000000);
    Serial.print(" MHz  sys=");
    Serial.print(clock_get_hz(clk_sys));
    Serial.print("  peri=");
    Serial.print(clock_get_hz(clk_peri));
    Serial.print("  vreg=");
    Serial.print(vreg_get_voltage() == VREG_VOLTAGE_1_15 ? "1.15V"
                 : vreg_get_voltage() == VREG_VOLTAGE_1_10 ? "1.10V" : "other");
    Serial.print("  delay(1000)=");
    Serial.print(delayUs);
    Serial.print(" us  loop=");
    Serial.print(busyLoopUs());
    Serial.println(" us");
}
