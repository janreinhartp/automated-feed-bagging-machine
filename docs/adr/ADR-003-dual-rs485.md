# ADR-003: Two RS485 buses and no direction GPIO yet

Date: 2026-11-11
Status: Accepted

## Context

The weighing transmitter and the I/O module are independent Modbus slaves. Sharing one transceiver would combine their grounds and their timing. The chosen converter claims automatic direction and galvanic isolation. Automatic direction is a common source of truncated bytes at 9600 baud, so leaving a DE pin unassigned forever would be careless, and wiring it now would ignore the selected part.

## Decision

UART1 is the weighing bus. UART2 is the I/O bus. UART0 stays the console. Each bus has its own converter, its own baud and slave address in NVS, and its own FreeRTOS task. No DE/RE pin is configured. GPIO10 and GPIO11 are named as reserved and are not driven.

A weighing timeout must not delay the I/O poll, which is why the buses are not serviced by one shared task. Two tasks, rather than four or five, is the split that matches the two ports.

## Consequences

Isolation is a wiring rule: isolated grounds stay on their own side. The first protocol bring-up includes a scope check of direction timing. Only a failed check adds a DE GPIO, using the reserved pins and an explicit code change. Phase 1 never writes either UART's TX data register from the device tasks.
