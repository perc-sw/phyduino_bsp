// USB-PD power: prints the board's power profiles, the charger's contract and
// the measured VBUS once a second. Send 5, 9, 12, 15 or 20 to ask a USB-PD
// charger for up to that many volts and watch VBUS change.
// A charger takes the USB-C port, so output also goes to Serial1 (D1 TX,
// D0 RX, 115200) and commands are read from it: watch from a USB-serial
// adapter, or a second board running a serial bridge.
// VBUS also feeds the VIN pin, so VIN follows the contract: disconnect
// anything on VIN that can't take the voltage you ask for.
// The request lasts until the board loses power; at the next start-up the
// Tools > USB-PD Power setting applies again.
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

static void askForVolts(long volts) {
    if (volts != 5 && volts != 9 && volts != 12 && volts != 15 && volts != 20) {
        out.println("Send 5, 9, 12, 15 or 20.");
        return;
    }
    // The board's profiles up to that voltage, limited to 10 W.
    stusb4500::Profile profiles[stusb4500::kMaxProfiles];
    const uint8_t count = stusb4500::boardProfiles(volts * 1000, profiles);
    out.print("Asking for up to ");
    out.print(volts);
    out.println(stusb4500::setProfiles(profiles, count) ? " V" : " V: failed");
}

static void printStatus() {
    stusb4500::Profile profiles[stusb4500::kMaxProfiles];
    const uint8_t count = stusb4500::getProfiles(profiles);
    out.print("profiles:");
    for (uint8_t i = 0; i < count; i++) {
        out.print(' ');
        out.print(profiles[i].millivolts / 1000.0, 1);
        out.print("V/");
        out.print(profiles[i].milliamps / 1000.0, 2);
        out.print('A');
    }

    // Offer 0 means no PD contract: plain 5 V USB, or a PC port.
    out.print("   contract: offer ");
    out.print(stusb4500::contractPosition());
    out.print(", ");
    out.print(stusb4500::contractMilliamps());
    out.print(" mA   VBUS: ");
    out.print(stusb4500::vbusMillivolts() / 1000.0, 2);
    out.println(" V");
}

void setup() {
    Serial.begin(115200);
    Serial1.begin(115200);
    const unsigned long start = millis();
    while (!Serial && millis() - start < 5000) {}   // no PC on a charger: go on after 5 s

    if (!stusb4500::begin()) {
        out.println("USB-PD controller not answering");
        while (true) {}
    }
    out.println("Send 5, 9, 12, 15 or 20 to set the highest voltage.");
}

void loop() {
    // parseInt() returns 0 for a lone line ending, so 0 is ignored.
    if (Serial.available()) {
        const long volts = Serial.parseInt();
        if (volts != 0) askForVolts(volts);
    }
    if (Serial1.available()) {
        const long volts = Serial1.parseInt();
        if (volts != 0) askForVolts(volts);
    }

    printStatus();
    delay(1000);
}
