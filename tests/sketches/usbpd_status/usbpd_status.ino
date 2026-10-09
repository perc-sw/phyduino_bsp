// USB-PD status: prints the board's power profiles, the active contract and
// the measured VBUS once a second. Send '5', '9' or 'f' (full) over Serial to
// limit the highest profile and watch VBUS change (needs a USB-PD charger).
// Output also goes to Serial1 (D1 TX, D0 RX, 115200), and commands are read
// from it, so a second board running serial_bridge can stand in for the PC
// when a charger is on the USB-C port.
#include <stusb4500.h>

// Writes to USB Serial and Serial1. USB Serial drops data when no PC is listening.
class BothSerials : public Print {
public:
    size_t write(uint8_t c) override {
        Serial.write(c);
        Serial1.write(c);
        return 1;
    }
};

static BothSerials out;

static void useProfiles(uint8_t count) {
    const stusb4500::Profile all[] = {{5000, 2000}, {9000, 1000}, {20000, 500}};
    out.println(stusb4500::setProfiles(all, count) ? "profiles updated" : "update failed");
}

void setup() {
    Serial.begin(115200);
    Serial1.begin(115200);
    const unsigned long start = millis();
    while (!Serial && millis() - start < 5000) {}   // no PC on a charger: go on after 5 s
    out.println(stusb4500::begin() ? "STUSB4500 found" : "STUSB4500 not answering");
}

static void handleCommand(int c) {
    switch (c) {
        case '5': useProfiles(1); break;
        case '9': useProfiles(2); break;
        case 'f': useProfiles(3); break;
    }
}

void loop() {
    // Handle everything waiting, not one character per second, so commands act at once.
    while (Serial.available()) handleCommand(Serial.read());
    while (Serial1.available()) handleCommand(Serial1.read());

    stusb4500::Profile profiles[stusb4500::kMaxProfiles];
    const uint8_t count = stusb4500::getProfiles(profiles);
    out.print("profiles:");
    for (uint8_t i = 0; i < count; i++) {
        out.print(' ');
        out.print(profiles[i].millivolts / 1000.0, 1);
        out.print("V/");
        out.print(profiles[i].milliamps / 1000.0, 1);
        out.print('A');
    }
    out.print("  contract: offer ");
    out.print(stusb4500::contractPosition());
    out.print(", ");
    out.print(stusb4500::contractMilliamps());
    out.print(" mA  VBUS: ");
    out.print(stusb4500::vbusMillivolts() / 1000.0, 2);
    out.println(" V");
    delay(1000);
}
