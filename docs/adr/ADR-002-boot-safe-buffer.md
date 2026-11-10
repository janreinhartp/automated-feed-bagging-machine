# ADR-002: Split buffer enables and a buffered motor enable

Date: 2026-11-11
Status: Accepted

## Context

The first map put feeder PWM, both servo PWMs, and a spare channel through one 74AHCT125, and wired GPIO7 straight to the BTS7960 enable.

ESP32-S3 pins are high-impedance inputs at reset. A 74AHCT125 `/OE` is active low. If `/OE` is floating or low, the buffer drives whatever is on its inputs. Floating inputs plus a floating enable can twitch a servo or run the feeder during boot. Duty 0 is a safe feeder command and an unsafe servo waveform, so one shared `/OE` cannot turn the feeder path on later without also presenting that waveform to the servos.

A direct GPIO7 connection has a second problem. Driver boards often pull logic inputs to 5 V. The ESP32-S3 GPIO absolute maximum is about 3.6 V. The selected buffer was already in the design and can sit between the GPIO and that 5 V node.

## Decision

Keep GPIO4, GPIO5, GPIO6, and GPIO7 as the four ESP32 signals. Add GPIO8 as the servo `/OE` (channels 1 and 2) and GPIO9 as the feeder `/OE` (PWM channel 3 and enable channel 4). Both `/OE` nets have external pull-ups. GPIO7 drives buffer channel 4 rather than the driver pin. The driver's reverse input is hardwired inactive. External pulldowns hold every buffer input and every load input low while the buffer is disabled.

Phase-1 firmware drives the enable low and both `/OE` pins high, and it programs LEDC duty 0. It never enables either buffer.

The selected ICs are unchanged. Channel 4 is no longer a spare; it is the enable. That is a use of the already-chosen buffer, not a new part.

## Consequences

Later motion code must start a valid servo pulse before it may lower GPIO8, and it must not lower GPIO8 just because GPIO9 is in use. The series resistor on channel 4 has to be confirmed against the driver's enable current. GPIO10 and GPIO11 remain the reserved direction pins and were not spent on this.
