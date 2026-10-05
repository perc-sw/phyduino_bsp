/**
 * @file wiring_shift.cpp
 * @brief shiftOut(), shiftIn(): bit-banged serial data, one bit per clock pulse.
 */
#include "Arduino.h"

void shiftOut(pin_size_t dataPin, pin_size_t clockPin, BitOrder bitOrder, uint8_t value) {
    for (uint8_t i = 0; i < 8; i++) {
        const uint8_t bit = (bitOrder == LSBFIRST) ? i : 7 - i;
        digitalWrite(dataPin, (value >> bit) & 1 ? HIGH : LOW);
        digitalWrite(clockPin, HIGH);
        digitalWrite(clockPin, LOW);
    }
}

uint8_t shiftIn(pin_size_t dataPin, pin_size_t clockPin, BitOrder bitOrder) {
    uint8_t value = 0;
    for (uint8_t i = 0; i < 8; i++) {
        digitalWrite(clockPin, HIGH);
        const uint8_t bit = (bitOrder == LSBFIRST) ? i : 7 - i;
        if (digitalRead(dataPin) == HIGH) value |= (1u << bit);
        digitalWrite(clockPin, LOW);
    }
    return value;
}
