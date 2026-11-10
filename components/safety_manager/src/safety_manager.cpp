#include "safety_manager.hpp"

#include "hardware_config.hpp"
#include "status.hpp"

namespace feed {

namespace {
// Proposed bit for DI6. Both the bit position and the active level come from
// an unverified module manual, so they have no effect until the polarity
// constant in hardware_config is changed and reviewed.
constexpr int kUnverifiedSafetyAuxBit = 5;
constexpr bool kUnverifiedAuxHighMeansHealthy = true;
}  // namespace

SafetyAssessment SafetyManager::evaluate(const SafetyInputs& inputs) const {
    SafetyAssessment assessment = {};
    assessment.feedback_trusted = hw::kSafetyFeedbackPolarityVerified;
    bool healthy = false;

    if (hw::kSafetyFeedbackPolarityVerified) {
        const bool usable = inputs.snapshot_valid && inputs.protocol_verified && inputs.communication_ok &&
                            (inputs.now_ms - inputs.snapshot_ms) <= hw::kIoStaleMs;
        assessment.snapshot_stale = !usable;
        if (usable) {
            const bool bit = (inputs.inputs & (1U << kUnverifiedSafetyAuxBit)) != 0;
            healthy = kUnverifiedAuxHighMeansHealthy ? bit : !bit;
            assessment.emergency_stop_requested = !healthy;
        }
    }

    assessment.motion_permitted =
        hw::kPhase1MotionEnabled && hw::kSafetyFeedbackPolarityVerified && healthy && !assessment.snapshot_stale;

    if (assessment.emergency_stop_requested) {
        copy_text(assessment.inhibit_reason, sizeof(assessment.inhibit_reason), "safety relay auxiliary is open");
    } else if (!hw::kSafetyFeedbackPolarityVerified) {
        copy_text(assessment.inhibit_reason, sizeof(assessment.inhibit_reason),
                  "motion inhibited; safety feedback is not commissioned");
    } else if (assessment.snapshot_stale) {
        copy_text(assessment.inhibit_reason, sizeof(assessment.inhibit_reason), "industrial I/O snapshot is not usable");
    } else if (!assessment.motion_permitted) {
        copy_text(assessment.inhibit_reason, sizeof(assessment.inhibit_reason),
                  "motion inhibited until hardware commissioning");
    }
    return assessment;
}

}  // namespace feed
