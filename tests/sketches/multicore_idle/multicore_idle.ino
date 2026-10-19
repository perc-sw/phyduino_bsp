// rp2040.idleOtherCore() / resumeOtherCore() and the inter-core FIFO, both
// directions. Works with Tools > Operating System None or FreeRTOS.
// Core 1 counts as fast as it can; core 0 stops it for 100 ms, checks the
// count did not move, lets it go and checks it moves again. Prints PASS/FAIL
// once a second; the LED blinks while all checks pass.

volatile uint32_t core1_count = 0;

void setup() {
    Serial.begin(115200);
    while (!Serial) {}
    pinMode(LED_BUILTIN, OUTPUT);
    Serial.println("multicore_idle: idleOtherCore() and the FIFO");
}

void loop() {
    static int rounds = 0;
    bool ok = true;

    // FIFO: core 1 sends back each value plus one.
    rp2040.fifo.push(rounds);
    const uint32_t echoed = rp2040.fifo.pop();
    ok = ok && echoed == static_cast<uint32_t>(rounds) + 1;

    // Idle: the count must not move while core 1 is stopped.
    rp2040.idleOtherCore();
    const uint32_t before = core1_count;
    delayMicroseconds(100000);  // busy-wait: delay() may let another task run
    const uint32_t during = core1_count;
    rp2040.resumeOtherCore();
    delay(10);
    const uint32_t after = core1_count;
    ok = ok && during == before && after != during;

    Serial.print(ok ? "PASS" : "FAIL");
    Serial.print("  round ");
    Serial.print(rounds++);
    Serial.print("  echo ");
    Serial.print(echoed);
    Serial.print("  count while idle +");
    Serial.print(during - before);
    Serial.print(", after resume +");
    Serial.println(after - during);

    if (ok) {
        digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
    } else {
        digitalWrite(LED_BUILTIN, LOW);
    }
    delay(1000);
}

void loop1() {
    uint32_t value;
    if (rp2040.fifo.pop_nb(&value)) {
        rp2040.fifo.push(value + 1);
    }
    core1_count++;
}
