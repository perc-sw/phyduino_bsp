/**
 * @file cmsis_device_rp2040.h
 * @brief CMSIS_device_header for compiling the CMSIS-RTOS2 wrapper (cmsis_os2.c).
 *
 * The wrapper only needs the CMSIS compiler intrinsics (__get_IPSR(),
 * __get_PRIMASK(), __disable_irq(), __WEAK, ...), so this includes just those.
 * The SDK's full CMSIS device header (RP2040.h) would redefine the SDK's own
 * register addresses, and its SysTick definition would make the wrapper add a
 * SysTick_Handler the FreeRTOS RP2040 port does not use.
 */
#ifndef PHYDUINO_CMSIS_DEVICE_RP2040_H
#define PHYDUINO_CMSIS_DEVICE_RP2040_H

#include "cmsis_compiler.h"

#endif  // PHYDUINO_CMSIS_DEVICE_RP2040_H
