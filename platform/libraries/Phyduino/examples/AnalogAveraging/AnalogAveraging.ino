// Shows how averaging steadies a reading. For each setting, takes 100
// readings of A0 and A3 and prints the lowest, the highest and the spread
// between them (in ADC steps). Connect a steady voltage, e.g. a potentiometer
// wiper, to A0 (0-3.3 V) and A3 (0-2.048 V), or leave them as they are.
//
// analogReadAveraging() applies to every analog pin: A3-A5 average in the
// external ADC itself, A0-A2 take the samples one after another.

const int READINGS = 100;
const uint8_t SETTINGS[] = {1, 4, 8, 16, 32};

void printSpread(const char* name, pin_size_t pin) {
    int lowest = 4095;
    int highest = 0;
    for (int i = 0; i < READINGS; i++) {
        const int value = analogRead(pin);
        if (value < lowest) lowest = value;
        if (value > highest) highest = value;
    }
    Serial.print(name);
    Serial.print(" ");
    Serial.print(lowest);
    Serial.print("-");
    Serial.print(highest);
    Serial.print(" (spread ");
    Serial.print(highest - lowest);
    Serial.print(")   ");
}

void setup() {
    Serial.begin(115200);
    while (!Serial) {}
    analogReadResolution(12);  // 0-4095, to see the small changes
}

void loop() {
    for (uint8_t samples : SETTINGS) {
        analogReadAveraging(samples);
        Serial.print("Averaging ");
        Serial.print(samples);
        Serial.print(":   ");
        printSpread("A0", A0);
        printSpread("A3", A3);
        Serial.println();
    }
    Serial.println();
    delay(3000);
}
