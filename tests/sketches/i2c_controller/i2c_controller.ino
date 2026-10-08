// I2C controller for checklist item 13: runs on a second board (any Arduino,
// e.g. Raspberry Pi Pico 2) with the Phyduino Pico running PeripheralReceiver at 0x42.
// Writes a line to the peripheral, then reads back its 1-byte counter.
// A 5 V board needs a level shifter on SDA/SCL: the RP2040 is not 5 V tolerant.
#include <Wire.h>

const uint8_t PERIPHERAL = 0x42;

void setup() {
    Serial.begin(115200);
    Wire.begin();
}

void loop() {
    static unsigned int count = 0;

    Wire.beginTransmission(PERIPHERAL);
    Wire.print("hello ");
    Wire.println(count++);
    uint8_t error = Wire.endTransmission();
    Serial.print("write: ");
    Serial.println(error == 0 ? "ok" : "no answer");

    Serial.print("read:");
    Wire.requestFrom(PERIPHERAL, (uint8_t)1);  // PeripheralReceiver sends 1 byte per read
    while (Wire.available()) {
        Serial.print(' ');
        Serial.print(Wire.read());
    }
    Serial.println();

    delay(1000);
}
