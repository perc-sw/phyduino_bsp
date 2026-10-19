/**
 * @file tusb_config.h
 * @brief TinyUSB configuration compiled into libpico.a.
 *
 * The USB descriptors and callbacks are in the core
 * (platform/cores/phyduino/usb/usb_descriptors.c); they must agree with the
 * class counts below. Changing this file requires regenerating the SDK bundle.
 */
#ifndef PHYDUINO_TUSB_CONFIG_H
#define PHYDUINO_TUSB_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

// --- Common ------------------------------------------------------------------
#define CFG_TUSB_RHPORT0_MODE   OPT_MODE_DEVICE
#define CFG_TUD_ENABLED         1
#define CFG_TUD_ENDPOINT0_SIZE  64

#ifndef CFG_TUSB_MEM_SECTION
#define CFG_TUSB_MEM_SECTION
#endif
#ifndef CFG_TUSB_MEM_ALIGN
#define CFG_TUSB_MEM_ALIGN      __attribute__((aligned(4)))
#endif

// --- Device classes ----------------------------------------------------------
#define CFG_TUD_CDC             1  // Serial (USB CDC-ACM)
#define CFG_TUD_MSC             0
#define CFG_TUD_HID             0
#define CFG_TUD_MIDI            0
#define CFG_TUD_VENDOR          0

#define CFG_TUD_CDC_RX_BUFSIZE  256
#define CFG_TUD_CDC_TX_BUFSIZE  256
#define CFG_TUD_CDC_EP_BUFSIZE  64

#ifdef __cplusplus
}
#endif

#endif  // PHYDUINO_TUSB_CONFIG_H
