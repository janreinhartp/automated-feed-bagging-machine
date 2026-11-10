# RS485 architecture

Two independent buses, two UART controllers, two isolated transceivers. The buses are not tied together.

| Bus | UART | ESP32 pins | Device | Factory serial setting | Slave address |
|---|---|---|---|---|---|
| Weighing | UART1 | TX GPIO17, RX GPIO18 | QL-MT01 | 9600 8N1, from the project brief | Unset (stored as 0) |
| Industrial I/O | UART2 | TX GPIO15, RX GPIO16 | N4D8B08 | 9600 8N1 is an unverified placeholder | Unset (stored as 0) |

UART0 stays on GPIO43/GPIO44 for the console. No DE or RE GPIO is assigned. The selected DIYMORE converter is specified as automatic-direction and galvanically isolated. GPIO10 and GPIO11 are reserved, not driven, in case a bench test shows that the converter's direction timing does not match this UART.

## Isolation

Each converter has a logic side and an isolated RS485 side.

- Logic ground is the ESP32 ground.
- Isolated ground is that bus's device ground only.
- Do not bond the isolated ground to the ESP32 ground.
- Do not share a converter between the two devices.
- Isolation is void if a shield, a power wire, or a scope ground bridges the barrier.

The 74AHCT125 is not an isolator. It does not protect the RS485 barrier, and the RS485 converters do not protect the servo or motor PWM.

## Termination and wiring

The converter listing shows an onboard 120 Ω resistor. Confirm whether it is always in circuit. Fit one termination at each physical end of a bus and nowhere else. Use twisted pair. Take A/B polarity from the device printing, not from a catalog drawing. Bias resistors, if the module needs them, have to be confirmed per device; this firmware does not assume them.

Supported weighing baud rates in the brief are 9600, 19200, and 38400. The settings validator allows those three on both buses so a later, verified I/O baud can be stored. The I/O default of 9600 is not a measured value. Because phase 1 does not transmit, a wrong stored baud only programs the UART clock.

## Direction and turnaround

Automatic-direction converters derive transmit enable from the TX edge. At 9600 baud a byte is about 1.1 ms, and a Modbus frame gap is about 4 ms. A converter whose enable drops before the last stop bit, or stays enabled into the slave's reply, will fail even when the CRC code is correct. The acceptance test is a known-good request, after the register map exists, with a scope on TX, DE inside the converter if a test point exists, and the slave reply. Do not add a DE GPIO until that test fails.

The transport sets the UART RX timeout to four byte-times so a later transaction can read one RTU frame. `ModbusMaster::transact` returns `Denied` while `kModbusTransmissionEnabled` is false and does not call `write`.

## Ownership

`WeighingService` owns UART1. `IndustrialIo` owns UART2. Each creates one task. No other task may call `uart_write_bytes` on those ports. The generic master is the only code path that would, and the device tasks do not call it.
