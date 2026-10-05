// Serial2 loopback: wire D4 (TX) to D5 (RX). Sends a counter on Serial2 and
// reports on USB Serial what came back.
void setup() {
    Serial.begin(115200);
    Serial2.begin(115200);
}

void loop() {
    static unsigned int count = 0;
    Serial2.println(count++);
    delay(100);
    while (Serial2.available()) {
        Serial.write(Serial2.read());
    }
    delay(400);
}
