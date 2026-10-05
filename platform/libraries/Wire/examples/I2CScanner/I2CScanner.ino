// Scans the I2C bus (SDA/SCL header pins) and prints every address that answers.
#include <Wire.h>

void setup() {
    Serial.begin(115200);
    while (!Serial) {}
    Wire.begin();
}

void loop() {
    int found = 0;
    for (uint8_t address = 0x08; address < 0x78; address++) {
        Wire.beginTransmission(address);
        if (Wire.endTransmission() == 0) {
            Serial.print("Device at 0x");
            Serial.println(address, HEX);
            found++;
        }
    }
    Serial.print(found);
    Serial.println(" device(s) found.");
    delay(5000);
}
