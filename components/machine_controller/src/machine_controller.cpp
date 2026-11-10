#include "machine_controller.hpp"

#include "hardware_config.hpp"

#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/task.h"

namespace feed {

namespace {
constexpr const char* kTag = "machine";

uint32_t now_ms() {
    return static_cast<uint32_t>(esp_timer_get_time() / 1000);
}
}  // namespace

MachineController::MachineController(SettingsStore& settings)
    : settings_(settings),
      calibration_(settings_),
      feeder_(),
      servo_(),
      weighing_(),
      industrial_io_(),
      safety_(),
      state_(MachineState::Booting),
      commands_(nullptr),
      status_mutex_(nullptr),
      published_(),
      drivers_ok_(false),
      started_(false),
      inhibit_reason_() {
    copy_text(inhibit_reason_, sizeof(inhibit_reason_), "booting");
}

void MachineController::apply(MachineEvent event) {
    MachineState next = state_;
    if (!machine_transition(state_, event, next)) {
        return;
    }
    ESP_LOGI(kTag, "State %s -> %s", machine_state_name(state_), machine_state_name(next));
    state_ = next;
}

void MachineController::note(ErrorCode code, const char* description) {
    const int32_t weight = published_.weight_valid ? published_.weight_g : 0;
    diagnostics_record(code, static_cast<uint16_t>(state_), weight, 0, description, now_ms());
}

Status MachineController::init() {
    status_mutex_ = xSemaphoreCreateMutex();
    commands_ = xQueueCreate(8, sizeof(MachineCommand));
    apply(MachineEvent::BootComplete);

    drivers_ok_ = feeder_.init() == Status::Ok && servo_.init() == Status::Ok &&
                  weighing_.init(settings_.comms().weighing) == Status::Ok &&
                  industrial_io_.init(settings_.comms().industrial_io) == Status::Ok;
    if (drivers_ok_) {
        apply(MachineEvent::InitComplete);
    } else {
        apply(MachineEvent::FaultRaised);
        note(ErrorCode::DriverInitFailed, "A peripheral driver did not initialize");
    }

    note(ErrorCode::BootOutputsForcedSafe, "Buffer disabled and motor enable held low");
    note(ErrorCode::SafetyFeedbackUnverified, "Safety auxiliary polarity is not commissioned");
    note(ErrorCode::WeighingProtocolUnverified, "QL-MT01 register map is not verified");
    note(ErrorCode::IoProtocolUnverified, "N4D8B08 register map is not verified");
    note(ErrorCode::MotionInhibited, "Phase 1 firmware cannot command motion");
    note(ErrorCode::SequenceNotImplemented, "Filling sequence is not implemented");
    copy_text(inhibit_reason_, sizeof(inhibit_reason_), "motion inhibited; safety feedback is not commissioned");
    publish();
    ESP_LOGI(kTag, "Controller at %s. Motion remains inhibited.", machine_state_name(state_));
    return drivers_ok_ ? Status::Ok : Status::ProtocolError;
}

void MachineController::start() {
    if (started_) {
        return;
    }
    weighing_.start();
    industrial_io_.start();
    if (xTaskCreate(&MachineController::task_entry, "control", 8192, this, 6, nullptr) == pdPASS) {
        started_ = true;
    }
}

Status MachineController::submit_command(MachineCommand command) {
    if (command == MachineCommand::StartCycle) {
        note(ErrorCode::SequenceNotImplemented, "Cycle start was rejected");
        return Status::NotImplemented;
    }
    if (commands_ == nullptr || xQueueSend(commands_, &command, 0) != pdTRUE) {
        return Status::Denied;
    }
    return Status::Ok;
}

void MachineController::copy_status(ControllerStatus& out) const {
    if (status_mutex_ == nullptr || xSemaphoreTake(status_mutex_, pdMS_TO_TICKS(20)) != pdTRUE) {
        out = {};
        out.state = state_;
        copy_text(out.state_name, sizeof(out.state_name), machine_state_name(state_));
        copy_text(out.inhibit_reason, sizeof(out.inhibit_reason), inhibit_reason_);
        return;
    }
    out = published_;
    xSemaphoreGive(status_mutex_);
}

void MachineController::publish() {
    ControllerStatus status = {};
    status.state = state_;
    status.motion_permitted = false;
    status.emergency_stop_latched = state_ == MachineState::EmergencyStop;
    status.safety_feedback_trusted = hw::kSafetyFeedbackPolarityVerified;
    status.weighing_protocol_verified = hw::kQlMt01RegisterMapVerified;
    status.io_protocol_verified = hw::kN4d8b08RegisterMapVerified;
    status.weight_valid = false;
    status.weight_g = 0;
    status.drivers_ok = drivers_ok_;
    status.password_change_required = settings_.web_password_is_default();
    status.uptime_ms = now_ms();
    status.completed_bags = 0;
    status.weighing_baud = settings_.comms().weighing.baud;
    status.io_baud = settings_.comms().industrial_io.baud;
    status.weighing_slave = settings_.comms().weighing.slave_address;
    status.io_slave = settings_.comms().industrial_io.slave_address;
    copy_text(status.state_name, sizeof(status.state_name), machine_state_name(state_));
    copy_text(status.inhibit_reason, sizeof(status.inhibit_reason), inhibit_reason_);

    WeightSample weight = {};
    if (weighing_.latest(weight) && weight.value_valid) {
        status.weight_valid = true;
        status.weight_g = weight.grams;
    }
    if (status_mutex_ != nullptr && xSemaphoreTake(status_mutex_, pdMS_TO_TICKS(20)) == pdTRUE) {
        published_ = status;
        xSemaphoreGive(status_mutex_);
    }
}

void MachineController::task_entry(void* context) {
    static_cast<MachineController*>(context)->task();
}

void MachineController::task() {
    TickType_t last_wake = xTaskGetTickCount();
    while (true) {
        IoSnapshot io_snapshot = {};
        const bool have_io = industrial_io_.latest(io_snapshot);
        SafetyInputs inputs = {};
        inputs.snapshot_valid = have_io;
        inputs.snapshot_ms = io_snapshot.time_ms;
        inputs.now_ms = now_ms();
        inputs.inputs = io_snapshot.inputs;
        inputs.protocol_verified = io_snapshot.protocol_verified;
        inputs.communication_ok = io_snapshot.communication_ok;
        const SafetyAssessment assessment = safety_.evaluate(inputs);

        feeder_.force_safe();
        servo_.force_safe();
        if (assessment.emergency_stop_requested && state_ != MachineState::EmergencyStop) {
            apply(MachineEvent::EstopAsserted);
            note(ErrorCode::EmergencyStop, assessment.inhibit_reason);
        }
        copy_text(inhibit_reason_, sizeof(inhibit_reason_), assessment.inhibit_reason);

        MachineCommand command = MachineCommand::AcknowledgeFault;
        while (commands_ != nullptr && xQueueReceive(commands_, &command, 0) == pdTRUE) {
            if (command == MachineCommand::StartCycle) {
                note(ErrorCode::SequenceNotImplemented, "Cycle start was rejected");
                continue;
            }
            if (command != MachineCommand::AcknowledgeFault) {
                continue;
            }
            const bool blocked = (state_ == MachineState::Fault && !drivers_ok_) ||
                                 (state_ == MachineState::EmergencyStop && assessment.emergency_stop_requested);
            if (!blocked) {
                apply(MachineEvent::ResetRequested);
            }
        }

        publish();
        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(hw::kControlPeriodMs));
    }
}

}  // namespace feed
