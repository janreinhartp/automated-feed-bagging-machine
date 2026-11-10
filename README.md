# Automated animal feed bagging machine

Local controller firmware for a machine that weighs animal feed and releases a finished batch into a bag held by the operator. The controller is an ESP32-S3 DevKitC-1 running ESP-IDF 5.5 in C++. The machine does not use a cloud service.

This repository is the architecture and the phase-1 firmware skeleton. It builds a controller that boots into a safe idle state, stores settings, and serves a local panel. It does not run the filling sequence, and it does not command the feeder or either servo.

## Safety boundary

The emergency stop is a hardwired safety relay. It must remove power from the vibrating feeder without help from this firmware or from the Modbus I/O module. The ESP32 only monitors an auxiliary contact, and that contact is not commissioned yet.

Phase 1 holds the 74AHCT125 output-enable pins inactive and the motor-enable input low. PWM duty stays at zero. Modbus transmission is compiled out behind a firmware constant. Do not connect servo power or the feeder motor until the bench checks in `docs/DEVELOPMENT_ROADMAP.md` are done. A successful compile is not a hardware test.

Factory access, for a local access point only:

| Item | Factory value |
|---|---|
| SSID | `FeedBagging` |
| Wi-Fi password | `Bagging-Setup-1` |
| Panel address | `http://192.168.4.1/` |
| Web user | `operator` |
| Web password | `operator-setup` |

Change both passwords before any commissioning session. The access point has no TLS, and NVS stores passwords as plaintext. Physical access to the flash is enough to read them.

## Build

Validated toolchain on this development machine: ESP-IDF 5.5.4, target `esp32s3`.

```powershell
. C:\esp\v5.5.4\esp-idf\export.ps1
cd <this repository>
idf.py set-target esp32s3
idf.py build
```

`sdkconfig.defaults` selects a 4 MB flash image so the same binary fits a 4 MB module and can be flashed to a larger WROOM module. Confirm the module's flash size before flashing. The project warns if the detected ESP-IDF version is not 5.5.x.

Host-side checks of the state machine, Modbus CRC, settings validation, and PWM math live in `test/unit`. They also run from `app_main` before the controller starts. The ESP Clang installed here is a RISC-V cross compiler, so it cannot execute that host binary on Windows.

## Layout

```text
main/                  application entry
components/            firmware boundaries
web/                   panel assets stored in flash
docs/                  architecture and hardware notes
docs/adr/              architecture decision records
hardware/              bill of materials and document slots
test/unit              host self-tests
test/integration       intentionally empty
```

Start with `docs/SYSTEM_ARCHITECTURE.md` and `docs/OPEN_HARDWARE_QUESTIONS.md`.

## Phase 1 behavior

On boot the firmware forces the actuator enables safe, runs its logic self-tests, loads versioned NVS settings (or writes defaults), and enters `idle`. A control task keeps the enables safe. Two bus tasks own the two UARTs and publish an unverified sample; they do not transmit. The panel shows status over HTTP and WebSocket. Settings and calibration drafts can be stored. A cycle-start command returns HTTP 409.

`ready` and every filling state exist in the transition table so the later sequence has one place to grow. This firmware never emits the event that enters them.
