#include "feeder_controller.hpp"

#include "hardware_config.hpp"

#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_log.h"

namespace feed {

namespace {
constexpr const char* kTag = "feeder";
}

Status FeederController::init() {
    force_safe();
    ledc_timer_config_t timer = {};
    timer.speed_mode = LEDC_LOW_SPEED_MODE;
    timer.duty_resolution = LEDC_TIMER_10_BIT;
    timer.timer_num = static_cast<ledc_timer_t>(hw::kLedcFeederTimer);
    timer.freq_hz = hw::kFeederPwmHz;
    timer.clk_cfg = LEDC_AUTO_CLK;
    if (ledc_timer_config(&timer) != ESP_OK) {
        ESP_LOGE(kTag, "Feeder timer init failed");
        return Status::ProtocolError;
    }
    ledc_channel_config_t channel = {};
    channel.gpio_num = hw::kGpioFeederPwm;
    channel.speed_mode = LEDC_LOW_SPEED_MODE;
    channel.channel = static_cast<ledc_channel_t>(hw::kLedcFeederChannel);
    channel.intr_type = LEDC_INTR_DISABLE;
    channel.timer_sel = static_cast<ledc_timer_t>(hw::kLedcFeederTimer);
    channel.duty = 0;
    channel.hpoint = 0;
    channel.sleep_mode = LEDC_SLEEP_MODE_NO_ALIVE_NO_PD;
    channel.flags.output_invert = 0;
    if (ledc_channel_config(&channel) != ESP_OK) {
        ESP_LOGE(kTag, "Feeder channel init failed");
        return Status::ProtocolError;
    }
    ledc_ready_ = true;
    force_safe();
    ESP_LOGI(kTag, "Feeder PWM installed at duty 0 with enable low and the buffer disabled");
    return Status::Ok;
}

void FeederController::force_safe() {
    gpio_set_level(static_cast<gpio_num_t>(hw::kGpioMotorEnable), 0);
    gpio_set_level(static_cast<gpio_num_t>(hw::kGpioFeederBufferOe), 1);
    if (!ledc_ready_) {
        return;
    }
    ledc_set_duty(LEDC_LOW_SPEED_MODE, static_cast<ledc_channel_t>(hw::kLedcFeederChannel), 0);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, static_cast<ledc_channel_t>(hw::kLedcFeederChannel));
}

Status FeederController::set_percent(int percent) {
    (void)percent;
    force_safe();
    return Status::Denied;
}

}  // namespace feed
