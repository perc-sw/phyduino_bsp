/**
 * @file os_tick_rp2040.c
 * @brief The three OS Tick functions (CMSIS_6 os_tick.h) the CMSIS-RTOS2
 *        wrapper uses for osKernelGetSysTimerCount().
 *
 * The FreeRTOS RP2040 port runs the tick from SysTick on core 0 only
 * (configTICK_CORE), and each core's SysTick is private to it. On core 0 these
 * read SysTick, giving clk_sys resolution; on core 1 they report the start of
 * the current tick, so osKernelGetSysTimerCount() steps once per tick (1 ms)
 * there. The port sets SysTick up itself, so the OS_Tick_Setup/Enable/...
 * functions are not provided.
 */
#include "os_tick.h"

#include "FreeRTOS.h"
#include "hardware/clocks.h"
#include "hardware/structs/scb.h"
#include "hardware/structs/systick.h"
#include "pico/platform.h"

uint32_t OS_Tick_GetInterval(void) {
    if (get_core_num() == configTICK_CORE) {
        return systick_hw->rvr + 1U;
    }
    return clock_get_hz(clk_sys) / configTICK_RATE_HZ;
}

uint32_t OS_Tick_GetCount(void) {
    if (get_core_num() == configTICK_CORE) {
        return systick_hw->rvr - systick_hw->cvr;   // SysTick counts down
    }
    return 0U;
}

uint32_t OS_Tick_GetOverflow(void) {
    if (get_core_num() == configTICK_CORE) {
        // A tick interrupt is pending: SysTick has wrapped but the tick
        // count has not been updated yet.
        return (scb_hw->icsr & M0PLUS_ICSR_PENDSTSET_BITS) != 0U;
    }
    return 0U;
}
