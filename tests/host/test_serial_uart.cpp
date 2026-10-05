/**
 * @file test_serial_uart.cpp
 * @brief Serial1 / Serial2 on the RP2040 UARTs (cores/phyduino/SerialUART.cpp).
 */
#include "test_support.h"

TEST_CASE_FIXTURE(HostTest, "Serial1.begin sets up UART0 on D1 (TX) / D0 (RX), 8N1") {
    Serial1.begin(9600);
    const auto& uart = fake::hw.uart[0];
    CHECK(uart.enabled);
    CHECK(uart.baud == 9600);
    CHECK(uart.data_bits == 8);
    CHECK(uart.stop_bits == 1);
    CHECK(uart.parity == UART_PARITY_NONE);
    CHECK(uart.rx_irq);
    CHECK(fake::hw.irq_enabled[UART0_IRQ]);
    CHECK(gpioOf(PIN_SERIAL1_TX).function == GPIO_FUNC_UART);
    CHECK(gpioOf(PIN_SERIAL1_RX).function == GPIO_FUNC_UART);
    CHECK(static_cast<bool>(Serial1));
    Serial1.end();
}

TEST_CASE_FIXTURE(HostTest, "SERIAL_xyz configs map to data bits, parity and stop bits") {
    Serial1.begin(115200, SERIAL_7E2);
    CHECK(fake::hw.uart[0].data_bits == 7);
    CHECK(fake::hw.uart[0].parity == UART_PARITY_EVEN);
    CHECK(fake::hw.uart[0].stop_bits == 2);

    Serial1.begin(115200, SERIAL_8O1);
    CHECK(fake::hw.uart[0].data_bits == 8);
    CHECK(fake::hw.uart[0].parity == UART_PARITY_ODD);
    CHECK(fake::hw.uart[0].stop_bits == 1);

    Serial1.begin(115200, SERIAL_5N1);
    CHECK(fake::hw.uart[0].data_bits == 5);
    Serial1.end();
}

TEST_CASE_FIXTURE(HostTest, "write and print go out of the UART") {
    Serial1.begin(9600);
    Serial1.write('A');
    Serial1.print("bc");
    Serial1.println(42);
    CHECK(fake::hw.uart[0].tx == "Abc42\r\n");
    Serial1.end();
}

TEST_CASE_FIXTURE(HostTest, "received bytes are buffered by the RX interrupt") {
    Serial1.begin(9600);
    for (uint8_t c : {'x', 'y', 'z'}) fake::hw.uart[0].rx.push_back(c);
    fake::raise_irq(UART0_IRQ);

    CHECK(Serial1.available() == 3);
    CHECK(Serial1.peek() == 'x');
    CHECK(Serial1.read() == 'x');
    CHECK(Serial1.read() == 'y');
    CHECK(Serial1.read() == 'z');
    CHECK(Serial1.read() == -1);
    CHECK(fake::hw.interrupts_disabled == 0);    // every critical section was closed
    Serial1.end();
}

TEST_CASE_FIXTURE(HostTest, "the RX buffer keeps the first 256 bytes when it overflows") {
    Serial1.begin(9600);
    for (int i = 0; i < 300; i++) fake::hw.uart[0].rx.push_back(static_cast<uint8_t>(i));
    fake::raise_irq(UART0_IRQ);
    CHECK(Serial1.available() == 256);
    CHECK(Serial1.read() == 0);
    Serial1.end();
}

TEST_CASE_FIXTURE(HostTest, "Serial2 is UART1 on D4 (TX) / D5 (RX)") {
    Serial2.begin(19200);
    CHECK(fake::hw.uart[1].enabled);
    CHECK(fake::hw.uart[1].baud == 19200);
    CHECK_FALSE(fake::hw.uart[0].enabled);
    CHECK(fake::hw.irq_enabled[UART1_IRQ]);
    CHECK(gpioOf(PIN_SERIAL2_TX).function == GPIO_FUNC_UART);
    Serial2.print("hi");
    CHECK(fake::hw.uart[1].tx == "hi");
    Serial2.end();
}

TEST_CASE_FIXTURE(HostTest, "end() releases the UART and pins; writes are then dropped") {
    Serial1.begin(9600);
    Serial1.end();
    CHECK_FALSE(fake::hw.uart[0].enabled);
    CHECK_FALSE(fake::hw.irq_enabled[UART0_IRQ]);
    CHECK(gpioOf(PIN_SERIAL1_TX).function == GPIO_FUNC_SIO);
    CHECK_FALSE(static_cast<bool>(Serial1));
    CHECK(Serial1.write('x') == 0);
    CHECK(fake::hw.uart[0].tx.empty());
    Serial1.end();                               // a second end() is harmless
}
