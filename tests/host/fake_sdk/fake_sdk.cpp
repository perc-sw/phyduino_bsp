/**
 * @file fake_sdk.cpp
 * @brief Simulated hardware behind the fake pico-sdk (see fake_sdk.h).
 */
#include "fake_sdk.h"

#include <algorithm>

namespace fake {

uart_inst_t uart_instances[2] = {{0}, {1}};
spi_inst_t  spi_instances[2]  = {{0}, {1}};
i2c_inst_t  i2c_instances[2]  = {{0}, {1}};
Hardware hw;
AdcHw adc_regs = {0};
static UartHw uart_hw[2] = {{{0}}, {{1}}};
static SpiHw spi_hw[2] = {{0}, {0}};

void reset() {
    hw = Hardware{};
}

void advance_us(uint64_t us) {
    const uint64_t target = hw.now_us + us;
    if (hw.in_alarm) {           // time read inside an alarm callback: no nested alarms
        hw.now_us = target;
        return;
    }
    for (;;) {
        // Earliest alarm due by `target`.
        auto due = std::min_element(hw.alarms.begin(), hw.alarms.end(),
                                    [](const Alarm& a, const Alarm& b) { return a.at_us < b.at_us; });
        if (due == hw.alarms.end() || due->at_us > target) break;
        const Alarm alarm = *due;
        hw.alarms.erase(due);
        hw.now_us = std::max(hw.now_us, alarm.at_us);
        hw.in_alarm = true;
        const int64_t again = alarm.callback(alarm.id, alarm.data);
        hw.in_alarm = false;
        if (again > 0) {
            hw.alarms.push_back({alarm.id, alarm.at_us + static_cast<uint64_t>(again), alarm.callback, alarm.data});
        }
    }
    hw.now_us = std::max(hw.now_us, target);
}

void raise_irq(uint irq) {
    if (irq < 32 && hw.irq_enabled[irq] && hw.irq_handlers[irq]) hw.irq_handlers[irq]();
}

void trigger_gpio_irq(uint gpio, uint32_t events) {
    if (hw.gpio_callback && (hw.gpio[gpio].irq_events & events)) {
        hw.gpio_callback(gpio, events & hw.gpio[gpio].irq_events);
    }
}

bool pin_level(uint gpio) {
    const Gpio& g = hw.gpio[gpio];
    if (g.output && g.function == GPIO_FUNC_SIO) return g.out_level;
    if (g.waveform) return g.waveform(hw.now_us);
    return g.input_level;
}

UartData::operator uint32_t() const {
    auto& rx = hw.uart[index].rx;
    if (rx.empty()) return 0;
    const uint8_t c = rx.front();
    rx.pop_front();
    return c;
}

}  // namespace fake

using fake::hw;

// --- time ----------------------------------------------------------------------------
uint64_t time_us_64() {
    const uint64_t now = hw.now_us;
    fake::advance_us(hw.us_per_time_read);
    return now;
}
uint32_t time_us_32() { return static_cast<uint32_t>(time_us_64()); }
void sleep_ms(uint32_t ms) { fake::advance_us(static_cast<uint64_t>(ms) * 1000); }
void sleep_us(uint64_t us) { fake::advance_us(us); }
void busy_wait_us_32(uint32_t us) { fake::advance_us(us); }
void busy_wait_at_least_cycles(uint32_t) {}

alarm_id_t add_alarm_in_us(uint64_t us, alarm_callback_t callback, void* data, bool) {
    const alarm_id_t id = hw.next_alarm_id++;
    hw.alarms.push_back({id, hw.now_us + us, callback, data});
    return id;
}
alarm_id_t add_alarm_in_ms(uint32_t ms, alarm_callback_t callback, void* data, bool fire_if_past) {
    return add_alarm_in_us(static_cast<uint64_t>(ms) * 1000, callback, data, fire_if_past);
}
static int64_t repeatingTimerAlarm(alarm_id_t, void* data) {
    auto* timer = static_cast<repeating_timer_t*>(data);
    const int64_t period = timer->delay_us < 0 ? -timer->delay_us : timer->delay_us;
    return timer->callback(timer) ? period : 0;
}
bool add_repeating_timer_us(int64_t delay_us, repeating_timer_callback_t callback, void* user_data,
                            repeating_timer_t* out) {
    out->delay_us = delay_us;
    out->callback = callback;
    out->user_data = user_data;
    out->alarm_id = add_alarm_in_us(static_cast<uint64_t>(delay_us < 0 ? -delay_us : delay_us),
                                    repeatingTimerAlarm, out, true);
    return true;
}
bool cancel_repeating_timer(repeating_timer_t* timer) { return cancel_alarm(timer->alarm_id); }
bool cancel_alarm(alarm_id_t id) {
    const auto before = hw.alarms.size();
    hw.alarms.erase(std::remove_if(hw.alarms.begin(), hw.alarms.end(),
                                   [id](const fake::Alarm& a) { return a.id == id; }),
                    hw.alarms.end());
    return hw.alarms.size() != before;
}

