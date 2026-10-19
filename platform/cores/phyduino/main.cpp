/**
 * @file main.cpp
 * @brief Program entry: the start-up order for every sketch.
 *
 *   pico-sdk runtime (crt0, clocks at the SDK's 125 MHz)
 *     -> main()
 *        -> init()         core: CPU clock (F_CPU), ADC, USB, inter-core FIFO
 *        -> initVariant()  board: on-board peripherals (variant.cpp)
 *        -> setup()        sketch
 *        -> loop()         sketch, forever
 *
 * If the sketch defines setup1() or loop1(), core 1 runs them, starting just
 * before setup(). With Tools > Operating System > FreeRTOS, setup()/loop() and
 * setup1()/loop1() are FreeRTOS tasks on cores 0 and 1 instead (freertos.cpp).
 */
#include "wiring_private.h"
#include "hardware/adc.h"
#include "hardware/clocks.h"
#include "hardware/timer.h"
#include "hardware/vreg.h"

#if PHYDUINO_FREERTOS
#include "FreeRTOS.h"
#include "task.h"
#endif

// Tools > CPU Speed sets F_CPU (boards.txt). The SDK has already started the
// clocks at 125 MHz; switch before anything reads clk_sys / clk_peri (UART, SPI,
// I2C and PWM rates are worked out from them in begin()).
static void setCpuClock() {
    if (clock_get_hz(clk_sys) == F_CPU) return;
    if (F_CPU > 133000000) {
        // Above 133 MHz the RP2040 needs a 1.15 V core (rated to 200 MHz there).
        // Same voltage and 1 ms settle time as the pico-sdk's 200 MHz option.
        vreg_set_voltage(VREG_VOLTAGE_1_15);
        busy_wait_us(1000);
    }
    set_sys_clock_khz(F_CPU / 1000, true);  // every menu speed is exact with the 12 MHz crystal

    // set_sys_clock_khz() leaves clk_peri (UART, SPI) on the 48 MHz USB PLL. Up to
    // 133 MHz put it back on clk_sys, as at boot, so SPI can reach F_CPU / 2.
    if (F_CPU <= 133000000) {
        clock_configure_undivided(clk_peri, 0, CLOCKS_CLK_PERI_CTRL_AUXSRC_VALUE_CLK_SYS, F_CPU);
    }
}

void init() {
    setCpuClock();
    adc_init();
    serialUsbInitStack();  // USB enumerates even if the sketch never uses Serial
    rp2040.fifo.begin();
}

// initVariant() is defined by every variant (variants/<board>/variant.cpp). No
// weak default here on purpose: the core is linked from an archive, where a weak
// default could silently win over the variant's version.

// Called by delay() and while waiting on Serial; a sketch may override it.
// With FreeRTOS it lets other tasks of the same priority run.
void yield() __attribute__((weak));
void yield() {
#if PHYDUINO_FREERTOS
    if (__get_current_exception() == 0 && xTaskGetSchedulerState() == taskSCHEDULER_RUNNING) {
        taskYIELD();
    }
#endif
}

int main() {
    init();
    initVariant();
#if PHYDUINO_FREERTOS
    startFreeRtos();  // never returns
#else
    startCore1();
    setup();
    for (;;) {
        loop();
    }
#endif
}
