#pragma once

#include "settings.hpp"

namespace feed {

class CalibrationService {
public:
    explicit CalibrationService(SettingsStore& settings) : settings_(settings) {}

    const ServoCalibration& servo() const { return settings_.servo(); }
    const WeighingCalibration& weighing() const { return settings_.weighing(); }

    Status save_servo_draft(const ServoCalibration& value);
    Status save_weighing_draft(const WeighingCalibration& value);

private:
    SettingsStore& settings_;
};

}  // namespace feed
