/**
 * @file usb_descriptors.c
 * @brief USB identity of every sketch: one CDC serial interface.
 *
 * VID/PID and the manufacturer/product strings come from boards.txt
 * (USB_VID, USB_PID, USB_MANUFACTURER, USB_PRODUCT). The serial number is
 * the flash chip's unique ID. Must agree with sdk-builder/config/tusb_config.h.
 */
#include <string.h>

#include "pico/unique_id.h"
#include "tusb.h"

// --- Device -----------------------------------------------------------------------
enum { STR_LANGID, STR_MANUFACTURER, STR_PRODUCT, STR_SERIAL, STR_CDC };

static const tusb_desc_device_t device_descriptor = {
    .bLength            = sizeof(tusb_desc_device_t),
    .bDescriptorType    = TUSB_DESC_DEVICE,
    .bcdUSB             = 0x0200,
    // Interface Association Descriptor (required for CDC on Windows)
    .bDeviceClass       = TUSB_CLASS_MISC,
    .bDeviceSubClass    = MISC_SUBCLASS_COMMON,
    .bDeviceProtocol    = MISC_PROTOCOL_IAD,
    .bMaxPacketSize0    = CFG_TUD_ENDPOINT0_SIZE,
    .idVendor           = USB_VID,
    .idProduct          = USB_PID,
    .bcdDevice          = 0x0100,
    .iManufacturer      = STR_MANUFACTURER,
    .iProduct           = STR_PRODUCT,
    .iSerialNumber      = STR_SERIAL,
    .bNumConfigurations = 1,
};

const uint8_t* tud_descriptor_device_cb(void) {
    return (const uint8_t*)&device_descriptor;
}

// --- Configuration: one CDC (two interfaces: control + data) ----------------
enum { ITF_CDC_CONTROL, ITF_CDC_DATA, ITF_COUNT };

#define EP_CDC_NOTIFY   0x81
#define EP_CDC_OUT      0x02
#define EP_CDC_IN       0x82
#define CONFIG_LEN      (TUD_CONFIG_DESC_LEN + TUD_CDC_DESC_LEN)

static const uint8_t configuration_descriptor[] = {
    TUD_CONFIG_DESCRIPTOR(1, ITF_COUNT, 0, CONFIG_LEN, 0, 250),
    TUD_CDC_DESCRIPTOR(ITF_CDC_CONTROL, STR_CDC, EP_CDC_NOTIFY, 8,
                       EP_CDC_OUT, EP_CDC_IN, CFG_TUD_CDC_EP_BUFSIZE),
};

const uint8_t* tud_descriptor_configuration_cb(uint8_t index) {
    (void)index;
    return configuration_descriptor;
}

// --- Strings (UTF-16LE) -------------------------------------------------------
const uint16_t* tud_descriptor_string_cb(uint8_t index, uint16_t langid) {
    (void)langid;
    static uint16_t out[64];
    char serial[2 * PICO_UNIQUE_BOARD_ID_SIZE_BYTES + 1];
    const char* text;

    switch (index) {
        case STR_LANGID:
            out[1] = 0x0409;  // English (US)
            out[0] = (uint16_t)((TUSB_DESC_STRING << 8) | 4);
            return out;
        case STR_MANUFACTURER: text = USB_MANUFACTURER; break;
        case STR_PRODUCT:      text = USB_PRODUCT;      break;
        case STR_SERIAL:
            pico_get_unique_board_id_string(serial, sizeof(serial));
            text = serial;
            break;
        case STR_CDC:          text = USB_PRODUCT " Serial"; break;
        default:               return NULL;
    }

    size_t len = strlen(text);
    if (len > 63) len = 63;
    for (size_t i = 0; i < len; i++) out[1 + i] = (uint8_t)text[i];
    out[0] = (uint16_t)((TUSB_DESC_STRING << 8) | (2 * len + 2));
    return out;
}
