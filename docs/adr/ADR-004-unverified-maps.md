# ADR-004: No invented device registers

Date: 2026-11-11
Status: Accepted

## Context

The brief gives the QL-MT01 baud and format, and it gives the N4D8B08's I/O count. It does not give register addresses, scaling, a stable bit, or the slave addresses. Catalog copies of both devices disagree with each other. A plausible map that happens to match a different module can tare the wrong register or energize a relay.

## Decision

The Modbus CRC and RTU transaction code are generic and tested. Device tasks do not call them. `kQlMt01RegisterMapVerified`, `kN4d8b08RegisterMapVerified`, and `kModbusTransmissionEnabled` are compile-time constants, all false. Stored slave address 0 means unset. The panel shows weight as unavailable, not as zero. Weighing span can be stored as a draft and cannot be marked calibrated.

The proposed DI and DO names are the field allocation, not bit 0 of a guessed register.

## Consequences

The machine cannot weigh or light a lamp until someone transcribes the supplied manual into the device source and changes those constants in review. That delay is the point. A similar-looking online map is not an acceptable source.
