// USB-to-UART bridge for checklist items 14, 15, 20 and 27: runs on a second
// board (a Raspberry Pi Pico 2 with the arduino-pico core) wired to the
// Phyduino Pico's D0/D1, so the PC can talk to usbpd_status or UsbPdPower
// while a charger is on the Phyduino's USB-C port.
// Uses Serial2 (UART1): GP4 TX (pin 6), GP5 RX (pin 7); GND on pin 8.
// Connect GND only, not 3V3 or VBUS.
// (Serial1 on GP0/GP12 sent nothing from the test Pico 2 with arduino-pico
// 6.3.2, found 2026-11-11; Serial2 works.)

void setup() {
    Serial.begin(115200);
    Serial2.setTX(4);
    Serial2.setRX(5);
    Serial2.begin(115200);
}

void loop() {
    while (Serial.available()) {
        Serial2.write(Serial.read());
    }
    while (Serial2.available()) {
        Serial.write(Serial2.read());
    }
}