// --- gpio ----------------------------------------------------------------------------
void gpio_init(uint gpio) {
    auto& g = hw.gpio[gpio];
    g.initialised = true;
    g.function = GPIO_FUNC_SIO;
    g.output = false;
    g.out_level = false;
}
void gpio_set_function(uint gpio, uint function) { hw.gpio[gpio].function = function; }
void gpio_set_dir(uint gpio, bool out) { hw.gpio[gpio].output = out; }
void gpio_put(uint gpio, bool value) {
    hw.gpio[gpio].out_level = value;
    if (hw.on_gpio_put) hw.on_gpio_put(gpio, value);
}
bool gpio_get(uint gpio) { return fake::pin_level(gpio); }
void gpio_pull_up(uint gpio) { hw.gpio[gpio].pull_up = true; hw.gpio[gpio].pull_down = false; }
void gpio_pull_down(uint gpio) { hw.gpio[gpio].pull_up = false; hw.gpio[gpio].pull_down = true; }
void gpio_disable_pulls(uint gpio) { hw.gpio[gpio].pull_up = false; hw.gpio[gpio].pull_down = false; }
void gpio_set_drive_strength(uint gpio, gpio_drive_strength drive) { hw.gpio[gpio].drive = drive; }
void gpio_set_irq_enabled(uint gpio, uint32_t events, bool enabled) {
    if (enabled) hw.gpio[gpio].irq_events |= events;
    else         hw.gpio[gpio].irq_events &= ~events;
}
void gpio_set_irq_enabled_with_callback(uint gpio, uint32_t events, bool enabled, gpio_irq_callback_t callback) {
    hw.gpio_callback = callback;
    gpio_set_irq_enabled(gpio, events, enabled);
}

// --- adc -------------------------------------------------------------------------------
void adc_init() { hw.adc_initialised = true; }
void adc_gpio_init(uint gpio) {
    hw.gpio[gpio].function = GPIO_FUNC_NULL;
    gpio_disable_pulls(gpio);
}
void adc_select_input(uint input) { hw.adc_selected = input; }
static uint16_t adcConvert() {
    hw.adc_conversions++;
    return hw.adc_sample ? hw.adc_sample(hw.adc_selected) : hw.adc_values[hw.adc_selected];
}
uint16_t adc_read() { return adcConvert(); }
void adc_fifo_setup(bool en, bool dreq_en, uint16_t, bool, bool) {
    hw.adc_fifo_enabled = en;
    hw.adc_fifo_dreq = dreq_en;
}
void adc_fifo_drain() {}
void adc_set_clkdiv(float clkdiv) { hw.adc_clkdiv = clkdiv; }
void adc_run(bool run) {
    hw.adc_running = run;
    if (run) hw.adc_clkdiv_at_run = hw.adc_clkdiv;
}

// --- dma ---------------------------------------------------------------------------------
// Transfers run all at once when polled: the ADC FIFO (DREQ_ADC) fills the
// buffer from adc_sample/adc_values; an SPI RX channel runs together with the
// TX channel of the same SPI, 16-bit frames through the SPI device.
static uint16_t spiTransfer16(int index, uint16_t out);

