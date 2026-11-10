#include "diagnostics.hpp"
#include "hardware_config.hpp"
#include "machine_controller.hpp"
#include "machine_state.hpp"
#include "modbus_crc.hpp"
#include "settings.hpp"
#include "settings_model.hpp"
#include "web_server.hpp"

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace {

constexpr const char* kTag = "app";

bool run_self_tests() {
    const bool state_ok = feed::machine_state_self_test();
    const bool crc_ok = feed::modbus_crc_self_test();
    const bool settings_ok = feed::settings_validate_self_test();
    const bool pwm_ok = feed::hw::pwm_math_self_test();
    if (!state_ok || !crc_ok || !settings_ok || !pwm_ok) {
        ESP_LOGE(kTag, "Self-test failed state=%d crc=%d settings=%d pwm=%d", state_ok, crc_ok, settings_ok, pwm_ok);
        return false;
    }
    ESP_LOGI(kTag, "Self-test passed");
    return true;
}

}  // namespace

extern "C" void app_main(void) {
    feed::hw::boot_safe_outputs();
    feed::diagnostics_init();
    if (!run_self_tests()) {
        feed::diagnostics_record(feed::ErrorCode::SelfTestFailed, 0, 0, 0, "Firmware self-test failed", 0);
        while (true) {
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }

    static feed::SettingsStore settings;
    if (settings.init() != feed::Status::Ok) {
        ESP_LOGW(kTag, "Settings storage failed; safe defaults remain in memory");
        feed::diagnostics_record(feed::ErrorCode::StorageFailed, 0, 0, 0, "NVS settings were not loaded", 0);
    }

    static feed::MachineController controller(settings);
    if (controller.init() != feed::Status::Ok) {
        ESP_LOGE(kTag, "Controller initialized with a driver fault. Motion stays inhibited.");
    }
    controller.start();

    static feed::WebServer web(controller);
    if (web.start() != feed::Status::Ok) {
        ESP_LOGE(kTag, "Local panel did not start. The controller remains in its safe state.");
    }
}
