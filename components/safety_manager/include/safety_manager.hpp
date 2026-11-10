#pragma once

#include <cstdint>

namespace feed {

struct SafetyInputs {
    bool snapshot_valid;
    uint32_t snapshot_ms;
    uint32_t now_ms;
    uint8_t inputs;
    bool protocol_verified;
    bool communication_ok;
};

struct SafetyAssessment {
    bool motion_permitted;
    bool emergency_stop_requested;
    bool feedback_trusted;
    bool snapshot_stale;
    char inhibit_reason[64];
};

class SafetyManager {
public:
    SafetyAssessment evaluate(const SafetyInputs& inputs) const;
};

}  // namespace feed
