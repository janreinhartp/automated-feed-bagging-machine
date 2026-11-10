#include "industrial_io.hpp"

#include "hardware_config.hpp"

#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace feed {

namespace {
constexpr const char* kTag = "io";
}

IndustrialIo::IndustrialIo() : master_(transport_) {}

Status IndustrialIo::init(const BusParameters& bus) {
    if (bus.slave_address != 0) {
        ESP_LOGW(kTag, "A slave address is stored, but the N4D8B08 map is not verified so it will not be used");
    }
    Rs485Pins pins = {};
    pins.uart_port = hw::kUartIo;
    pins.tx_gpio = hw::kGpioIoTx;
    pins.rx_gpio = hw::kGpioIoRx;
    pins.baud = bus.baud;
    pins.timeout_ms = bus.timeout_ms;
    const Status status = transport_.init(pins);
    (void)kProposedDiStart;
    (void)kProposedDoReady;
    return status;
}

void IndustrialIo::start() {
    if (started_ || !transport_.installed()) {
        return;
    }
    if (xTaskCreate(&IndustrialIo::task_entry, "ind_io", 4096, this, 5, nullptr) == pdPASS) {
        started_ = true;
    }
}

bool IndustrialIo::latest(IoSnapshot& out) const {
    return mailbox_.read(out);
}

void IndustrialIo::task_entry(void* context) {
    static_cast<IndustrialIo*>(context)->task();
}

void IndustrialIo::task() {
    ESP_LOGI(kTag, "I/O poll disabled; relay outputs are not driven");
    while (true) {
        IoSnapshot snapshot = {};
        snapshot.time_ms = static_cast<uint32_t>(esp_timer_get_time() / 1000);
        snapshot.protocol_verified = hw::kN4d8b08RegisterMapVerified;
        snapshot.communication_ok = false;
        snapshot.inputs = 0;
        snapshot.commanded_outputs = 0;
        snapshot.outputs_driven = false;
        if (hw::kN4d8b08RegisterMapVerified && hw::kModbusTransmissionEnabled) {
            snapshot.outputs_driven = false;
        }
        (void)master_;
        mailbox_.publish(snapshot);
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

}  // namespace feed