static int polls_left[12] = {};
int dma_claim_unused_channel(bool) {
    for (int i = 0; i < 12; i++) {
        if (!hw.dma[i].claimed) {
            hw.dma[i] = fake::DmaChannel{};
            hw.dma[i].claimed = true;
            polls_left[i] = 0;
            return i;
        }
    }
    return -1;
}
void dma_channel_unclaim(uint channel) { hw.dma[channel].claimed = false; }
dma_channel_config dma_channel_get_default_config(uint) { return {DMA_SIZE_32, true, false, 0x3F}; }
void channel_config_set_transfer_data_size(dma_channel_config* c, dma_channel_transfer_size size) { c->size = size; }
void channel_config_set_read_increment(dma_channel_config* c, bool incr) { c->read_increment = incr; }
void channel_config_set_write_increment(dma_channel_config* c, bool incr) { c->write_increment = incr; }
void channel_config_set_dreq(dma_channel_config* c, uint dreq) { c->dreq = dreq; }
void dma_channel_configure(uint channel, const dma_channel_config* config, volatile void* write_addr,
                           const volatile void* read_addr, uint transfer_count, bool trigger) {
    auto& c = hw.dma[channel];
    c.dreq = config->dreq;
    c.read_increment = config->read_increment;
    c.write_increment = config->write_increment;
    c.write = write_addr;
    c.read = read_addr;
    c.count = transfer_count;
    c.done = false;
    c.started = trigger;
}
void dma_start_channel_mask(uint32_t mask) {
    for (int i = 0; i < 12; i++) {
        if (mask & (1u << i)) hw.dma[i].started = true;
    }
}
bool dma_channel_is_busy(uint channel) {
    auto& c = hw.dma[channel];
    if (!c.started || c.done) return false;
    if (polls_left[channel] < hw.dma_busy_polls) {
        polls_left[channel]++;
        return true;
    }
    polls_left[channel] = 0;
    auto* out = static_cast<volatile uint16_t*>(c.write);
    if (c.dreq == DREQ_ADC) {
        if (!hw.adc_running || !hw.adc_fifo_dreq) return true;
        for (uint32_t i = 0; i < c.count; i++) out[c.write_increment ? i : 0] = adcConvert();
    } else if (c.dreq >= DREQ_SPI0_TX && c.dreq < DREQ_SPI0_TX + 4 && (c.dreq - DREQ_SPI0_TX) % 2 == 1) {
        const uint tx_dreq = c.dreq - 1;
        fake::DmaChannel* tx = nullptr;
        for (auto& other : hw.dma) {
            if (other.claimed && other.started && !other.done && other.dreq == tx_dreq) tx = &other;
        }
        if (tx == nullptr) return true;
        const int index = static_cast<int>(tx_dreq - DREQ_SPI0_TX) / 2;
        const auto* in = static_cast<const volatile uint16_t*>(tx->read);
        for (uint32_t i = 0; i < c.count; i++) {
            out[c.write_increment ? i : 0] = spiTransfer16(index, in[tx->read_increment ? i : 0]);
        }
        tx->done = true;
    } else {
        return true;  // nothing paces it in the fake
    }
    c.done = true;
    return false;
}
void dma_channel_abort(uint channel) {
    hw.dma[channel].started = false;
    polls_left[channel] = 0;
}

// --- pwm / clocks --------------------------------------------------------------------------
uint pwm_gpio_to_slice_num(uint gpio) { return (gpio >> 1) & 7; }
void pwm_set_clkdiv_int_frac4(uint slice, uint8_t integer, uint8_t) { hw.pwm[slice].div_int = integer; }
void pwm_set_wrap(uint slice, uint16_t wrap) { hw.pwm[slice].wrap = wrap; }
void pwm_set_enabled(uint slice, bool enabled) { hw.pwm[slice].enabled = enabled; }
void pwm_set_gpio_level(uint gpio, uint16_t level) { hw.pwm[pwm_gpio_to_slice_num(gpio)].level[gpio & 1] = level; }
uint32_t clock_get_hz(clock_index) { return hw.sys_hz; }

// --- irq / sync ------------------------------------------------------------------------------
void irq_set_exclusive_handler(uint irq, irq_handler_t handler) { hw.irq_handlers[irq] = handler; }
void irq_remove_handler(uint irq, irq_handler_t) { hw.irq_handlers[irq] = nullptr; }
void irq_set_enabled(uint irq, bool enabled) { hw.irq_enabled[irq] = enabled; }
uint32_t save_and_disable_interrupts() { return static_cast<uint32_t>(hw.interrupts_disabled++); }
void restore_interrupts(uint32_t) { hw.interrupts_disabled--; }

// --- uart ---------------------------------------------------------------------------------------
uint uart_init(uart_inst_t* u, uint baud) {
    auto& s = hw.uart[u->index];
    s.enabled = true;
    s.baud = baud;
    return baud;
}
void uart_deinit(uart_inst_t* u) { hw.uart[u->index].enabled = false; }
void uart_set_format(uart_inst_t* u, uint data_bits, uint stop_bits, uart_parity_t parity) {
    auto& s = hw.uart[u->index];
    s.data_bits = data_bits;
    s.stop_bits = stop_bits;
    s.parity = parity;
}
void uart_set_fifo_enabled(uart_inst_t*, bool) {}
void uart_set_irq_enables(uart_inst_t* u, bool rx, bool) { hw.uart[u->index].rx_irq = rx; }
bool uart_is_readable(uart_inst_t* u) { return !hw.uart[u->index].rx.empty(); }
void uart_putc_raw(uart_inst_t* u, char c) { hw.uart[u->index].tx.push_back(c); }
void uart_tx_wait_blocking(uart_inst_t*) {}
uint uart_get_index(uart_inst_t* u) { return static_cast<uint>(u->index); }
fake::UartHw* uart_get_hw(uart_inst_t* u) { return &fake::uart_hw[u->index]; }

