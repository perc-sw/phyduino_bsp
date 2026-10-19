/**
 * @file heap.cpp
 * @brief rp2040.getTotalHeap(), getUsedHeap(), getFreeHeap().
 *
 * Separate from RP2040.cpp because it needs the linker script's heap symbols
 * and newlib's mallinfo(), which the PC-side tests do not have.
 */
#include "Arduino.h"
#include <malloc.h>

extern "C" char __end__;      // start of the heap (linker script)
extern "C" char __HeapLimit;  // end of the heap

int RP2040Class::getTotalHeap() {
    return static_cast<int>(&__HeapLimit - &__end__);
}

int RP2040Class::getUsedHeap() {
    return static_cast<int>(mallinfo().uordblks);
}

int RP2040Class::getFreeHeap() {
    return getTotalHeap() - getUsedHeap();
}
