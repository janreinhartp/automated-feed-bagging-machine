#pragma once

#include <cstdint>

namespace feed {

enum class MachineState : uint8_t {
    Booting = 0,
    Initializing,
    Idle,
    Ready,
    Tare,
    CoarseFill,
    MediumFill,
    FineFill,
    Settling,
    WeightValidation,
    WaitingForBag,
    Discharging,
    VerifyingEmpty,
    WaitingForBagRemoval,
    Stopping,
    Manual,
    Fault,
    EmergencyStop,
};

enum class MachineEvent : uint8_t {
    BootComplete = 0,
    InitComplete,
    ChecksPassed,
    SelectorManual,
    SelectorAutomatic,
    EstopAsserted,
    ResetRequested,
    StopRequested,
    StoppingComplete,
    FaultRaised,
    CycleStartRequested,
};

const char* machine_state_name(MachineState state);
bool is_cycle_state(MachineState state);
bool machine_transition(MachineState from, MachineEvent event, MachineState& to);
bool machine_state_self_test();

}  // namespace feed
