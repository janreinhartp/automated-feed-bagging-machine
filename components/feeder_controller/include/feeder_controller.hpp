#pragma once

#include "status.hpp"

namespace feed {

class FeederController {
public:
    Status init();
    void force_safe();
    Status set_percent(int percent);

private:
    bool ledc_ready_ = false;
};

}  // namespace feed
