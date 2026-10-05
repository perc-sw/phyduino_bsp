/**
 * @file random.cpp
 * @brief random(), randomSeed() (map() and makeWord() come from ArduinoCore-API).
 */
#include "Arduino.h"
#include <stdlib.h>

void randomSeed(unsigned long seed) {
    if (seed != 0) srand(static_cast<unsigned>(seed));
}

long random(long howbig) {
    if (howbig <= 0) return 0;
    return rand() % howbig;
}

long random(long howsmall, long howbig) {
    if (howsmall >= howbig) return howsmall;
    return random(howbig - howsmall) + howsmall;
}
