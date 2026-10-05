// Counts falling edges on D2 (connect a button to GND) and reports them.
volatile unsigned long presses = 0;

void onPress() { presses++; }

void setup() {
    Serial.begin(115200);
    pinMode(2, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(2), onPress, FALLING);
}

void loop() {
    noInterrupts();
    const unsigned long count = presses;
    interrupts();
    Serial.println(count);
    delay(1000);
}
