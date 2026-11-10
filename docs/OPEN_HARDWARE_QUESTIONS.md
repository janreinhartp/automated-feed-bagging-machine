# Open hardware questions

Anything marked here is unverified. Firmware treats the related function as absent.

## Controller

1. Which ESP32-S3-WROOM variant is on the DevKit (flash size, and quad versus octal PSRAM)?
2. Is the board DevKitC-1 v1.0 (RGB LED on GPIO48) or v1.1 (RGB LED on GPIO38)?

## Weighing

3. What is the QL-MT01 slave address?
4. Is the protocol Modbus RTU or a manufacturer frame?
5. Which registers or bytes contain weight, and what is the scale?
6. Is there a stable-weight or motion flag, and what is its polarity?
7. What is the tare and calibration command, if the panel is allowed to send one?
8. What load-cell capacity and sensitivity are fitted?

## N4D8B08

9. Is the purchased module the 12 V or the 24 V version?
10. What are the input voltage and current ratings, and is the input NPN-only?
11. What are the relay contact ratings?
12. What is the slave address, baud, parity, and stop bits?
13. What is the register map, including bit order for DI1 and DO1?
14. Is the input-to-relay follow mode enabled from the factory, and which register disables it?
15. Does DI6 need a dry contact, a sink, or a 24 V source?

## Bag sensor

16. Is the photoelectric sensor PNP or NPN, and what is its residual voltage?
17. If it is PNP and the module is NPN-only, which interposing relay will be used?

## RS485 converters

18. What is the exact pinout and logic supply current of the purchased DIYMORE module?
19. Is the 120 Ω termination permanent or jumpered?
20. Does automatic direction pass a scope test at 9600 baud with both slaves?
21. Are the isolated-side grounds still isolated after the machine is fully wired?

## Servos and feeder

22. What are the purchased servo SKU's voltage, stall current, and mechanical travel?
23. Which pulse is closed for each gate, measured, not taken from the 500–2,500 µs advertisement?
24. Does an unpowered gate rest closed, rest open, or drift?
25. Should the 7.4 V rail drop on emergency stop?
26. What is the motor's measured start and stall current?
27. What continuous current does this BTS7960 PCB hold before the heatsink is unacceptable?
28. Does the driver board expose a digital fault, or only the BTN7960 current-sense pin?
29. What is the logic-high threshold of the enable and PWM inputs, and do those pins have 5 V pull-ups?

## Power and safety

30. Final fuse values, wire sizes, contactor, and safety-relay part numbers after the stall-current test and the risk assessment.
31. Where is the single bond between servo return and logic ground, and is motor current kept out of it?

Until 3–7 and 9–15 are answered, the matching `k*Verified` constant stays false.
