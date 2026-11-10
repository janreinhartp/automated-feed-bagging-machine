#pragma once

#include <cstddef>
#include <cstdint>

namespace feed {

enum class ErrorCode : uint16_t {
    None = 0,
    BootOutputsForcedSafe = 1,
    SelfTestFailed = 2,
    SafetyFeedbackUnverified = 100,
    WeighingProtocolUnverified = 101,
    IoProtocolUnverified = 102,
    MotionInhibited = 103,
    SequenceNotImplemented = 104,
    EmergencyStop = 200,
    IoCommunicationLost = 201,
    WeighingCommunicationLost = 202,
    DriverInitFailed = 203,
    InvalidSettings = 300,
    StorageFailed = 301,
};

struct DiagnosticEvent {
    uint32_t time_ms;
    uint16_t state;
    uint16_t code;
    int32_t weight_g;
    uint16_t comm_flags;
    uint16_t reserved;
    char description[48];
};

constexpr std::size_t kDiagnosticCapacity = 32;

const char* error_name(ErrorCode code);
void diagnostics_init();
void diagnostics_record(ErrorCode code, uint16_t state, int32_t weight_g, uint16_t comm_flags,
                        const char* description, uint32_t time_ms);
std::size_t diagnostics_count();
bool diagnostics_at(std::size_t newest_index, DiagnosticEvent& out);

}  // namespace feed
