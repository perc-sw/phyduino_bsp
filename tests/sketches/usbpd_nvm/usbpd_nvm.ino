// STUSB4500 NVM tool: run once per board to set the USB-PD chip's stored
// (power-up) settings to 5 V only, so VIN is 5 V from plug-in until a sketch
// asks for more (Tools > USB-PD Power changes the working registers only).
//
// Serial Monitor at 115200, send a command line:
//   r        read the NVM and print it (raw sectors + decoded profiles). Read-only.
//   write5v  set 5 V only, 2.0 A (sector 3 only). Skipped if already set.
//   factory  put back ST's factory profiles (5 V/1.5 A, 15 V/1.5 A, 20 V/1.0 A),
//            sector 3 only; the other sectors keep this chip's own values.
// Unplug and replug the board afterwards: the chip loads its NVM at power-up.
//
// The NVM is good for about 1000 writes, so this is a tool, never start-up code.
// Procedure and layout as in ST's reference driver (via SparkFun's STUSB4500 library).
#include "hardware/i2c.h"

namespace {

constexpr uint8_t kRegPassword = 0x95, kPassword = 0x47;
constexpr uint8_t kRegCtrl0 = 0x96, kRegCtrl1 = 0x97, kRegBuffer = 0x53;
constexpr uint8_t kPwr = 0x80, kRstN = 0x40, kReq = 0x10;        // FTP_CTRL_0 bits
constexpr uint8_t kOpRead = 0x00, kOpWritePl = 0x01, kOpWriteSer = 0x02,
                  kOpErase = 0x05, kOpProg = 0x06, kOpSoftProg = 0x07;  // FTP_CTRL_1 opcodes
constexpr uint8_t kSectors = 5;

// ST's factory NVM contents (reference driver). Sector 0 can differ per chip
// (one board read B0 AB in bytes 2-3), so only sector 3 is ever restored from here.
const uint8_t kFactory[kSectors][8] = {
    {0x00, 0x00, 0xB0, 0xAA, 0x00, 0x45, 0x00, 0x00},
    {0x10, 0x40, 0x9C, 0x1C, 0xFF, 0x01, 0x3C, 0xDF},
    {0x02, 0x40, 0x0F, 0x00, 0x32, 0x00, 0xFC, 0xF1},
    {0x00, 0x19, 0x56, 0xAF, 0xF5, 0x35, 0x5F, 0x00},
    {0x00, 0x4B, 0x90, 0x21, 0x43, 0x00, 0x40, 0xFB},
};

bool writeReg(uint8_t reg, const uint8_t* data, size_t length) {
    uint8_t buffer[1 + 8];
    buffer[0] = reg;
    for (size_t i = 0; i < length; i++) buffer[1 + i] = data[i];
    return i2c_write_timeout_us(PHYDUINO_USBPD_I2C, PHYDUINO_USBPD_I2C_ADDR, buffer, length + 1,
                                false, 10000) == static_cast<int>(length + 1);
}

bool writeReg(uint8_t reg, uint8_t value) { return writeReg(reg, &value, 1); }

bool readReg(uint8_t reg, uint8_t* data, size_t length) {
    return i2c_write_timeout_us(PHYDUINO_USBPD_I2C, PHYDUINO_USBPD_I2C_ADDR, &reg, 1, true, 10000) == 1 &&
           i2c_read_timeout_us(PHYDUINO_USBPD_I2C, PHYDUINO_USBPD_I2C_ADDR, data, length, false,
                               10000) == static_cast<int>(length);
}

// Starts an NVM operation (opcode already in FTP_CTRL_1) and waits for it.
bool run(uint8_t sector = 0) {
    if (!writeReg(kRegCtrl0, (sector & 0x07) | kPwr | kRstN | kReq)) return false;
    const uint32_t start = millis();
    uint8_t ctrl;
    do {
        if (!readReg(kRegCtrl0, &ctrl, 1)) return false;
        if (millis() - start > 500) return false;
    } while (ctrl & kReq);
    return true;
}

bool enter() {
    return writeReg(kRegPassword, kPassword) && writeReg(kRegCtrl0, 0) &&
           writeReg(kRegCtrl0, kPwr | kRstN);
}

void leave() {
    const uint8_t ctrl[2] = {kRstN, 0x00};
    writeReg(kRegCtrl0, ctrl, 2);
    writeReg(kRegPassword, 0);
}

bool readNvm(uint8_t out[kSectors][8]) {
    bool ok = enter();
    for (uint8_t s = 0; ok && s < kSectors; s++) {
        ok = writeReg(kRegCtrl0, kPwr | kRstN) && writeReg(kRegCtrl1, kOpRead) && run(s) &&
             readReg(kRegBuffer, out[s], 8);
    }
    leave();
    return ok;
}

// Erases and reprograms only the sectors that differ from `target`.
bool writeNvm(const uint8_t target[kSectors][8], const uint8_t current[kSectors][8]) {
    uint8_t mask = 0;
    for (uint8_t s = 0; s < kSectors; s++) {
        if (memcmp(target[s], current[s], 8) != 0) mask |= 1 << s;
    }
    if (mask == 0) {
        Serial.println("NVM already holds these settings: nothing written.");
        return true;
    }
    Serial.print("Writing sector mask 0x");
    Serial.println(mask, HEX);

    bool ok = writeReg(kRegPassword, kPassword) && writeReg(kRegBuffer, 0) &&  // buffer 0 for partial erase
              writeReg(kRegCtrl0, 0) && writeReg(kRegCtrl0, kPwr | kRstN) &&
              writeReg(kRegCtrl1, ((mask << 3) & 0xF8) | kOpWriteSer) && run() &&
              writeReg(kRegCtrl1, kOpSoftProg) && run() &&
              writeReg(kRegCtrl1, kOpErase) && run();
    for (uint8_t s = 0; ok && s < kSectors; s++) {
        if (!(mask & (1 << s))) continue;
        ok = writeReg(kRegBuffer, target[s], 8) && writeReg(kRegCtrl0, kPwr | kRstN) &&
             writeReg(kRegCtrl1, kOpWritePl) && run() &&
             writeReg(kRegCtrl1, kOpProg) && run(s);
    }
    leave();
    return ok;
}

uint32_t currentMa(uint8_t code) {  // 4-bit NVM current code
    if (code == 0) return 0;         // 0 = "flex" current
    return code < 11 ? 250 * code + 250 : 500 * code - 2500;
}

void print(const uint8_t nvm[kSectors][8]) {
    for (uint8_t s = 0; s < kSectors; s++) {
        Serial.print("sector ");
        Serial.print(s);
        Serial.print(":");
        for (uint8_t b = 0; b < 8; b++) {
            Serial.print(nvm[s][b] < 0x10 ? " 0" : " ");
            Serial.print(nvm[s][b], HEX);
        }
        Serial.println(memcmp(nvm[s], kFactory[s], 8) == 0 ? "  (= ST default)" : "  (differs from ST default)");
    }
    const uint8_t count = (nvm[3][2] & 0x06) >> 1;
    const uint32_t mv2 = (((nvm[4][1] << 2) | (nvm[4][0] >> 6)) & 0x3FF) * 50;
    const uint32_t mv3 = ((((nvm[4][3] & 0x03) << 8) | nvm[4][2])) * 50;
    Serial.print("profiles: ");
    Serial.println(count);
    Serial.print("  1: 5000 mV ");
    Serial.print(currentMa(nvm[3][2] >> 4));
    Serial.println(" mA");
    Serial.print(count >= 2 ? "  2: " : "  (2: ");
    Serial.print(mv2);
    Serial.print(" mV ");
    Serial.print(currentMa(nvm[3][4] & 0x0F));
    Serial.println(count >= 2 ? " mA" : " mA, unused)");
    Serial.print(count >= 3 ? "  3: " : "  (3: ");
    Serial.print(mv3);
    Serial.print(" mV ");
    Serial.print(currentMa(nvm[3][5] >> 4));
    Serial.println(count >= 3 ? " mA" : " mA, unused)");
}

void command(const String& line) {
    uint8_t nvm[kSectors][8];
    if (!readNvm(nvm)) {
        Serial.println("ERROR: could not read the NVM (STUSB4500 not answering?)");
        return;
    }
    if (line == "r") {
        print(nvm);
        return;
    }
    uint8_t target[kSectors][8];
    if (line == "write5v") {
        memcpy(target, nvm, sizeof(nvm));
        // Sector 3 byte 2: bits 7:4 PDO1 current code, bits 2:1 profile count.
        target[3][2] = static_cast<uint8_t>((nvm[3][2] & 0x09) | (7 << 4) | (1 << 1));  // 2.0 A, 1 profile
    } else if (line == "factory") {
        memcpy(target, nvm, sizeof(nvm));
        memcpy(target[3], kFactory[3], 8);
    } else {
        Serial.println("Commands: r, write5v, factory");
        return;
    }
    Serial.println("Before:");
    print(nvm);
    if (!writeNvm(target, nvm) || !readNvm(nvm)) {
        Serial.println("ERROR: NVM write failed; send r to check, retry the command");
        return;
    }
    Serial.println("After (read back):");
    print(nvm);
    Serial.println(memcmp(nvm, target, sizeof(nvm)) == 0
                       ? "OK: verified. Unplug and replug the board to use the new settings."
                       : "ERROR: read-back differs from what was written");
}

String line;

}  // namespace

void setup() {
    Serial.begin(115200);
}

void loop() {
    while (Serial.available()) {
        const char c = static_cast<char>(Serial.read());
        if (c == '\n' || c == '\r') {
            if (line.length() > 0) command(line);
            line = "";
        } else if (line.length() < 16) {
            line += c;
        }
    }
}
