// Compile check for pulseIn(), shiftOut(), shiftIn(), random(), String.
void setup() {
    Serial.begin(115200);
    pinMode(3, INPUT);
    pinMode(4, OUTPUT);
    pinMode(5, OUTPUT);
    randomSeed(analogRead(A0));
}

void loop() {
    const unsigned long width = pulseIn(3, HIGH, 100000);
    shiftOut(4, 5, MSBFIRST, static_cast<uint8_t>(random(256)));
    const uint8_t in = shiftIn(3, 5, LSBFIRST);
    String line = String("pulse=") + width + " in=" + String(in, HEX) + " pi=" + String(3.14159, 3);
    Serial.println(line);
    delay(500);
}
