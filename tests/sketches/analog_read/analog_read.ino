// Prints A0-A5 every 500 ms. A0-A2 = RP2040 ADC (3.3 V full scale),
// A3-A5 = on-board MAX11635 (2.048 V full scale).
void setup() {
    Serial.begin(115200);
    analogReadResolution(12);
}

void loop() {
    for (int pin = A0; pin <= A5; pin++) {
        Serial.print(analogRead(pin));
        Serial.print(pin < A5 ? '\t' : '\n');
    }
    delay(500);
}
