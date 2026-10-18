// Samples A3 in the background while the sketch keeps working. Each block
// of 1000 samples (0.1 s at 10000 samples/s) is captured into one buffer
// while the previous block, in the other buffer, is processed, so sampling
// restarts within microseconds ("double buffering"). The built-in LED blinks
// to show that loop() is not blocked.
//
// For each block, prints the mean and the RMS of the signal around the mean
// (the AC part), in millivolts.

const pin_size_t PIN = A3;
const uint32_t RATE_HZ = 10000;
const size_t BLOCK = 1000;

uint16_t buffers[2][BLOCK];
int filling = 0;  // buffer the burst is writing to
unsigned long last_blink = 0;

void processBlock(const uint16_t* block) {
    const float mv_per_step = analogFullScaleMillivolts(PIN) / 4096.0f;
    float sum = 0;
    for (size_t i = 0; i < BLOCK; i++) sum += block[i];
    const float mean = sum / BLOCK;
    float squares = 0;
    for (size_t i = 0; i < BLOCK; i++) squares += (block[i] - mean) * (block[i] - mean);

    Serial.print("Mean ");
    Serial.print(mean * mv_per_step, 1);
    Serial.print(" mV, RMS ");
    Serial.print(sqrtf(squares / BLOCK) * mv_per_step, 2);
    Serial.println(" mV");
}

void setup() {
    Serial.begin(115200);
    while (!Serial) {}
    pinMode(LED_BUILTIN, OUTPUT);
    analogReadResolution(12);
    analogStartBurst(PIN, buffers[filling], BLOCK, RATE_HZ);
}

void loop() {
    if (analogBurstDone()) {
        const int full = filling;
        filling = 1 - filling;
        analogStartBurst(PIN, buffers[filling], BLOCK, RATE_HZ);  // next block straight away
        processBlock(buffers[full]);
    }

    // Other work carries on while the samples are taken.
    if (millis() - last_blink >= 250) {
        last_blink = millis();
        digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
    }
}
