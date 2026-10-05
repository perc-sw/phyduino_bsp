/**
 * @file phyduino_pico.h
 * @brief pico-sdk board definition for the Phyduino Pico (RP2040).
 *
 * Only settings the pico-sdk itself uses live here (flash, boot stage 2,
 * crystal, SDK default peripherals). The Arduino pin map lives in
 * platform/variants/phyduino_pico/pins_arduino.h.
 *
 * Changing this file requires regenerating the SDK bundle:
 *   docs/maintenance/update-pico-sdk.md
 */
#ifndef PHYDUINO_BOARDS_PHYDUINO_PICO_H
#define PHYDUINO_BOARDS_PHYDUINO_PICO_H

pico_board_cmake_set(PICO_PLATFORM, rp2040)

#define PHYDUINO_PICO

// --- Clock: 12 MHz crystal (Y1) ---------------------------------------------
#ifndef PICO_XOSC_STARTUP_DELAY_MULTIPLIER
#define PICO_XOSC_STARTUP_DELAY_MULTIPLIER 64
#endif

// --- Flash: W25Q128JV, 16 MB (U9) -------------------------------------------
// The W25Q080 boot stage 2 supports the whole W25Q family, including W25Q128JV.
#define PICO_BOOT_STAGE2_CHOOSE_W25Q080 1

#ifndef PICO_FLASH_SPI_CLKDIV
#define PICO_FLASH_SPI_CLKDIV 2
#endif

pico_board_cmake_set_default(PICO_FLASH_SIZE_BYTES, (16 * 1024 * 1024))
#ifndef PICO_FLASH_SIZE_BYTES
#define PICO_FLASH_SIZE_BYTES (16 * 1024 * 1024)
#endif

#ifndef PICO_RP2040_B0_SUPPORTED
#define PICO_RP2040_B0_SUPPORTED 1
#endif

// --- SDK default peripherals (match the Arduino header pins) ----------------
#ifndef PICO_DEFAULT_LED_PIN
#define PICO_DEFAULT_LED_PIN 5
#endif

#ifndef PICO_DEFAULT_UART
#define PICO_DEFAULT_UART 0  // D1 = TX, D0 = RX
#endif
#ifndef PICO_DEFAULT_UART_TX_PIN
#define PICO_DEFAULT_UART_TX_PIN 0
#endif
#ifndef PICO_DEFAULT_UART_RX_PIN
#define PICO_DEFAULT_UART_RX_PIN 1
#endif

#ifndef PICO_DEFAULT_I2C
#define PICO_DEFAULT_I2C 0  // D18 = SDA, D19 = SCL
#endif
#ifndef PICO_DEFAULT_I2C_SDA_PIN
#define PICO_DEFAULT_I2C_SDA_PIN 24
#endif
#ifndef PICO_DEFAULT_I2C_SCL_PIN
#define PICO_DEFAULT_I2C_SCL_PIN 25
#endif

#ifndef PICO_DEFAULT_SPI
#define PICO_DEFAULT_SPI 1  // D13 = SCK, D11 = MOSI, D12 = MISO, D10 = SS
#endif
#ifndef PICO_DEFAULT_SPI_SCK_PIN
#define PICO_DEFAULT_SPI_SCK_PIN 14
#endif
#ifndef PICO_DEFAULT_SPI_TX_PIN
#define PICO_DEFAULT_SPI_TX_PIN 11
#endif
#ifndef PICO_DEFAULT_SPI_RX_PIN
#define PICO_DEFAULT_SPI_RX_PIN 12
#endif
#ifndef PICO_DEFAULT_SPI_CSN_PIN
#define PICO_DEFAULT_SPI_CSN_PIN 13
#endif

#endif  // PHYDUINO_BOARDS_PHYDUINO_PICO_H
