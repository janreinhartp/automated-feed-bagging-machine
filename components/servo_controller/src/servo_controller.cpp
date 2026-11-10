#include "servo_controller.hpp"

#include "hardware_config.hpp"

#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_log.h"

namespace feed {

namespace {
constexpr const char* kTag = "servo";

Status configure_channel(int gpio, ledc_channel_t channel) {
    ledc_channel_config_t config = {};
    config.gpio_num = gpio;
    config.speed_mode = LEDC_LOW_SPEED_MODE;
    config.channel = channel;
    config.intr_type = LEDC_INTR_DISABLE;
    config.timer_sel = static_cast<ledc_timer_t>(hw::kLedcServoTimer);
    config.duty = 0;
    config.hpoint = 0;
    config.sleep_mode = LEDC_SLEEP_MODE_NO_ALIVE_NO_PD;
    config.flags.output_invert = 0;
    return ledc_channel_config(&config) == ESP_OK ? Status::Ok : Status::ProtocolError;
}
}  // namespace

Status ServoController::init() {
    force_safe();
    ledc_timer_config_t timer = {};
    timer.speed_mode = LEDC_LOW_SPEED_MODE;
    timer.duty_resolution = LEDC_TIMER_14_BIT;
    timer.timer_num = static_cast<ledc_timer_t>(hw::kLedcServoTimer);
    timer.freq_hz = hw::kServoPwmHz;
    timer.clk_cfg = LEDC_AUTO_CLK;
    if (ledc_timer_config(&timer) != ESP_OK) {
        ESP_LOGE(kTag, "Servo timer init failed");
        return Status::ProtocolError;
    }
    if (configure_channel(hw::kGpioServo1Pwm, static_cast<ledc_channel_t>(hw::kLedcServo1Channel)) != Status::Ok ||
        configure_channel(hw::kGpioServo2Pwm, static_cast<ledc_channel_t>(hw::kLedcServo2Channel)) != Status::Ok) {
        ESP_LOGE(kTag, "Servo channel init failed");
        return Status::ProtocolError;
    }
    ledc_ready_ = true;
    force_safe();
    ESP_LOGI(kTag, "Servo PWM installed at duty 0 with the buffer disabled");
    return Status::Ok;
}

void ServoController::force_safe() {
    gpio_set_level(static_cast<gpio_num_t>(hw::kGpioServoBufferOe), 1);
    if (!ledc_ready_) {
        return;
    }
    ledc_set_duty(LEDC_LOW_SPEED_MODE, static_cast<ledc_channel_t>(hw::kLedcServo1Channel), 0);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, static_cast<ledc_channel_t>(hw::kLedcServo1Channel));
    ledc_set_duty(LEDC_LOW_SPEED_MODE, static_cast<ledc_channel_t>(hw::kLedcServo2Channel), 0);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, static_cast<ledc_channel_t>(hw::kLedcServo2Channel));
}

Status ServoController::command_percent(int servo_index, int percent) {
    (void)servo_index;
    (void)percent;
    force_safe();
    return Status::Denied;
}

}  // namespace feed
