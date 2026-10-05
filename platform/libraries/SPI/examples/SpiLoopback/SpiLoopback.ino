// SPI loopback test: connect D11 (MOSI) to D12 (MISO).
// Every byte sent should come straight back.
#include <SPI.h>

void setup() {
    Serial.begin(115200);
    SPI.begin();
}

void loop() {
    static uint8_t value = 0;
    SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0));
    const uint8_t received = SPI.transfer(value);
    SPI.endTransaction();

    Serial.print("sent ");
    Serial.print(value);
    Serial.print(" received ");
    Serial.println(received);
    value++;
    delay(500);
}
