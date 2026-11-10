#include "weighing.hpp"

#include "hardware_config.hpp"

#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace feed {

namespace {
constexpr const char* kTag = "weigh";
}

WeighingService::WeighingService() : master_(transport_) {}

Status WeighingService::init(const BusParameters& bus) {
    if (bus.slave_address != 0) {
        ESP_LOGW(kTag, "A slave address is stored, but the QL-MT01 map is not verified so it will not be used");
    }
    Rs485Pins pins = {};
    pins.uart_port = hw::kUartWeight;
    pins.tx_gpio = hw::kGpioWeightTx;
    pins.rx_gpio = hw::kGpioWeightRx;
    pins.baud = bus.baud;
    pins.timeout_ms = bus.timeout_ms;
    return transport_.init(pins);
}

void WeighingService::start() {
    if (started_ || !transport_.installed()) {
        return;
    }
    if (xTaskCreate(&WeighingService::task_entry, "weigh", 4096, this, 5, nullptr) == pdPASS) {
        started_ = true;
    }
}

bool WeighingService::latest(WeightSample& out) const {
    return mailbox_.read(out);
}

void WeighingService::task_entry(void* context) {
    static_cast<WeighingService*>(context)->task();
}

void WeighingService::task() {
    ESP_LOGI(kTag, "Weighing poll disabled until the manufacturer register map is verified");
    while (true) {
        WeightSample sample = {};
        sample.time_ms = static_cast<uint32_t>(esp_timer_get_time() / 1000);
        sample.protocol_verified = hw::kQlMt01RegisterMapVerified;
        sample.communication_ok = false;
        sample.value_valid = false;
        sample.grams = 0;
        if (hw::kQlMt01RegisterMapVerified && hw::kModbusTransmissionEnabled) {
            // The register map is intentionally absent. There is no request to send.
            sample.communication_ok = false;
        }
        (void)master_;
        mailbox_.publish(sample);
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}

}  // namespace feed
