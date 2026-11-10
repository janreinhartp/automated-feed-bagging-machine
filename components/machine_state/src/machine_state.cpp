#include "machine_state.hpp"

namespace feed {

const char* machine_state_name(MachineState state) {
    switch (state) {
        case MachineState::Booting:
            return "booting";
        case MachineState::Initializing:
            return "initializing";
        case MachineState::Idle:
            return "idle";
        case MachineState::Ready:
            return "ready";
        case MachineState::Tare:
            return "tare";
        case MachineState::CoarseFill:
            return "coarse_fill";
        case MachineState::MediumFill:
            return "medium_fill";
        case MachineState::FineFill:
            return "fine_fill";
        case MachineState::Settling:
            return "settling";
        case MachineState::WeightValidation:
            return "weight_validation";
        case MachineState::WaitingForBag:
            return "waiting_for_bag";
        case MachineState::Discharging:
            return "discharging";
        case MachineState::VerifyingEmpty:
            return "verifying_empty";
        case MachineState::WaitingForBagRemoval:
            return "waiting_for_bag_removal";
        case MachineState::Stopping:
            return "stopping";
        case MachineState::Manual:
            return "manual";
        case MachineState::Fault:
            return "fault";
        case MachineState::EmergencyStop:
            return "emergency_stop";
    }
    return "unknown";
}

bool is_cycle_state(MachineState state) {
    switch (state) {
        case MachineState::Tare:
        case MachineState::CoarseFill:
        case MachineState::MediumFill:
        case MachineState::FineFill:
        case MachineState::Settling:
        case MachineState::WeightValidation:
        case MachineState::WaitingForBag:
        case MachineState::Discharging:
        case MachineState::VerifyingEmpty:
        case MachineState::WaitingForBagRemoval:
            return true;
        case MachineState::Booting:
        case MachineState::Initializing:
        case MachineState::Idle:
        case MachineState::Ready:
        case MachineState::Stopping:
        case MachineState::Manual:
        case MachineState::Fault:
        case MachineState::EmergencyStop:
            return false;
    }
    return false;
}

bool machine_transition(MachineState from, MachineEvent event, MachineState& to) {
    // The filling sequence has no entry event in this firmware.
    if (event == MachineEvent::CycleStartRequested) {
        return false;
    }
    if (event == MachineEvent::EstopAsserted) {
        if (from == MachineState::EmergencyStop) {
            return false;
        }
        to = MachineState::EmergencyStop;
        return true;
    }
    if (event == MachineEvent::FaultRaised) {
        if (from == MachineState::EmergencyStop || from == MachineState::Fault ||
            from == MachineState::Booting) {
            return false;
        }
        to = MachineState::Fault;
        return true;
    }

    switch (from) {
        case MachineState::Booting:
            if (event == MachineEvent::BootComplete) {
                to = MachineState::Initializing;
                return true;
            }
            break;
        case MachineState::Initializing:
            if (event == MachineEvent::InitComplete) {
                to = MachineState::Idle;
                return true;
            }
            break;
        case MachineState::Idle:
            if (event == MachineEvent::ChecksPassed) {
                to = MachineState::Ready;
                return true;
            }
            if (event == MachineEvent::SelectorManual) {
                to = MachineState::Manual;
                return true;
            }
            break;
        case MachineState::Ready:
            if (event == MachineEvent::SelectorManual) {
                to = MachineState::Manual;
                return true;
            }
            if (event == MachineEvent::StopRequested) {
                to = MachineState::Idle;
                return true;
            }
            break;
        case MachineState::Manual:
            if (event == MachineEvent::SelectorAutomatic || event == MachineEvent::StopRequested) {
                to = MachineState::Idle;
                return true;
            }
            break;
        case MachineState::Stopping:
            if (event == MachineEvent::StoppingComplete) {
                to = MachineState::Idle;
                return true;
            }
            break;
        case MachineState::Fault:
        case MachineState::EmergencyStop:
            if (event == MachineEvent::ResetRequested) {
                to = MachineState::Idle;
                return true;
            }
            break;
        case MachineState::Tare:
        case MachineState::CoarseFill:
        case MachineState::MediumFill:
        case MachineState::FineFill:
        case MachineState::Settling:
        case MachineState::WeightValidation:
        case MachineState::WaitingForBag:
        case MachineState::Discharging:
        case MachineState::VerifyingEmpty:
        case MachineState::WaitingForBagRemoval:
            if (event == MachineEvent::StopRequested) {
                to = MachineState::Stopping;
                return true;
            }
            break;
    }
    return false;
}

bool machine_state_self_test() {
    MachineState next = MachineState::Booting;
    if (!machine_transition(MachineState::Booting, MachineEvent::BootComplete, next) ||
        next != MachineState::Initializing) {
        return false;
    }
    if (!machine_transition(MachineState::Initializing, MachineEvent::InitComplete, next) ||
        next != MachineState::Idle) {
        return false;
    }
    if (machine_transition(MachineState::Idle, MachineEvent::CycleStartRequested, next)) {
        return false;
    }
    if (machine_transition(MachineState::Ready, MachineEvent::CycleStartRequested, next)) {
        return false;
    }
    if (!machine_transition(MachineState::Idle, MachineEvent::ChecksPassed, next) ||
        next != MachineState::Ready) {
        return false;
    }
    if (!machine_transition(MachineState::CoarseFill, MachineEvent::EstopAsserted, next) ||
        next != MachineState::EmergencyStop) {
        return false;
    }
    if (!machine_transition(MachineState::EmergencyStop, MachineEvent::ResetRequested, next) ||
        next != MachineState::Idle) {
        return false;
    }
    if (!machine_transition(MachineState::FineFill, MachineEvent::StopRequested, next) ||
        next != MachineState::Stopping) {
        return false;
    }
    if (!machine_transition(MachineState::Stopping, MachineEvent::StoppingComplete, next) ||
        next != MachineState::Idle) {
        return false;
    }
    if (machine_transition(MachineState::EmergencyStop, MachineEvent::FaultRaised, next)) {
        return false;
    }
    if (is_cycle_state(MachineState::Idle) || is_cycle_state(MachineState::Stopping) ||
        !is_cycle_state(MachineState::Discharging)) {
        return false;
    }
    return machine_state_name(MachineState::EmergencyStop)[0] != '\0';
}

}  // namespace feed
