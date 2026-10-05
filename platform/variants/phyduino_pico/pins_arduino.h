/**
 * @file pins_arduino.h
 * @brief Phyduino Pico (RP2040): THE pin map. Every Arduino pin number and
 *        on-board peripheral pin is defined here and nowhere else.
 *
 * Board details and schematic notes: docs/boards/phyduino_pico.md
 *
 * Arduino pin numbers (Uno-style):
 *
 *   Pin   Name        RP2040            Pin   Name        RP2040
 *   0     D0 / RX     GPIO1  UART0 RX   11    D11 / MOSI  GPIO11 SPI1 TX
 *   1     D1 / TX     GPIO0  UART0 TX   12    D12 / MISO  GPIO12 SPI1 RX
 *   2     D2          GPIO6             13    D13 / SCK   GPIO14 SPI1 SCK
 *   3     D3          GPIO7             14    A0          GPIO26 ADC0
 *   4     D4          GPIO8  UART1 TX   15    A1          GPIO27 ADC1
 *   5     D5          GPIO9  UART1 RX   16    A2          GPIO28 ADC2
 *   6     D6          GPIO10            17    A3          MAX11635 AIN0 (analog only)
 *   7     D7          GPIO15            18    A4          MAX11635 AIN1 (analog only)
 *   8     D8          GPIO16            19    A5          MAX11635 AIN2 (analog only)
 *   9     D9          GPIO17            20    SDA         GPIO24 I2C0 SDA
 *   10    D10 / SS    GPIO13 SPI1 CSn   21    SCL         GPIO25 I2C0 SCL
 *                                       22    LED_BUILTIN GPIO5
 */
#pragma once

#include <stdint.h>

// --- Pin counts ----------------------------------------------------------------
#define PINS_COUNT          23
#define NUM_DIGITAL_PINS    23
#define NUM_ANALOG_INPUTS   6

// Marks an Arduino pin with no RP2040 GPIO behind it (A3-A5).
#define NOT_A_GPIO          0xFF

// Arduino pin number -> RP2040 GPIO, indexed by pin number (see table above).
#define PHYDUINO_PIN_TO_GPIO {                                    \
    1, 0, 6, 7, 8, 9, 10, 15, 16, 17, 13, 11, 12, 14, /* D0-D13 */ \
    26, 27, 28,                                       /* A0-A2  */ \
    NOT_A_GPIO, NOT_A_GPIO, NOT_A_GPIO,               /* A3-A5  */ \
    24, 25,                                           /* SDA, SCL */ \
    5,                                                /* LED    */ \
}

// --- Named pins --------------------------------------------------------------
#define PIN_A0  (14u)
#define PIN_A1  (15u)
#define PIN_A2  (16u)
#define PIN_A3  (17u)
#define PIN_A4  (18u)
#define PIN_A5  (19u)

static const uint8_t A0 = PIN_A0;
static const uint8_t A1 = PIN_A1;
static const uint8_t A2 = PIN_A2;
static const uint8_t A3 = PIN_A3;
static const uint8_t A4 = PIN_A4;
static const uint8_t A5 = PIN_A5;

#define PIN_LED         (22u)
#define LED_BUILTIN     PIN_LED

// --- Serial1 / Serial2 (UART) -------------------------------------------------
#define PIN_SERIAL1_TX  (1u)    // D1, UART0
#define PIN_SERIAL1_RX  (0u)    // D0
#define PIN_SERIAL2_TX  (4u)    // D4, UART1
#define PIN_SERIAL2_RX  (5u)    // D5

// --- SPI (SPI1 on the header) --------------------------------------------------
#define PIN_SPI_SS      (10u)
#define PIN_SPI_MOSI    (11u)
#define PIN_SPI_MISO    (12u)
#define PIN_SPI_SCK     (13u)
#define PHYDUINO_SPI_INSTANCE   spi1

static const uint8_t SS   = PIN_SPI_SS;
static const uint8_t MOSI = PIN_SPI_MOSI;
static const uint8_t MISO = PIN_SPI_MISO;
static const uint8_t SCK  = PIN_SPI_SCK;

// --- Wire (I2C0 on the header) -------------------------------------------------
#define PIN_WIRE_SDA    (20u)
#define PIN_WIRE_SCL    (21u)
#define PHYDUINO_WIRE_INSTANCE  i2c0

static const uint8_t SDA = PIN_WIRE_SDA;
static const uint8_t SCL = PIN_WIRE_SCL;

// --- On-board peripherals (RP2040 GPIO numbers, not Arduino pins) ------------
// External ADC MAX11635 (U5) on SPI0: feeds A3-A5; AIN3 measures VBUS.
#define PHYDUINO_EXT_ADC_SPI        spi0
#define PHYDUINO_EXT_ADC_GPIO_SCK   22
#define PHYDUINO_EXT_ADC_GPIO_MOSI  23
#define PHYDUINO_EXT_ADC_GPIO_MISO  20
#define PHYDUINO_EXT_ADC_GPIO_CS    21
#define PHYDUINO_EXT_ADC_GPIO_EOC   19
#define PHYDUINO_EXT_ADC_GPIO_CNVST 18
#define PHYDUINO_EXT_ADC_VREF_MV    2048    // LM4040 (U7) on AREF
#define PHYDUINO_EXT_ADC_VBUS_CH    3

// USB-PD controller STUSB4500 (U6) on I2C1, address 0x28.
#define PHYDUINO_USBPD_I2C          i2c1
#define PHYDUINO_USBPD_I2C_ADDR     0x28
#define PHYDUINO_USBPD_GPIO_SDA     2
#define PHYDUINO_USBPD_GPIO_SCL     3
#define PHYDUINO_USBPD_GPIO_ALERT   4
#define PHYDUINO_USBPD_GPIO_RESET   29

// Internal ADC full scale (RP2040 ADC reference = 3.3 V rail).
#define PHYDUINO_ADC_VREF_MV        3300
