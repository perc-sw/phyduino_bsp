// Prints the voltage on every analog pin, in millivolts, once a second.
// A0-A2 (RP2040 ADC) read 0-3300 mV and A3-A5 (on-board MAX11635) read
// 0-2048 mV; analogReadMillivolts() handles the difference, so the same code
// works on every pin.

const pin_size_t PINS[] = {A0, A1, A2, A3, A4, A5};
const char* const NAMES[] = {"A0", "A1", "A2", "A3", "A4", "A5"};

void setup() {
    Serial.begin(115200);
    while (!Serial) {}

    Serial.println("Full scale of each pin:");
    for (int i = 0; i < 6; i++) {
        Serial.print(NAMES[i]);
        Serial.print(": ");
        Serial.print(analogFullScaleMillivolts(PINS[i]));
        Serial.println(" mV");
    }
}

void loop() {
    for (int i = 0; i < 6; i++) {
        Serial.print(NAMES[i]);
        Serial.print(" ");
        Serial.print(analogReadMillivolts(PINS[i]));
        Serial.print(" mV   ");
    }
    Serial.println();
    delay(1000);
}
