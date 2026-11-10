#pragma once

#include "mailbox.hpp"
#include "modbus_master.hpp"
#include "settings_model.hpp"
#include "status.hpp"

#include <cstdint>

namespace feed {

// Proposed field allocation. Bit positions are not taken from a verified manual.
inline constexpr int kProposedDiStart = 0;
inline constexpr int kProposedDiStop = 1;
inline constexpr int kProposedDiReset = 2;
inline constexpr int kProposedDiMode = 3;
inline constexpr int kProposedDiBag = 4;
inline constexpr int kProposedDiSafetyAux = 5;
inline constexpr int kProposedDiDriverFault = 6;
inline constexpr int kProposedDiSpare = 7;

inline constexpr int kProposedDoReady = 0;
inline constexpr int kProposedDoRunning = 1;
inline constexpr int kProposedDoWarning = 2;
inline constexpr int kProposedDoFault = 3;
inline constexpr int kProposedDoBuzzer = 4;
inline constexpr int kProposedDoAux = 5;
inline constexpr int kProposedDoSpareA = 6;
inline constexpr int kProposedDoSpareB = 7;

struct IoSnapshot {
    uint32_t time_ms;
    bool protocol_verified;
    bool communication_ok;
    uint8_t inputs;
    uint8_t commanded_outputs;
    bool outputs_driven;
};

class IndustrialIo {
public:
    IndustrialIo();
    Status init(const BusParameters& bus);
    void start();
    bool latest(IoSnapshot& out) const;

private:
    static void task_entry(void* context);
    void task();

    Rs485Transport transport_;
    ModbusMaster master_;
    Mailbox<IoSnapshot> mailbox_;
    bool started_ = false;
};

}  // namespace feed
