#include "settings_model.hpp"

namespace feed {

bool settings_validate_self_test() {
    if (validate_machine(default_machine()) != Status::Ok) {
        return false;
    }
    MachineParameters bad = default_machine();
    bad.lower_limit_g = 3000;
    if (validate_machine(bad) == Status::Ok) {
        return false;
    }
    bad = default_machine();
    bad.fine_feeder_percent = 90;
    bad.coarse_feeder_percent = 20;
    if (validate_machine(bad) == Status::Ok) {
        return false;
    }
    ServoCalibration servo = default_servo();
    servo.feeder_gate.calibrated = 1;
    if (validate_servo(servo, false) != Status::Denied) {
        return false;
    }
    servo = default_servo();
    servo.discharge_gate.closed_pulse_us = 400;
    if (validate_servo(servo, false) == Status::Ok) {
        return false;
    }
    WeighingCalibration weighing = default_weighing();
    weighing.calibrated = 1;
    if (validate_weighing(weighing) != Status::Denied) {
        return false;
    }
    CommsParameters comms = default_comms();
    comms.weighing.baud = 115200;
    if (validate_comms(comms) == Status::Ok) {
        return false;
    }
    return validate_network(default_network()) == Status::Ok;
}

}  // namespace feed
