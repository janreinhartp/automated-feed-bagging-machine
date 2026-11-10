# Hardware design

Selected hardware is unchanged. The wiring around the buffer and the motor enable was changed because a direct 3.3 V connection can be damaged by a 5 V driver input, and a shared buffer enable cannot keep the servos dark while the feeder PWM is alive. Those changes are in ADR-002.

## Signal chain

```text
ESP32-S3 GPIO  --3.3 V-->  74AHCT125 input  --5 V-->  servo or BTS7960 input
                 pulldown                         series resistor and pulldown
```

The 74AHCT125 is powered from the 5 V logic rail, with 100 nF at the package. Its inputs are TTL thresholds, so a 3.3 V high is a valid high. A 74HC125 at 5 V would not be a safe substitute. The part is a buffer, not an isolator. ESP32 ground and buffer ground are the same ground.

| Buffer channel | Input | Output | `/OE` |
|---|---|---|---|
| 1 | GPIO5 servo 1 PWM | Servo 1 signal | GPIO8, active low |
| 2 | GPIO6 servo 2 PWM | Servo 2 signal | GPIO8, active low |
| 3 | GPIO4 feeder PWM | BTS7960 forward PWM | GPIO9, active low |
| 4 | GPIO7 motor enable | BTS7960 R_EN and L_EN | GPIO9, active low |

Every buffer input has a pulldown to logic ground so a high-impedance ESP32 pin during reset reads as low. Every buffer output has a pulldown at the load and a series resistor. 220 Ω is the provisional series value; confirm it against the load's input current. Channel `/OE` pins are pulled up so the outputs are high-impedance until firmware drives them low. Firmware in this phase never drives them low.

The reverse PWM input of the BTS7960 is tied inactive at the driver. Firmware has no direction pin. The feeder can only be commanded in the wired direction, and only after a later phase enables the buffer.

## Servos

Two 9iMOD-class 35 kg, 180-degree digital servos.

| Servo | Mechanism |
|---|---|
| 1 | Vibrating-feeder outlet gate |
| 2 | Weighing-hopper discharge gate |

Initial pulse timing from the brief, not from a measured horn travel: 50 Hz, center 1,500 µs, advertised range 500–2,500 µs. The stored draft window is narrower (1,000–2,000 µs) and both endpoints are stored at 1,500 µs with `calibrated = 0`. That is a clamp against an accidental extreme value, not a measurement of safe travel. The driver ignores it and writes duty 0.

Servo current is not taken from the ESP32 3.3 V or 5 V pin. The 7.4 V rail is separate. Its ground joins logic ground at one point so PWM return current does not wander through the ESP32 ground. There is no position feedback. Software stores a commanded pulse and must not display it as a measured angle.

Only one gate is allowed to be commanded open at a time, once motion exists. Phase 1 commands neither.

## Feeder driver

The vibrating feeder is a 24 V motor, expected running current 5–10 A, driven by a DIYMORE BTS7960B / BTN7960 module in one direction. PWM is 1 kHz. The module's advertised 43 A figure is a device headline, not a continuous rating of this PCB and heatsink. Stall current, case temperature, and the fuse all have to be measured before the motor is left running.

The enable is active high at the driver and is pulled down. A floating enable is a motor-on fault. Boot, reset, fault, and the phase-1 control loop all hold it low, and the buffer that delivers it stays disabled.

## Weighing

QL-MT01 transmitter, QLMT display, and a compatible load cell, on their own isolated RS485 bus. The brief fixes 9600 8N1 and the three baud rates. It does not fix the register map, the byte order, the scale factor, or a stable-weight flag. Those stay out of the firmware until the manual is transcribed. See `MODBUS_PROTOCOL.md`.

## Industrial I/O

N4D8B08, own isolated RS485 bus. Confirm 12 V versus 24 V supply, input thresholds, relay contact ratings, slave address, baud, and whether the input-to-relay link is on. Treat the relays as dry contacts. See `IO_MAPPING.md`.

## Controller

ESP32-S3 DevKitC-1. Do not power it from USB and from the 5 V pin at the same time. Logic 5 V feeds the DevKit 5 V pin and the 74AHCT125. The two RS485 modules take their logic-side supply from that same 5 V rail and their isolated side from the field bus.

## Enclosure rules that are part of the design

- Main AC isolator and overcurrent protection ahead of the DC supplies.
- Separate fuses for the motor branch, the 24 V control branch, the servo branch, and the 5 V logic branch. Provisional ratings are in `POWER_DISTRIBUTION.md` and are not final.
- Motor wiring physically separated from signal wiring.
- Protective earth bonded to the enclosure.
- Cable glands on every entry.
- Surge suppression across contactor coils and the motor.

No conductor size is frozen here. Size each branch from the measured current, the length, and the installation method after the motor stall test.
