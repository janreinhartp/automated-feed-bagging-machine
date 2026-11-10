#include "calibration.hpp"

namespace feed {

Status CalibrationService::save_servo_draft(const ServoCalibration& value) {
    ServoCalibration draft = value;
    draft.feeder_gate.calibrated = 0;
    draft.discharge_gate.calibrated = 0;
    return settings_.save_servo(draft);
}

Status CalibrationService::save_weighing_draft(const WeighingCalibration& value) {
    WeighingCalibration draft = value;
    draft.calibrated = 0;
    return settings_.save_weighing(draft);
}

}  // namespace feed
