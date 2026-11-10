#pragma once

#include "settings_model.hpp"

namespace feed {

class SettingsStore {
public:
    Status init();

    const MachineParameters& machine() const { return machine_; }
    const ServoCalibration& servo() const { return servo_; }
    const WeighingCalibration& weighing() const { return weighing_; }
    const CommsParameters& comms() const { return comms_; }
    const NetworkParameters& network() const { return network_; }

    Status save_machine(const MachineParameters& value);
    Status save_servo(const ServoCalibration& value);
    Status save_weighing(const WeighingCalibration& value);
    Status save_comms(const CommsParameters& value);
    Status save_network(const NetworkParameters& value);
    Status save_web_password(const char* next_password);

    bool web_password_matches(const char* candidate) const;
    bool web_password_is_default() const { return network_.web_password_is_default != 0; }

private:
    MachineParameters machine_ = default_machine();
    ServoCalibration servo_ = default_servo();
    WeighingCalibration weighing_ = default_weighing();
    CommsParameters comms_ = default_comms();
    NetworkParameters network_ = default_network();
};

}  // namespace feed
