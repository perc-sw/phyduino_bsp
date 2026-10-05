/**
 * @file string_conversions.cpp
 * @brief itoa() family and dtostrf(): AVR-era helpers that ArduinoCore-API's
 *        String class needs but leaves to the core.
 */
#include "Arduino.h"
#include "api/itoa.h"
#include <stdio.h>

static char* unsignedToString(unsigned long value, char* out, int radix) {
    if (radix < 2 || radix > 36) {
        *out = '\0';
        return out;
    }
    char digits[sizeof(unsigned long) * 8 + 1];
    int n = 0;
    do {
        const unsigned d = value % radix;
        digits[n++] = static_cast<char>(d < 10 ? '0' + d : 'a' + d - 10);
        value /= radix;
    } while (value != 0);

    char* p = out;
    while (n > 0) *p++ = digits[--n];
    *p = '\0';
    return out;
}

// Arduino/AVR convention: only base 10 shows a minus sign; other bases print
// the bit pattern of the original type (`bits`, e.g. int -1 in hex = ffffffff).
static char* signedToString(long value, unsigned long bits, char* out, int radix) {
    if (radix == 10 && value < 0) {
        out[0] = '-';
        unsignedToString(0ul - static_cast<unsigned long>(value), out + 1, radix);
        return out;
    }
    return unsignedToString(bits, out, radix);
}

extern "C" {

char* ultoa(unsigned long value, char* out, int radix) { return unsignedToString(value, out, radix); }
char* utoa(unsigned value, char* out, int radix)       { return unsignedToString(value, out, radix); }
char* ltoa(long value, char* out, int radix) {
    return signedToString(value, static_cast<unsigned long>(value), out, radix);
}
char* itoa(int value, char* out, int radix) {
    return signedToString(value, static_cast<unsigned>(value), out, radix);
}

char* dtostrf(double value, signed char width, unsigned char precision, char* out) {
    sprintf(out, "%*.*f", width, precision, value);
    return out;
}

}  // extern "C"
