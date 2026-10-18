// Captures a waveform: 500 samples at a fixed rate, taken by the hardware at
// evenly spaced times, then plotted in the Serial Plotter (Tools > Serial
// Plotter, 115200 baud). Prints the sample rate, lowest, highest and mean too.
//
// Try it with a signal generator, or with tone(2, 500) and a wire from D2 to
// A0 (D2 is 0-3.3 V, so use A0-A2 for it, not A3-A5).
//
// Highest rates: A0-A2 500000 samples/s, A3-A5 300000 samples/s.

const pin_size_t PIN = A0;
const uint32_t RATE_HZ = 20000;  // samples per second
const size_t SAMPLES = 500;

uint16_t samples[SAMPLES];

void setup() {
    Serial.begin(115200);
    while (!Serial) {}
    analogReadResolution(12);
    tone(2, 500);  // test signal on D2; remove if you use your own
}

void loop() {
    const uint32_t rate = analogReadBurst(PIN, samples, SAMPLES, RATE_HZ);
    if (rate == 0) {
        Serial.println("Burst refused: check PIN and RATE_HZ");
        delay(2000);
        return;
    }

    uint16_t lowest = 4095;
    uint16_t highest = 0;
    uint32_t sum = 0;
    for (size_t i = 0; i < SAMPLES; i++) {
        if (samples[i] < lowest) lowest = samples[i];
        if (samples[i] > highest) highest = samples[i];
        sum += samples[i];
        Serial.print("Signal:");
        Serial.println(samples[i]);
    }

    Serial.print("Rate ");
    Serial.print(rate);
    Serial.print(" Hz, lowest ");
    Serial.print(lowest);
    Serial.print(", highest ");
    Serial.print(highest);
    Serial.print(", mean ");
    Serial.println(sum / SAMPLES);
    delay(2000);
}
