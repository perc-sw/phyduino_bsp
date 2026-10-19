// Runs two sketches at once, one on each core of the RP2040. setup() and
// loop() run on core 0 as usual; defining setup1() and loop1() starts core 1,
// which runs them at the same time. Core 0 prints a count every second while
// core 1 blinks the LED: neither one's delay() holds up the other.

void setup() {
    Serial.begin(115200);
    while (!Serial) {}
    Serial.print("setup() runs on core ");
    Serial.println(rp2040.cpuid());
}

void loop() {
    static unsigned long count = 0;
    Serial.print("loop() on core ");
    Serial.print(rp2040.cpuid());
    Serial.print(": ");
    Serial.println(count++);
    delay(1000);
}

void setup1() {
    pinMode(LED_BUILTIN, OUTPUT);
}

void loop1() {
    digitalWrite(LED_BUILTIN, HIGH);
    delay(100);
    digitalWrite(LED_BUILTIN, LOW);
    delay(400);
}
