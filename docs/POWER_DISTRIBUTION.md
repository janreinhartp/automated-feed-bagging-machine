# Power distribution

Ratings below are the provisional planning values from the project brief. They are not a finished electrical specification and they are not a substitute for the measured stall current.

| Supply | Load | Provisional rating | Switched by the safety relay |
|---|---|---|---|
| 24 VDC | Vibrating feeder motor, through the BTS7960 | 15 A branch | Yes |
| 24 VDC | Sensors, lamps, I/O module, relay loads | 5 A branch | No |
| 7.4 VDC | Two servos | 10 A branch | Open decision, see below |
| 5 VDC | ESP32-S3 and 74AHCT125 logic | 3 A branch | No |

The motor branch and the control branch are separate fuses on the 24 V source. A motor fault must not drop the controller, the lamps, or the weighing transmitter.

## What the emergency stop removes

The safety relay opens the motor-branch contactor. The ESP32, the 5 V logic, the control 24 V, and the RS485 devices stay powered so the panel can show a fault and so the auxiliary contact can be read later. The relay coil and the contactor coil are hardwired. No GPIO, no Modbus coil, and no web request is in that circuit.

The ESP32 5 V rail must not be taken from a supply that the safety relay kills. If the controller dies with the motor, it cannot record the stop, and a brown-out during the stop is an extra reset transient on the PWM pins.

## Servo supply during an emergency stop

This is unresolved on purpose.

If the 7.4 V rail drops with the safety relay, both digital servos go limp. Without a spring or a mechanical bias, a gate can fall open or closed. If the 7.4 V rail stays up, a firmware fault can hold a gate at the last commanded pulse. Phase 1 never enables the servo buffer, so the second case does not yet exist, but the mechanical choice still has to be made before servo power is applied.

The recommendation to take into the risk assessment:

1. The feeder 24 V always drops on emergency stop.
2. Prefer gates that rest closed when the servo is unpowered, and drop the 7.4 V rail with the safety relay.
3. If the mechanism cannot rest closed, keep servo power up and add a hardware pulse source, independent of the ESP32, that drives the closed pulse while the safety relay is open. That source is not in the selected bill of materials. Do not pretend the firmware is that source.

Software may later command both gates closed when it sees the auxiliary contact. That is mitigation for an ESP32 that is still alive. It is not the safety function.

## Grounds

| Domain | Joined to |
|---|---|
| Protective earth | Enclosure and supply PE terminals |
| Logic ground | ESP32, 74AHCT125, converter logic sides, one point on the servo return |
| Servo power return | Servo supply, then a single bond to logic ground |
| Motor power return | BTS7960 power ground and the 24 V motor return. Do not use the logic ground as the motor current path |
| Weighing RS485 isolated ground | QL-MT01 side only |
| I/O RS485 isolated ground | N4D8B08 side only |

A USB cable plugged into a PC while the machine grounds are bonded can defeat the isolation or create a ground loop. Develop with one ground reference at a time, and do not use a scope ground on the isolated side while its ground clip is on the ESP32 ground.

## Logic versus motor

The BTS7960 logic inputs are driven from the 5 V buffer. The motor power pins are the switched 24 V branch. Losing the 5 V rail must leave the driver disabled; the external pulldown on enable is what does that, not the ESP32.
