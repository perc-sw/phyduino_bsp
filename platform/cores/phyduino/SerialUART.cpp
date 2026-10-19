/**
 * @file SerialUART.cpp
 * @brief Serial1/Serial2 on the RP2040 UARTs (interrupt-driven receive).
 */
#include "Arduino.h"
#include "hardware/gpio.h"
#include "hardware/irq.h"
#include "pico/critical_section.h"

SerialUARTClass Serial1(uart0, PIN_SERIAL1_TX, PIN_SERIAL1_RX);
SerialUARTClass Serial2(uart1, PIN_SERIAL2_TX, PIN_SERIAL2_RX);

static void uart0Irq() { Serial1.handleRxInterrupt(); }
static void uart1Irq() { Serial2.handleRxInterrupt(); }

// Arduino SERIAL_xyz config -> pico-sdk format arguments.
static uint dataBits(uint16_t config) { return ((config & SERIAL_DATA_MASK) >> 8) + 4; }
static uint stopBits(uint16_t config) { return (config & SERIAL_STOP_BIT_MASK) == SERIAL_STOP_BIT_2 ? 2 : 1; }
static uart_parity_t parity(uint16_t config) {
    switch (config & SERIAL_PARITY_MASK) {
        case SERIAL_PARITY_EVEN: return UART_PARITY_EVEN;
        case SERIAL_PARITY_ODD:  return UART_PARITY_ODD;
        default:                 return UART_PARITY_NONE;  // mark/space not supported
    }
}

void SerialUARTClass::begin(unsigned long baud, uint16_t config) {
    end();
    if (!critical_section_is_initialized(&rx_lock_)) critical_section_init(&rx_lock_);
    uart_init(uart_, baud);
    uart_set_format(uart_, dataBits(config), stopBits(config), parity(config));
    uart_set_fifo_enabled(uart_, true);
    gpio_set_function(digitalPinToGpio(tx_pin_), GPIO_FUNC_UART);
    gpio_set_function(digitalPinToGpio(rx_pin_), GPIO_FUNC_UART);

    const uint irq = uart_get_index(uart_) == 0 ? UART0_IRQ : UART1_IRQ;
    irq_set_exclusive_handler(irq, uart_get_index(uart_) == 0 ? uart0Irq : uart1Irq);
    irq_set_enabled(irq, true);
    uart_set_irq_enables(uart_, true, false);
    started_ = true;
}

void SerialUARTClass::end() {
    if (!started_) return;
    flush();
    const uint irq = uart_get_index(uart_) == 0 ? UART0_IRQ : UART1_IRQ;
    uart_set_irq_enables(uart_, false, false);
    irq_set_enabled(irq, false);
    irq_remove_handler(irq, uart_get_index(uart_) == 0 ? uart0Irq : uart1Irq);
    uart_deinit(uart_);
    gpio_init(digitalPinToGpio(tx_pin_));
    gpio_init(digitalPinToGpio(rx_pin_));
    rx_buffer_.clear();
    started_ = false;
}

void SerialUARTClass::handleRxInterrupt() {
    critical_section_enter_blocking(&rx_lock_);
    while (uart_is_readable(uart_)) {
        const uint8_t c = static_cast<uint8_t>(uart_get_hw(uart_)->dr);
        if (!rx_buffer_.isFull()) rx_buffer_.store_char(c);  // drop on overflow
    }
    critical_section_exit(&rx_lock_);
}

// The ring buffer is shared with the IRQ, which may run on the other core: a
// critical section (interrupts off on this core + a spin lock) covers both.
// Before begin() there is no lock, and nothing to read.
int SerialUARTClass::available() {
    if (!started_) return 0;
    critical_section_enter_blocking(&rx_lock_);
    const int n = rx_buffer_.available();
    critical_section_exit(&rx_lock_);
    return n;
}

int SerialUARTClass::peek() {
    if (!started_) return -1;
    critical_section_enter_blocking(&rx_lock_);
    const int c = rx_buffer_.peek();
    critical_section_exit(&rx_lock_);
    return c;
}

int SerialUARTClass::read() {
    if (!started_) return -1;
    critical_section_enter_blocking(&rx_lock_);
    const int c = rx_buffer_.read_char();
    critical_section_exit(&rx_lock_);
    return c;
}

void SerialUARTClass::flush() {
    uart_tx_wait_blocking(uart_);
}

size_t SerialUARTClass::write(uint8_t c) {
    if (!started_) return 0;
    uart_putc_raw(uart_, static_cast<char>(c));
    return 1;
}
