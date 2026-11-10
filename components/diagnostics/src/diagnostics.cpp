#include "diagnostics.hpp"

#include "status.hpp"

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

namespace feed {

namespace {
DiagnosticEvent events_[kDiagnosticCapacity] = {};
std::size_t count_ = 0;
std::size_t next_ = 0;
SemaphoreHandle_t mutex_ = nullptr;

void copy_event(DiagnosticEvent& out, const DiagnosticEvent& in) {
    out = in;
}
}  // namespace

const char* error_name(ErrorCode code) {
    switch (code) {
        case ErrorCode::None:
            return "none";
        case ErrorCode::BootOutputsForcedSafe:
            return "boot_outputs_forced_safe";
        case ErrorCode::SelfTestFailed:
            return "self_test_failed";
        case ErrorCode::SafetyFeedbackUnverified:
            return "safety_feedback_unverified";
        case ErrorCode::WeighingProtocolUnverified:
            return "weighing_protocol_unverified";
        case ErrorCode::IoProtocolUnverified:
            return "io_protocol_unverified";
        case ErrorCode::MotionInhibited:
            return "motion_inhibited";
        case ErrorCode::SequenceNotImplemented:
            return "sequence_not_implemented";
        case ErrorCode::EmergencyStop:
            return "emergency_stop";
        case ErrorCode::IoCommunicationLost:
            return "io_communication_lost";
        case ErrorCode::WeighingCommunicationLost:
            return "weighing_communication_lost";
        case ErrorCode::DriverInitFailed:
            return "driver_init_failed";
        case ErrorCode::InvalidSettings:
            return "invalid_settings";
        case ErrorCode::StorageFailed:
            return "storage_failed";
    }
    return "unknown";
}

void diagnostics_init() {
    if (mutex_ == nullptr) {
        mutex_ = xSemaphoreCreateMutex();
    }
}

void diagnostics_record(ErrorCode code, uint16_t state, int32_t weight_g, uint16_t comm_flags,
                        const char* description, uint32_t time_ms) {
    if (mutex_ == nullptr) {
        return;
    }
    if (xSemaphoreTake(mutex_, pdMS_TO_TICKS(20)) != pdTRUE) {
        return;
    }
    DiagnosticEvent& slot = events_[next_];
    slot = {};
    slot.time_ms = time_ms;
    slot.state = state;
    slot.code = static_cast<uint16_t>(code);
    slot.weight_g = weight_g;
    slot.comm_flags = comm_flags;
    copy_text(slot.description, sizeof(slot.description), description);
    next_ = (next_ + 1) % kDiagnosticCapacity;
    if (count_ < kDiagnosticCapacity) {
        ++count_;
    }
    xSemaphoreGive(mutex_);
}

std::size_t diagnostics_count() {
    return count_;
}

bool diagnostics_at(std::size_t newest_index, DiagnosticEvent& out) {
    if (mutex_ == nullptr || newest_index >= count_) {
        return false;
    }
    if (xSemaphoreTake(mutex_, pdMS_TO_TICKS(20)) != pdTRUE) {
        return false;
    }
    const std::size_t latest = (next_ + kDiagnosticCapacity - 1) % kDiagnosticCapacity;
    const std::size_t index = (latest + kDiagnosticCapacity - newest_index) % kDiagnosticCapacity;
    copy_event(out, events_[index]);
    xSemaphoreGive(mutex_);
    return true;
}

}  // namespace feed
