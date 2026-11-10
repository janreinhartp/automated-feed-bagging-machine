# Safety design

## Safety function

The emergency stop and the safety relay remove power from the vibrating feeder without software, without the N4D8B08, and without the web panel. The contactor coil is in that hardwired circuit. A web command, a Modbus timeout, or a locked-up control task cannot keep the motor supplied.

The ESP32 may read a safety-relay auxiliary contact later, on DI6. Phase 1 does not trust that bit. The panel says the feedback is not commissioned so an operator does not mistake `idle` for "the safety circuit has been checked."

## What is not a safety function

| Item | Role |
|---|---|
| DI2 stop button | Process stop, and only after the I/O map is verified. It is late by a Modbus poll |
| Web "request cycle" | Always rejected in this firmware |
| DI6 | Future monitor. Loss of the I/O bus must inhibit motion, not be treated as a healthy safety circuit |
| Servo close command | Mitigation if the controller is alive and servo power is up. Not a safety function |
| Green lamp | Indication only |

## Motion permission

`SafetyManager` computes permission from a snapshot. The phase-1 result is always "not permitted" because `kPhase1MotionEnabled` is false. The control task calls `force_safe()` on the feeder and both servos every 20 ms anyway. The actuator drivers also return `Denied` from their command methods and write duty 0.

Permission can become true only when all of these are true at once: the phase-1 motion constant is enabled, the auxiliary polarity constant is enabled, the I/O snapshot is fresh and verified, and the auxiliary reports the safety circuit healthy. None of those are true in this tree. The web server cannot change them.

## Fault latch and restart

A driver that fails to initialize enters `fault`. Acknowledging that fault from the panel is rejected while the driver is still failed. There is no path from emergency stop or fault back into a filling state. Reset returns to `idle` only, and only when the guard allows it. Power returning does not start a cycle. The state machine boots to `idle` with motion inhibited.

`stopping` is the later controlled stop: feeder off, flow gate closed, no discharge. It is in the transition table so a mid-fill stop does not have to be modeled as a fault. Phase 1 cannot be in a fill state, so it cannot enter `stopping` at run time.

## Warning is not a state

A warning such as "protocols unverified" or "factory password still set" is a flag on the status snapshot. Making it a state would force the machine out of whatever step it was in. Diagnostics record the condition instead.

## Servo hazard

See `POWER_DISTRIBUTION.md`. The open question is whether an unpowered gate rests closed. Until that is decided and built, servo plugs stay off. Firmware must keep distinguishing a commanded pulse from a verified position even after motion is allowed, because there is still no feedback sensor.

## Web

The access point is the network boundary. HTTP basic authentication protects settings, calibration drafts, network changes, the password change, and commands. Status and the diagnostic list are readable by anyone who has the Wi-Fi password. That is an accepted limit of an untunneled local panel, and it is why the factory Wi-Fi password has to change.

No command handler writes GPIO, LEDC, or UART. Cycle start is rejected in the HTTP handler and again in the controller.
