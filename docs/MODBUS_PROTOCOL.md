# Modbus and device protocols

The generic RTU layer is implemented. Neither device map is.

## Generic master

`modbus_crc` is the standard Modbus CRC-16 (polynomial 0xA001, initial value 0xFFFF, low byte first on the wire). The self-test checks the vector `01 03 00 00 00 0A` → CRC `C5 CD`, and the ASCII vector `123456789` → `0x4B37`. Those vectors prove the CRC. They are not QL-MT01 or N4D8B08 frames, and the firmware does not send them.

`ModbusMaster::transact` appends that CRC, flushes the receiver, writes, and checks the reply address, exception bit, and CRC. The whole body is unreachable while `kModbusTransmissionEnabled` is false. There is no public method that sets the flag true. Turning transmission on is a source change.

Slave address 0 is stored as "unset" and is rejected by the frame builder. Broadcast is not used.

## QL-MT01

Known from the brief, and programmed as the UART default:

- 9600 baud, 8 data bits, no parity, 1 stop bit
- Also capable of 19200 and 38400

Not known, and therefore not present as registers, scale factors, or commands:

- Slave address
- Whether the device speaks Modbus RTU or a private protocol
- Holding-register addresses for weight
- Signedness, byte order, and decimal places
- A stable-weight or motion bit
- Tare and span command frames
- Calibration permission and password

`WeighingService` publishes `protocol_verified = false`, `value_valid = false`. The panel shows the weight as unavailable rather than 0 g, so an empty reading is not mistaken for an empty hopper. The stored span and zero are drafts. `calibrated` cannot be set, and nothing multiplies a raw count by a span.

When the manual is in hand, add a register struct next to the weighing driver and only then set `kQlMt01RegisterMapVerified`. Do not guess addresses from a similar transmitter.

## N4D8B08

Not known:

- 12 V or 24 V supply version
- Slave address and baud
- Input and coil addresses, and whether bit 0 is DI1
- Active level in the register versus the electrical level
- The register that disables input-to-relay following
- Contact ratings

`IndustrialIo` publishes `outputs_driven = false` and does not write coils. The proposed DI and DO names in `industrial_io.hpp` are the field allocation from `IO_MAPPING.md`, not a claim about bit 0 of register 0.

## Communication health

Each snapshot carries `protocol_verified` and `communication_ok`. Both are false in phase 1. When transmission exists, a missed reply or a bad CRC sets `communication_ok` false, the control task treats the snapshot as stale, and motion stays inhibited. A stale safety snapshot must not be read as "auxiliary healthy."
