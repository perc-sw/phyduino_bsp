// USB-PD status: prints the board's power profiles, the active contract and
// the measured VBUS once a second. Send '5', '9' or 'f' (full) over Serial to
// limit the highest profile and watch VBUS change (needs a USB-PD charger).
#include <stusb4500.h>

static void useProfiles(uint8_t count) {
    const stusb4500::Profile all[] = {{5000, 2000}, {9000, 1000}, {20000, 500}};
    Serial.println(stusb4500::setProfiles(all, count) ? "profiles updated" : "update failed");
}

void setup() {
    Serial.begin(115200);
    while (!Serial) {}
    Serial.println(stusb4500::begin() ? "STUSB4500 found" : "STUSB4500 not answering");
}

void loop() {
    switch (Serial.read()) {
        case '5': useProfiles(1); break;
        case '9': useProfiles(2); break;
        case 'f': useProfiles(3); break;
    }

    stusb4500::Profile profiles[stusb4500::kMaxProfiles];
    const uint8_t count = stusb4500::getProfiles(profiles);
    Serial.print("profiles:");
    for (uint8_t i = 0; i < count; i++) {
        Serial.print(' ');
        Serial.print(profiles[i].millivolts / 1000.0, 1);
        Serial.print("V/");
        Serial.print(profiles[i].milliamps / 1000.0, 1);
        Serial.print('A');
    }
    Serial.print("  contract: offer ");
    Serial.print(stusb4500::contractPosition());
    Serial.print(", ");
    Serial.print(stusb4500::contractMilliamps());
    Serial.print(" mA  VBUS: ");
    Serial.print(stusb4500::vbusMillivolts() / 1000.0, 2);
    Serial.println(" V");
    delay(1000);
}
