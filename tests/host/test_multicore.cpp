/**
 * @file test_multicore.cpp
 * @brief rp2040: inter-core FIFO, core 1 start-up, idleOtherCore() (cores/phyduino/RP2040.cpp).
 *
 * Without FreeRTOS (the PC tests build the no-OS core). fake::hw.core_num
 * says which core the code under test is running on.
 */
#include "test_support.h"
#include "wiring_private.h"

// The test sketch uses core 1. (Defining them also avoids relying on weak
// undefined symbols, which not every PC linker supports.)
extern "C" void setup1() {}
extern "C" void loop1() {}

namespace {
void drainFifo() {
    uint32_t v;
    for (uint core : {0u, 1u}) {
        fake::hw.core_num = core;
        while (rp2040.fifo.pop_nb(&v)) {}
    }
    fake::hw.core_num = 0;
}
}  // namespace

TEST_CASE_FIXTURE(HostTest, "fifo: a value pushed on one core is popped on the other") {
    rp2040.fifo.begin();
    drainFifo();

    rp2040.fifo.push(123);
    rp2040.fifo.push(0xDEADBEEF);
    CHECK(rp2040.fifo.available() == 0);   // nothing for core 0 itself

    fake::hw.core_num = 1;
    CHECK(rp2040.fifo.available() == 2);
    CHECK(rp2040.fifo.pop() == 123);
    CHECK(rp2040.fifo.pop() == 0xDEADBEEF);
    CHECK(rp2040.fifo.available() == 0);

    rp2040.fifo.push(7);                   // and back the other way
    fake::hw.core_num = 0;
    CHECK(rp2040.fifo.pop() == 7);
}

TEST_CASE_FIXTURE(HostTest, "fifo: non-blocking push and pop report full and empty") {
    rp2040.fifo.begin();
    drainFifo();

    for (uint32_t i = 0; i < RP2040FifoClass::kDepth; i++) {
        CHECK(rp2040.fifo.push_nb(i));
    }
    CHECK_FALSE(rp2040.fifo.push_nb(99));   // core 1's queue is full

    fake::hw.core_num = 1;
    uint32_t v = 0;
    for (uint32_t i = 0; i < RP2040FifoClass::kDepth; i++) {
        REQUIRE(rp2040.fifo.pop_nb(&v));
        CHECK(v == i);                       // in order
    }
    v = 42;
    CHECK_FALSE(rp2040.fifo.pop_nb(&v));
    CHECK(v == 42);                          // unchanged when empty
    fake::hw.core_num = 0;
}

TEST_CASE_FIXTURE(HostTest, "begin() twice keeps values already queued") {
    rp2040.fifo.begin();
    drainFifo();
    rp2040.fifo.push(5);
    rp2040.fifo.begin();
    fake::hw.core_num = 1;
    CHECK(rp2040.fifo.pop() == 5);
    fake::hw.core_num = 0;
}

TEST_CASE_FIXTURE(HostTest, "idleOtherCore() does nothing while core 1 runs no sketch") {
    rp2040.idleOtherCore();
    CHECK(fake::hw.lockout_starts == 0);
    rp2040.resumeOtherCore();
    CHECK(fake::hw.lockout_ends == 0);
}

TEST_CASE_FIXTURE(HostTest, "startCore1 launches core 1 when the sketch has setup1()/loop1()") {
    startCore1();
    CHECK(fake::hw.core1_launches == 1);
    CHECK(fake::hw.core1_entry != nullptr);
    CHECK(fake::hw.lockout_victim[0]);       // core 1 can stop core 0
}

TEST_CASE_FIXTURE(HostTest, "idleOtherCore()/resumeOtherCore() pair up and do not nest") {
    fake::hw.lockout_victim[1] = true;       // core 1 is running the sketch
    rp2040.idleOtherCore();
    CHECK(fake::hw.lockout_active);
    rp2040.idleOtherCore();                  // already stopped
    CHECK(fake::hw.lockout_starts == 1);

    rp2040.resumeOtherCore();
    CHECK_FALSE(fake::hw.lockout_active);
    rp2040.resumeOtherCore();                // nothing stopped any more
    CHECK(fake::hw.lockout_ends == 1);
}

TEST_CASE_FIXTURE(HostTest, "restartCore1 resets and relaunches core 1, from core 0 only") {
    startCore1();                            // the core remembers core 1 was started
    const int launches = fake::hw.core1_launches;

    fake::hw.core_num = 1;
    rp2040.restartCore1();
    CHECK(fake::hw.core1_resets == 0);

    fake::hw.core_num = 0;
    rp2040.restartCore1();
    CHECK(fake::hw.core1_resets == 1);
    CHECK(fake::hw.core1_launches == launches + 1);
}

TEST_CASE_FIXTURE(HostTest, "cpuid() and f_cpu()") {
    CHECK(rp2040.cpuid() == 0);
    fake::hw.core_num = 1;
    CHECK(rp2040.cpuid() == 1);
    fake::hw.core_num = 0;
    fake::hw.sys_hz = 200000000;
    CHECK(rp2040.f_cpu() == 200000000u);
}
