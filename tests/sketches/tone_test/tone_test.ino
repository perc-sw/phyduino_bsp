// Plays a short scale on D8 (connect a piezo buzzer to D8 and GND).
const unsigned int notes[] = {262, 294, 330, 349, 392, 440, 494, 523};

void setup() {
    for (unsigned int note : notes) {
        tone(8, note, 200);
        delay(250);
    }
}

void loop() {}
