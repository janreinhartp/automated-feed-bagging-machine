#include "rs485_transport.hpp"

#include "driver/uart.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"

namespace feed {

namespace {
constexpr const char* kTag = "rs485";
}

Status Rs485Transport::init(const Rs485Pins& pins) {
    if (installed_ || pins.uart_port <= 0 || pins.baud == 0) {
        return Status::InvalidArgument;
    }
    uart_config_t config = {};
    config.baud_rate = static_cast<int>(pins.baud);
    config.data_bits = UART_DATA_8_BITS;
    config.parity = UART_PARITY_DISABLE;
    config.stop_bits = UART_STOP_BITS_1;
    config.flow_ctrl = UART_HW_FLOWCTRL_DISABLE;
    config.source_clk = UART_SCLK_DEFAULT;

    const uart_port_t port = static_cast<uart_port_t>(pins.uart_port);
    esp_err_t err = uart_driver_install(port, 256, 256, 0, nullptr, 0);
    if (err == ESP_OK) {
        err = uart_param_config(port, &config);
    }
    if (err == ESP_OK) {
        err = uart_set_pin(port, pins.tx_gpio, pins.rx_gpio, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    }
    if (err == ESP_OK) {
        err = uart_set_rx_timeout(port, 4);
    }
    if (err != ESP_OK) {
        ESP_LOGE(kTag, "UART%d init failed", pins.uart_port);
        return Status::ProtocolError;
    }
    uart_port_ = pins.uart_port;
    baud_ = pins.baud;
    installed_ = true;
    ESP_LOGI(kTag, "UART%d ready at %lu baud, TX GPIO%d RX GPIO%d, direction pin not used", pins.uart_port,
             static_cast<unsigned long>(pins.baud), pins.tx_gpio, pins.rx_gpio);
    return Status::Ok;
}

Status Rs485Transport::write(const uint8_t* data, std::size_t length) {
    if (!installed_ || data == nullptr || length == 0) {
        return Status::InvalidArgument;
    }
    const int written = uart_write_bytes(static_cast<uart_port_t>(uart_port_), data, length);
    if (written != static_cast<int>(length)) {
        return Status::ProtocolError;
    }
    if (uart_wait_tx_done(static_cast<uart_port_t>(uart_port_), pdMS_TO_TICKS(100)) != ESP_OK) {
        return Status::Timeout;
    }
    return Status::Ok;
}

Status Rs485Transport::read(uint8_t* data, std::size_t capacity, std::size_t& out_length, uint32_t timeout_ms) {
    out_length = 0;
    if (!installed_ || data == nullptr || capacity == 0) {
        return Status::InvalidArgument;
    }
    const int received = uart_read_bytes(static_cast<uart_port_t>(uart_port_), data, capacity, pdMS_TO_TICKS(timeout_ms));
    if (received < 0) {
        return Status::ProtocolError;
    }
    if (received == 0) {
        return Status::Timeout;
    }
    out_length = static_cast<std::size_t>(received);
    return Status::Ok;
}

void Rs485Transport::flush_input() {
    if (installed_) {
        uart_flush_input(static_cast<uart_port_t>(uart_port_));
    }
}

}  // namespace feed
