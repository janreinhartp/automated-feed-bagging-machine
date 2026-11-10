#pragma once

#include "mailbox.hpp"
#include "modbus_master.hpp"
#include "settings_model.hpp"
#include "status.hpp"

#include <cstdint>

namespace feed {

struct WeightSample {
    uint32_t time_ms;
    bool protocol_verified;
    bool communication_ok;
    bool value_valid;
    int32_t grams;
};

class WeighingService {
public:
    WeighingService();
    Status init(const BusParameters& bus);
    void start();
    bool latest(WeightSample& out) const;

private:
    static void task_entry(void* context);
    void task();

    Rs485Transport transport_;
    ModbusMaster master_;
    Mailbox<WeightSample> mailbox_;
    bool started_ = false;
};

}  // namespace feed
