// I2C peripheral at address 0x42: prints bytes a controller writes, and answers
// read requests with a counter. Pair with another board running a controller sketch.
#include <Wire.h>

volatile uint8_t counter = 0;

void onReceive(int count) {
    (void)count;
    while (Wire.available()) {
        Serial.write(Wire.read());
    }
}

void onRequest() {
    Wire.write(counter++);
}

void setup() {
    Serial.begin(115200);
    Wire.onReceive(onReceive);
    Wire.onRequest(onRequest);
    Wire.begin(0x42);
}

void loop() {}
