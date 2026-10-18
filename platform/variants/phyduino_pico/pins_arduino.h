/**
 * @file pins_arduino.h
 * @brief Phyduino Pico (RP2040): THE pin map. Every Arduino pin number and
 *        on-board peripheral pin is defined here and nowhere else.
 *
 * Board details and schematic notes: docs/boards/phyduino_pico.html
 *
 * Arduino pin numbers (Uno-style):
 *
 * @verbatim
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
 * @endverbatim
 */
#pragma once

#include <stdint.h>

// --- Pin counts ----------------------------------------------------------------
#define PINS_COUNT          23  ///< Arduino pin numbers 0-22
#define NUM_DIGITAL_PINS    23  ///< Pins 0-22; A3-A5 are analog only
#define NUM_ANALOG_INPUTS   6   ///< A0-A5

/// Marks an Arduino pin with no RP2040 GPIO behind it (A3-A5).
#define NOT_A_GPIO          0xFF

/// Arduino pin number -> RP2040 GPIO, indexed by pin number (see table above).
#define PHYDUINO_PIN_TO_GPIO {                                    \
    1, 0, 6, 7, 8, 9, 10, 15, 16, 17, 13, 11, 12, 14, /* D0-D13 */ \
    26, 27, 28,                                       /* A0-A2  */ \
    NOT_A_GPIO, NOT_A_GPIO, NOT_A_GPIO,               /* A3-A5  */ \
    24, 25,                                           /* SDA, SCL */ \
    5,                                                /* LED    */ \
}

// --- Named pins --------------------------------------------------------------
#define PIN_A0  (14u)  ///< A0, RP2040 ADC0
#define PIN_A1  (15u)  ///< A1, RP2040 ADC1
#define PIN_A2  (16u)  ///< A2, RP2040 ADC2
#define PIN_A3  (17u)  ///< A3, MAX11635 AIN0 (analog only)
#define PIN_A4  (18u)  ///< A4, MAX11635 AIN1 (analog only)
#define PIN_A5  (19u)  ///< A5, MAX11635 AIN2 (analog only)

static const uint8_t A0 = PIN_A0;  ///< Analog pin A0 (0-3.3 V)
static const uint8_t A1 = PIN_A1;  ///< Analog pin A1 (0-3.3 V)
static const uint8_t A2 = PIN_A2;  ///< Analog pin A2 (0-3.3 V)
static const uint8_t A3 = PIN_A3;  ///< Analog pin A3 (0-2.048 V, analog only)
static const uint8_t A4 = PIN_A4;  ///< Analog pin A4 (0-2.048 V, analog only)
static const uint8_t A5 = PIN_A5;  ///< Analog pin A5 (0-2.048 V, analog only)

#define PIN_LED         (22u)    ///< On-board LED, RP2040 GPIO5
#define LED_BUILTIN     PIN_LED  ///< On-board LED

// --- Serial1 / Serial2 (UART) -------------------------------------------------
#define PIN_SERIAL1_TX  (1u)    ///< Serial1 TX: D1, UART0
#define PIN_SERIAL1_RX  (0u)    ///< Serial1 RX: D0
#define PIN_SERIAL2_TX  (4u)    ///< Serial2 TX: D4, UART1
#define PIN_SERIAL2_RX  (5u)    ///< Serial2 RX: D5

// --- SPI (SPI1 on the header) --------------------------------------------------
#define PIN_SPI_SS      (10u)  ///< SPI chip select: D10 (not driven by the SPI library)
#define PIN_SPI_MOSI    (11u)  ///< SPI MOSI: D11
#define PIN_SPI_MISO    (12u)  ///< SPI MISO: D12
#define PIN_SPI_SCK     (13u)  ///< SPI clock: D13
#define PHYDUINO_SPI_INSTANCE   spi1  ///< RP2040 SPI peripheral used by the SPI library

static const uint8_t SS   = PIN_SPI_SS;    ///< SPI chip select (D10)
static const uint8_t MOSI = PIN_SPI_MOSI;  ///< SPI MOSI (D11)
static const uint8_t MISO = PIN_SPI_MISO;  ///< SPI MISO (D12)
static const uint8_t SCK  = PIN_SPI_SCK;   ///< SPI clock (D13)

// --- Wire (I2C0 on the header) -------------------------------------------------
#define PIN_WIRE_SDA    (20u)  ///< I2C data: SDA header pin, RP2040 GPIO24
#define PIN_WIRE_SCL    (21u)  ///< I2C clock: SCL header pin, RP2040 GPIO25
#define PHYDUINO_WIRE_INSTANCE  i2c0  ///< RP2040 I2C peripheral used by the Wire library

static const uint8_t SDA = PIN_WIRE_SDA;  ///< I2C data pin
static const uint8_t SCL = PIN_WIRE_SCL;  ///< I2C clock pin

// --- On-board peripherals (RP2040 GPIO numbers, not Arduino pins) ------------
// External ADC MAX11635 (U5) on SPI0: feeds A3-A5; AIN3 measures VBUS.
#define PHYDUINO_EXT_ADC_SPI        spi0  ///< External ADC: RP2040 SPI peripheral
#define PHYDUINO_EXT_ADC_GPIO_SCK   22    ///< External ADC: SPI clock
#define PHYDUINO_EXT_ADC_GPIO_MOSI  23    ///< External ADC: SPI MOSI
#define PHYDUINO_EXT_ADC_GPIO_MISO  20    ///< External ADC: SPI MISO
#define PHYDUINO_EXT_ADC_GPIO_CS    21    ///< External ADC: chip select
#define PHYDUINO_EXT_ADC_GPIO_EOC   19    ///< External ADC: end of conversion, low when ready
#define PHYDUINO_EXT_ADC_GPIO_CNVST 18    ///< External ADC: conversion start
#define PHYDUINO_EXT_ADC_VREF_MV    2048  ///< External ADC full scale in mV: LM4040 (U7) on AREF
#define PHYDUINO_EXT_ADC_VBUS_CH    3     ///< External ADC channel that measures VBUS

// USB-PD controller STUSB4500 (U6) on I2C1, address 0x28.
#define PHYDUINO_USBPD_I2C          i2c1  ///< USB-PD controller: RP2040 I2C peripheral
#define PHYDUINO_USBPD_I2C_ADDR     0x28  ///< USB-PD controller: 7-bit I2C address
#define PHYDUINO_USBPD_GPIO_SDA     2     ///< USB-PD controller: I2C data
#define PHYDUINO_USBPD_GPIO_SCL     3     ///< USB-PD controller: I2C clock
#define PHYDUINO_USBPD_GPIO_ALERT   4     ///< USB-PD controller: ALERT output
#define PHYDUINO_USBPD_GPIO_RESET   29    ///< USB-PD controller: RESET input

/// Internal ADC full scale in mV (RP2040 ADC reference = 3.3 V rail).
#define PHYDUINO_ADC_VREF_MV        3300
