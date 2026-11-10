#include "hardware_config.hpp"
#include "machine_state.hpp"
#include "modbus_crc.hpp"
#include "settings_model.hpp"

#include <cstdio>

int main() {
    int failed = 0;
    if (!feed::machine_state_self_test()) {
        std::puts("machine_state self-test failed");
        ++failed;
    }
    if (!feed::modbus_crc_self_test()) {
        std::puts("modbus crc self-test failed");
        ++failed;
    }
    if (!feed::settings_validate_self_test()) {
        std::puts("settings validation self-test failed");
        ++failed;
    }
    if (!feed::hw::pwm_math_self_test()) {
        std::puts("pwm math self-test failed");
        ++failed;
    }
    if (failed == 0) {
        std::puts("host self-tests passed");
    }
    return failed == 0 ? 0 : 1;
}
