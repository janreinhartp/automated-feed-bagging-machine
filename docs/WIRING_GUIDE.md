# Wiring guide

This is the wiring architecture. It is not a cable schedule, and it is not permission to energize the machine. Land power and the safety circuit first. Leave the motor unplugged and the servo plugs off until each signal has been measured at the buffer.

## Power landing order

1. Protective earth to the enclosure, the supply frames, and the door bond.
2. AC isolator and branch protection into the DC supplies.
3. 24 V motor fuse, then the safety-relay contact, then the contactor, then the BTS7960 power input. The motor wires stay off the terminal until the stall-current test.
4. 24 V control fuse to the lamp supply, the I/O-module supply if it is the 24 V version, and the sensor supply. Do not assume the module is 24 V.
5. 7.4 V servo fuse to a distribution block. Servo plugs stay disconnected.
6. 5 V logic fuse to the DevKit 5 V pin and the 74AHCT125 VCC. Do not also power the DevKit from USB once this supply is connected.

## Buffer

- VCC 5 V, ground common with the ESP32, 100 nF at the IC.
- Inputs: GPIO4, GPIO5, GPIO6, GPIO7, each with a pulldown.
- Outputs: series resistor, then the load, with a pulldown on the load side.
- Servo `/OE` pins tied to GPIO8 and pulled up.
- Feeder PWM `/OE` and enable `/OE` tied to GPIO9 and pulled up.
- BTS7960 reverse PWM input tied to the driver's inactive level, not to a GPIO.

## RS485

One DIYMORE isolated converter per bus. Logic side to the ESP32 UART and logic 5 V. Isolated side to that device only, twisted pair, A/B as marked on the device. Confirm whether the converter's 120 Ω resistor is permanent before adding another. No DE wire.

## Field I/O

Do not land the photoelectric sensor until its output type matches the module input, or until an interposing relay is fitted. Buttons are ordinary inputs on the control supply, through the module, and are not safety devices. Lamp commons go to the control supply; lamp switched legs go through the relay contacts.

The safety-relay auxiliary is a dry contact into DI6 only after the input electrical type is known. It is a monitor. The relay's safety contacts are the ones in the motor contactor circuit.

## Points to measure before any actuator power

| Point | Expected with phase-1 firmware and power on the logic rail only |
|---|---|
| GPIO8 and the servo `/OE` | High |
| GPIO9 and the feeder `/OE` | High |
| Buffer outputs | Pulled low by the load pulldowns, not driven |
| GPIO7 | Low |
| BTS7960 enable | Low |
| UART TX pins | Idle high, no bursts |
| Motor 24 V after the contactor | Absent while the safety relay is open, present only when the safety circuit is healthy and the motor is still unplugged |