// --- spi -------------------------------------------------------------------------------------------
static uint8_t spiTransfer(spi_inst_t* spi, uint8_t out) {
    auto& s = hw.spi[spi->index];
    s.sent.push_back(out);
    return s.device ? s.device(out) : 0;
}
uint spi_init(spi_inst_t* spi, uint baud) {
    hw.spi[spi->index].enabled = true;
    hw.spi[spi->index].baud = baud;
    return baud;
}
void spi_deinit(spi_inst_t* spi) { hw.spi[spi->index].enabled = false; }
uint spi_set_baudrate(spi_inst_t* spi, uint baud) { hw.spi[spi->index].baud = baud; return baud; }
void spi_set_format(spi_inst_t* spi, uint bits, spi_cpol_t cpol, spi_cpha_t cpha, spi_order_t) {
    hw.spi[spi->index].data_bits = bits;
    hw.spi[spi->index].cpol = cpol;
    hw.spi[spi->index].cpha = cpha;
}
int spi_write_blocking(spi_inst_t* spi, const uint8_t* src, size_t len) {
    for (size_t i = 0; i < len; i++) spiTransfer(spi, src[i]);
    return static_cast<int>(len);
}
int spi_read_blocking(spi_inst_t* spi, uint8_t repeated_tx, uint8_t* dst, size_t len) {
    for (size_t i = 0; i < len; i++) dst[i] = spiTransfer(spi, repeated_tx);
    return static_cast<int>(len);
}
int spi_write_read_blocking(spi_inst_t* spi, const uint8_t* src, uint8_t* dst, size_t len) {
    for (size_t i = 0; i < len; i++) dst[i] = spiTransfer(spi, src[i]);  // src == dst is allowed
    return static_cast<int>(len);
}

static uint16_t spiTransfer16(int index, uint16_t out) {
    const uint8_t high = spiTransfer(&fake::spi_instances[index], static_cast<uint8_t>(out >> 8));
    const uint8_t low = spiTransfer(&fake::spi_instances[index], static_cast<uint8_t>(out));
    return static_cast<uint16_t>(high << 8 | low);
}
int spi_write16_read16_blocking(spi_inst_t* spi, const uint16_t* src, uint16_t* dst, size_t len) {
    for (size_t i = 0; i < len; i++) dst[i] = spiTransfer16(spi->index, src[i]);
    return static_cast<int>(len);
}
bool spi_is_readable(spi_inst_t*) { return false; }
uint spi_get_dreq(spi_inst_t* spi, bool is_tx) {
    return DREQ_SPI0_TX + 2u * static_cast<uint>(spi->index) + (is_tx ? 0u : 1u);
}
fake::SpiHw* spi_get_hw(spi_inst_t* spi) { return &fake::spi_hw[spi->index]; }

// --- i2c ---------------------------------------------------------------------------------------------
uint i2c_init(i2c_inst_t* i2c, uint baud) {
    hw.i2c[i2c->index].enabled = true;
    hw.i2c[i2c->index].baud = baud;
    return baud;
}
void i2c_deinit(i2c_inst_t* i2c) { hw.i2c[i2c->index].enabled = false; }
uint i2c_set_baudrate(i2c_inst_t* i2c, uint baud) { hw.i2c[i2c->index].baud = baud; return baud; }
int i2c_write_timeout_us(i2c_inst_t* i2c, uint8_t addr, const uint8_t* src, size_t len, bool nostop, uint) {
    auto& s = hw.i2c[i2c->index];
    return s.on_write ? s.on_write(addr, src, len, nostop) : PICO_ERROR_GENERIC;
}
int i2c_read_timeout_us(i2c_inst_t* i2c, uint8_t addr, uint8_t* dst, size_t len, bool nostop, uint) {
    auto& s = hw.i2c[i2c->index];
    return s.on_read ? s.on_read(addr, dst, len, nostop) : PICO_ERROR_GENERIC;
}
size_t i2c_get_read_available(i2c_inst_t* i2c) { return hw.i2c[i2c->index].slave_rx.size(); }
uint8_t i2c_read_byte_raw(i2c_inst_t* i2c) {
    auto& rx = hw.i2c[i2c->index].slave_rx;
    const uint8_t c = rx.front();
    rx.pop_front();
    return c;
}
void i2c_write_byte_raw(i2c_inst_t* i2c, uint8_t value) { hw.i2c[i2c->index].slave_tx.push_back(value); }
void i2c_slave_init(i2c_inst_t* i2c, uint8_t address, i2c_slave_handler_t handler) {
    hw.i2c[i2c->index].slave_address = address;
    hw.i2c[i2c->index].slave_handler = handler;
}
void i2c_slave_deinit(i2c_inst_t* i2c) { hw.i2c[i2c->index].slave_handler = nullptr; }
