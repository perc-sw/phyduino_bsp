// Reads all six analog pins together and streams them to the Serial Plotter
// (Tools > Serial Plotter, 115200 baud), 20 times a second.
//
// analogReadMultiple() reads the pins as close together in time as it can:
// A3-A5 in one scan of the external ADC, A0-A2 back to back on the RP2040
// ADC. The values are scaled to millivolts, so the two full scales
// (3.3 V and 2.048 V) plot on the same axis.

const pin_size_t PINS[] = {A0, A1, A2, A3, A4, A5};
const char* const NAMES[] = {"A0", "A1", "A2", "A3", "A4", "A5"};
const uint8_t COUNT = 6;

void setup() {
    Serial.begin(115200);
    analogReadResolution(12);
}

void loop() {
    uint16_t values[COUNT];
    analogReadMultiple(PINS, COUNT, values);

    for (uint8_t i = 0; i < COUNT; i++) {
        const uint32_t millivolts = values[i] * analogFullScaleMillivolts(PINS[i]) / 4096;
        Serial.print(NAMES[i]);
        Serial.print(":");
        Serial.print(millivolts);
        Serial.print(i + 1 < COUNT ? "," : "\n");
    }
    delay(50);
}
