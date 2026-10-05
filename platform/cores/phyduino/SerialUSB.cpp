/**
 * @file SerialUSB.cpp
 * @brief USB CDC `Serial`, TinyUSB background servicing, 1200-baud reset.
 *
 * Concurrency: TinyUSB is not thread-safe. tud_task() runs from a low-priority
 * IRQ raised every 1 ms; it only runs if it can take `usb_mutex`, and every
 * Serial method takes the same mutex, so the two never overlap.
 * USB descriptors are in usb/usb_descriptors.c.
 */
#include "Arduino.h"
#include "hardware/irq.h"
#include "pico/bootrom.h"
#include "pico/mutex.h"
#include "pico/time.h"
#include "tusb.h"

SerialUSBClass Serial;

static constexpr uint32_t kWriteTimeoutUs = 500 * 1000;  // give up if the host stops reading
static constexpr uint32_t kTouchBaud = 1200;             // Arduino "reboot to bootloader" signal

static mutex_t usb_mutex;
static uint usb_irq;

// --- Background task -------------------------------------------------------------
static void usbIrqHandler() {
    uint32_t owner;
    if (mutex_try_enter(&usb_mutex, &owner)) {
        tud_task();
        mutex_exit(&usb_mutex);
    }
}

static int64_t usbTimerCallback(alarm_id_t, void*) {
    irq_set_pending(usb_irq);
    return 1000;  // reschedule in 1 ms (microseconds)
}

void serialUsbInitStack() {
    mutex_init(&usb_mutex);
    tusb_init();

    usb_irq = user_irq_claim_unused(true);
    irq_set_exclusive_handler(usb_irq, usbIrqHandler);
    irq_set_priority(usb_irq, PICO_LOWEST_IRQ_PRIORITY);
    irq_set_enabled(usb_irq, true);
    add_alarm_in_us(1000, usbTimerCallback, nullptr, true);
}

// --- 1200-baud touch -------------------------------------------------------------
// The IDE opens the port at 1200 baud and closes it (DTR drops): reboot to BOOTSEL.
extern "C" void tud_cdc_line_state_cb(uint8_t itf, bool dtr, bool rts) {
    (void)rts;
    cdc_line_coding_t coding;
    tud_cdc_n_get_line_coding(itf, &coding);
    if (!dtr && coding.bit_rate == kTouchBaud) {
        reset_usb_boot(0, 0);
    }
}

// --- Serial API --------------------------------------------------------------------
SerialUSBClass::operator bool() {
    mutex_enter_blocking(&usb_mutex);
    const bool connected = tud_cdc_connected();
    mutex_exit(&usb_mutex);
    return connected;
}

int SerialUSBClass::available() {
    mutex_enter_blocking(&usb_mutex);
    const int n = static_cast<int>(tud_cdc_available());
    mutex_exit(&usb_mutex);
    return n;
}

int SerialUSBClass::peek() {
    uint8_t c;
    mutex_enter_blocking(&usb_mutex);
    const bool ok = tud_cdc_peek(&c);
    mutex_exit(&usb_mutex);
    return ok ? c : -1;
}

int SerialUSBClass::read() {
    mutex_enter_blocking(&usb_mutex);
    const int c = tud_cdc_available() ? static_cast<int>(tud_cdc_read_char()) : -1;
    mutex_exit(&usb_mutex);
    return c;
}

void SerialUSBClass::flush() {
    mutex_enter_blocking(&usb_mutex);
    tud_cdc_write_flush();
    tud_task();
    mutex_exit(&usb_mutex);
}

size_t SerialUSBClass::write(const uint8_t* buffer, size_t size) {
    size_t written = 0;
    uint32_t last_progress = time_us_32();

    while (written < size) {
        mutex_enter_blocking(&usb_mutex);
        if (!tud_cdc_connected()) {         // nobody listening: drop the data
            mutex_exit(&usb_mutex);
            break;
        }
        const uint32_t n = tud_cdc_write(buffer + written, size - written);
        tud_cdc_write_flush();
        tud_task();
        mutex_exit(&usb_mutex);

        if (n > 0) {
            written += n;
            last_progress = time_us_32();
        } else if (time_us_32() - last_progress > kWriteTimeoutUs) {
            break;
        } else {
            yield();
        }
    }
    return written;
}
