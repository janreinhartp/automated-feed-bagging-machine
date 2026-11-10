#pragma once

#include "machine_controller.hpp"
#include "status.hpp"

namespace feed {

class WebServer {
public:
    explicit WebServer(MachineController& controller) : controller_(controller) {}

    Status start();

    MachineController& controller() { return controller_; }

private:
    MachineController& controller_;
    bool started_ = false;
};

}  // namespace feed
