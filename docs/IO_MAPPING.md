# Digital I/O mapping

The N4D8B08 is an 8-input, 8-relay Modbus module. Its relay contacts are dry contacts. They do not source voltage, and they are not used for PWM. The feeder PWM, both servo PWMs, and the motor enable stay on the ESP32 side of the 74AHCT125.

The bit index below is the proposed field allocation. It is not copied from a verified register map. Firmware constants name these bits and do not read or write them while `kN4d8b08RegisterMapVerified` is false.

## Inputs

| Proposed input | Proposed bit | Function | Trusted in phase 1 |
|---|---:|---|---|
| DI1 | 0 | Start | No |
| DI2 | 1 | Process stop | No |
| DI3 | 2 | Fault reset | No |
| DI4 | 3 | Auto / manual selector | No |
| DI5 | 4 | Bag photoelectric sensor | No |
| DI6 | 5 | Safety-relay auxiliary feedback | No |
| DI7 | 6 | Spare until a driver fault pin is proven | No |
| DI8 | 7 | Spare | No |

DI2 is a process stop. It is not the emergency stop. The emergency stop never passes through this module.

DI6 is monitoring only. The safety relay has already dropped the feeder supply before this bit can change, and the software will not treat the bit as meaningful until the contact polarity and the module's active level are both measured.

DI7 was proposed for BTS7960 fault feedback. The BTN7960 does not bring out a simple fault digital output; its status is mixed into the current-sense pin. Leave DI7 as a spare until the purchased PCB is inspected. Do not wire a current-sense analog pin into a digital input.

## Outputs

| Proposed relay | Proposed bit | Function | Driven in phase 1 |
|---|---:|---|---|
| DO1 | 0 | Green ready lamp | No |
| DO2 | 1 | Blue running lamp | No |
| DO3 | 2 | Amber warning lamp | No |
| DO4 | 3 | Red fault lamp | No |
| DO5 | 4 | Buzzer | No |
| DO6 | 5 | Auxiliary control contact | No |
| DO7 | 6 | Spare | No |
| DO8 | 7 | Spare | No |

Each lamp and the buzzer need their own supply, switched by the relay contact. The module does not provide that supply.

Some N4D8B08 units ship with an input-follows-output link enabled. That link must be confirmed off before any field wire is landed, or a sensor could close a relay by itself. The register that controls the link is not in this firmware because it has not been taken from the supplied manual.

## Sensor polarity

The selected module is described as NPN, active-low: the input is pulled up and a sensor must sink it. A 24 V PNP photoelectric sensor sources current and is not electrically compatible with that input. Do not connect the bag sensor until the purchased module's input rating and the sensor's output type are both known. An interposing relay is the likely interface if the sensor is PNP and the module is NPN-only. That is a wiring change, not a controller change.

## What does not belong on these relays

- Feeder speed
- Servo position
- BTS7960 enable
- Emergency-stop contactor coil, except as a monitored status elsewhere

Fast signals and the safety path stay off this bus because a Modbus timeout is too slow, and a lost bus must not be able to hold a motor enable or hide an emergency stop.
