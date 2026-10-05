// USB Serial: prints a banner, then echoes every received character.
void setup() {
    Serial.begin(115200);
    while (!Serial) {}
    Serial.println("Phyduino serial echo. Type something:");
}

void loop() {
    while (Serial.available()) {
        Serial.write(Serial.read());
    }
}
