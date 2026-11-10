# Host self-tests

These tests compile the protocol, state, settings, and PWM math that do not depend on ESP-IDF. The same functions also run from `app_main` before the controller starts.

From the repository root, with a host Clang:

```powershell
clang++ -std=c++17 -Wall -Wextra -Werror `
  -I components/common/include `
  -I components/hardware_config/include `
  -I components/machine_state/include `
  -I components/modbus/include `
  -I components/settings/include `
  test/unit/host_main.cpp `
  components/machine_state/src/machine_state.cpp `
  components/modbus/src/modbus_crc.cpp `
  components/settings/src/settings_model.cpp `
  -o build/host/unit_tests.exe
.\build\host\unit_tests.exe
```

The ESP Clang bundled with ESP-IDF on this machine targets RISC-V and cannot run the resulting binary on Windows. Use a host Clang or GCC for the command above. The same functions run on the ESP32 from `app_main` before the controller starts. Passing them does not exercise UART, LEDC, Wi-Fi, or either RS485 device.
