# Software architecture

ESP-IDF 5.5, C++20/23 as selected by the IDF toolchain (`gnu++2b`), FreeRTOS, NVS, LEDC, the native HTTP server, and a Wi-Fi access point. No exceptions are required. No third-party application framework and no CDN.

## Dependency direction

```text
main
  -> machine_controller -> safety_manager, machine_state, feeder, servo,
                           weighing, industrial_io, calibration, settings, diagnostics
  -> web_server -> machine_controller
weighing and industrial_io -> modbus -> rs485_transport
modbus CRC and machine_state have no ESP-IDF dependency
```

`web_server` does not include the LEDC or UART headers. `safety_manager` does not include the I/O driver; it reads a plain snapshot the controller copies in. Device tasks never call the state machine.

## State machine

`machine_transition` is a pure function. The controller is the only caller that feeds it real events. Self-test covers:

- boot → initializing → idle
- cycle start rejected from `idle` and from `ready`
- emergency stop from a fill state, and reset back to `idle` rather than back into that fill state
- process stop from a fill state into `stopping`, then idle
- a fault cannot displace an emergency stop

`ChecksPassed` is the only edge into `ready`. The controller does not send it.

## Settings

NVS namespaces: `machine`, `servo`, `weighcal`, `comms`, `network`. Each value is one versioned blob. A size or schema mismatch replaces that namespace with defaults and leaves the others alone. A corrupt NVS partition format follows the ESP-IDF recovery of erasing the NVS partition; that is logged.

`flags` bit 0 stays set on every machine-settings save in this phase so a placeholder profile cannot be marked as a finished calibration by the panel. Servo and weighing saves force `calibrated` to 0. The validator is `constexpr` and the defaults are `static_assert`ed, so an illegal factory profile does not compile.

Network settings are applied on the next boot. The web password is used on the next request.

## Diagnostics

A 32-event RAM ring stores time since boot, state, error code, a short description, the last weight, and communication flags. It is not written to flash on every event; that would wear the NVS sector for little gain before a real service history is designed. The panel shows the newest 16.

## Web

Assets are `EMBED_TXTFILES` from `web/`. Nothing is fetched at runtime. Routes:

| Method | Path | Auth |
|---|---|---|
| GET | `/`, the five pages, `/site.css`, `/app.js` | No |
| GET | `/api/status`, `/api/settings`, `/api/calibration`, `/api/diagnostics` | No |
| GET | `/ws` | No. Pushes the same status JSON |
| POST | `/api/settings`, `/api/calibration`, `/api/password`, `/api/network`, `/api/command` | HTTP basic, user `operator` |

`start_cycle` is HTTP 409. Calibration commits are HTTP 409. A driver-fault acknowledge is HTTP 409.

## Concurrency rules

- One owner per UART.
- Actuator writes only from the control task and from `init`, which runs before that task starts.
- The status snapshot is published under a mutex. Readers skip rather than block the control loop for long; the take timeout is 20 ms.
- Command queue depth is 8. A full queue rejects the command. Cycle start is not queued.

## Tests

`app_main` runs the state, CRC, settings, and PWM-math self-tests before starting tasks. If one fails, the enables stay in the boot-safe levels and the panel does not start. The same functions are what `test/unit/host_main.cpp` calls. They do not open a UART or a timer.
