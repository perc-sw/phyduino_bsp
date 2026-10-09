// USB-to-Serial1 bridge for checklist items 14-15: runs on a second board
// (e.g. Raspberry Pi Pico 2, Serial1 on GP0 TX / GP1 RX) wired to the Phyduino
// Pico's D0/D1, so the PC can talk to usbpd_status while a charger is on the
// Phyduino's USB-C port. Connect GND only, not 3V3 or VBUS.

void setup() {
    Serial.begin(115200);
    Serial1.begin(115200);
}

void loop() {
    while (Serial.available()) {
        Serial1.write(Serial.read());
    }
    while (Serial1.available()) {
        Serial.write(Serial1.read());
    }
}
