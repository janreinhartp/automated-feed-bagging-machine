#pragma once

#include "status.hpp"

namespace feed {

// Commanded pulse only. This controller has no position feedback and must not
// report a verified physical angle.
class ServoController {
public:
    Status init();
    void force_safe();
    Status command_percent(int servo_index, int percent);

private:
    bool ledc_ready_ = false;
};

}  // namespace feed
