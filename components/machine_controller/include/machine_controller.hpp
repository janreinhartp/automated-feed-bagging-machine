#pragma once

#include "calibration.hpp"
#include "diagnostics.hpp"
#include "feeder_controller.hpp"
#include "industrial_io.hpp"
#include "machine_state.hpp"
#include "safety_manager.hpp"
#include "servo_controller.hpp"
#include "settings.hpp"
#include "weighing.hpp"

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"

#include <cstdint>

namespace feed {

enum class MachineCommand : uint8_t {
    AcknowledgeFault = 1,
    StartCycle = 2,
};

struct ControllerStatus {
    MachineState state;
    bool motion_permitted;
    bool emergency_stop_latched;
    bool safety_feedback_trusted;
    bool weighing_protocol_verified;
    bool io_protocol_verified;
    bool weight_valid;
    bool drivers_ok;
    bool password_change_required;
    int32_t weight_g;
    uint32_t uptime_ms;
    uint32_t completed_bags;
    uint32_t weighing_baud;
    uint32_t io_baud;
    uint8_t weighing_slave;
    uint8_t io_slave;
    char state_name[24];
    char inhibit_reason[64];
};

class MachineController {
public:
    explicit MachineController(SettingsStore& settings);

    Status init();
    void start();
    Status submit_command(MachineCommand command);
    void copy_status(ControllerStatus& out) const;

    SettingsStore& settings() { return settings_; }
    CalibrationService& calibration() { return calibration_; }

private:
    static void task_entry(void* context);
    void task();
    void apply(MachineEvent event);
    void publish();
    void note(ErrorCode code, const char* description);

    SettingsStore& settings_;
    CalibrationService calibration_;
    FeederController feeder_;
    ServoController servo_;
    WeighingService weighing_;
    IndustrialIo industrial_io_;
    SafetyManager safety_;
    MachineState state_;
    QueueHandle_t commands_;
    SemaphoreHandle_t status_mutex_;
    ControllerStatus published_;
    bool drivers_ok_;
    bool started_;
    char inhibit_reason_[64];
};

}  // namespace feed
