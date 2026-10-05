// Serial1 loopback: wire D1 (TX) to D0 (RX). Sends a counter on Serial1 and
// reports on USB Serial what came back.
void setup() {
    Serial.begin(115200);
    Serial1.begin(115200);
    Serial2.begin(9600, SERIAL_8E1);  // compile check for config formats
}

void loop() {
    static unsigned int count = 0;
    Serial1.println(count++);
    delay(100);
    while (Serial1.available()) {
        Serial.write(Serial1.read());
    }
    delay(400);
}
