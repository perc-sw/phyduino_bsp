// tone() check without a buzzer: wire D8 (tone output) to D2 (input).
// Plays each frequency on D8 and measures it on D2 with pulseIn(), then checks
// that a timed tone stops after its duration.
const unsigned int frequencies[] = {262, 440, 1000, 2000, 5000};

void setup() {
    Serial.begin(115200);
    while (!Serial) {}
    pinMode(2, INPUT_PULLDOWN);   // reads LOW once tone() releases D8
}

void loop() {
    for (unsigned int f : frequencies) {
        tone(8, f);
        delay(50);
        const unsigned long high = pulseIn(2, HIGH, 100000);
        const unsigned long low = pulseIn(2, LOW, 100000);
        noTone(8);
        const unsigned long period = high + low;
        Serial.print("tone ");
        Serial.print(f);
        Serial.print(" Hz -> measured ");
        Serial.print(period ? 1000000.0 / period : 0.0, 1);
        Serial.print(" Hz, duty ");
        Serial.print(period ? 100.0 * high / period : 0.0, 0);
        Serial.println(" %");
    }

    tone(8, 1000, 200);
    delay(100);
    const bool playing_at_100ms = pulseIn(2, HIGH, 5000) > 0;
    delay(200);
    const bool playing_at_300ms = pulseIn(2, HIGH, 5000) > 0;
    Serial.print("tone(8, 1000, 200): playing at 100 ms = ");
    Serial.print(playing_at_100ms ? "yes" : "no");
    Serial.print(", at 300 ms = ");
    Serial.println(playing_at_300ms ? "yes" : "no");
    Serial.println("---");
    delay(2000);
}
