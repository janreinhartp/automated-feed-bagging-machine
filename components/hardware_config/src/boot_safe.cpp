#include "hardware_config.hpp"

#include "driver/gpio.h"
#include "esp_log.h"

namespace feed::hw {

namespace {

constexpr const char* kTag = "boot";

void drive_output(gpio_num_t pin, int level, bool pull_up, bool pull_down) {
    gpio_config_t config = {};
    config.pin_bit_mask = 1ULL << static_cast<unsigned>(pin);
    config.mode = GPIO_MODE_OUTPUT;
    config.pull_up_en = pull_up ? GPIO_PULLUP_ENABLE : GPIO_PULLUP_DISABLE;
    config.pull_down_en = pull_down ? GPIO_PULLDOWN_ENABLE : GPIO_PULLDOWN_DISABLE;
    config.intr_type = GPIO_INTR_DISABLE;
    gpio_config(&config);
    gpio_set_level(pin, level);
}

}  // namespace

void boot_safe_outputs() {
    // External resistors are the protection during reset. These levels take
    // over as soon as the application starts, before LEDC or UART setup.
    drive_output(static_cast<gpio_num_t>(kGpioMotorEnable), 0, false, true);
    drive_output(static_cast<gpio_num_t>(kGpioServoBufferOe), 1, true, false);
    drive_output(static_cast<gpio_num_t>(kGpioFeederBufferOe), 1, true, false);
    ESP_LOGI(kTag, "Actuator buffer disabled and motor enable held low");
}

}  // namespace feed::hw
