# Preliminary bill of materials

Quantities are for one machine. Electrical ratings are provisional. Part numbers for the safety circuit are intentionally not invented.

| Qty | Item | Role | Status |
|---|---|---|---|
| 1 | ESP32-S3-DevKitC-1 | Controller | Selected. Module variant unverified |
| 1 | QL-MT01 weighing transmitter | Weight | Selected. Protocol unverified |
| 1 | QLMT display | Local weight display | Selected |
| 1 | Load cell, capacity matched to a 2 kg batch plus hopper tare | Weight | Capacity unverified |
| 1 | N4D8B08 Modbus I/O, 8 inputs, 8 relays | Buttons, sensor, lamps | Selected. 12 V versus 24 V unverified |
| 2 | DIYMORE 3–5 V isolated TTL-RS485 converter, automatic direction | One converter per bus | Selected. Termination and pinout unverified |
| 1 | 74AHCT125 | 3.3 V to 5 V buffer | Selected. Not an isolator |
| 2 | 9iMOD 35 kg class digital servo, 180° | Flow gate and discharge gate | Selected. SKU voltage and stall current unverified |
| 1 | 24 V vibrating feeder motor | Feed delivery | Selected. Stall current unverified |
| 1 | DIYMORE BTS7960B / BTN7960 driver | One-direction PWM | Selected. Continuous current unverified |
| 1 | 24 V photoelectric sensor | Bag present | Selected. PNP versus NPN unverified |
| 1 | 24 V supply, motor branch | Feeder | Provisional 15 A |
| 1 | 24 V supply, control branch | Sensors, lamps, I/O module | Provisional 5 A. May be the same supply with a separate fuse |
| 1 | 7.4 V supply | Servos | Provisional 10 A |
| 1 | 5 V supply | ESP32 and 74AHCT125 | Provisional 3 A |
| 4 | Branch fuses or breakers | Motor, control 24 V, servo, logic | Required. Values not final |
| 1 | AC isolator and upstream overcurrent protection | Incoming supply | Required. Part not selected |
| 1 | Safety relay with force-guided contacts | Hardwired feeder isolation | Required. Part not selected |
| 1 | Contactor, sized above the measured motor current | Motor branch | Required. Part not selected |
| 1 | Mushroom emergency stop, dual channel into the safety relay | Emergency stop | Required. Part not selected |
| 1 | Enclosure, glands, PE bond, coil suppression | Installation | Required. Part not selected |
| — | 100 nF at the 74AHCT125, input pulldowns, output pulldowns, series resistors | Boot-safe buffer | Required. 220 Ω series is provisional |
| 1 | Interposing relay for the bag sensor | Only if the sensor is PNP and the module input is NPN-only | Conditional. Not selected yet |

Lamp and buzzer current is part of the 24 V control branch. The N4D8B08 relays switch that supply; they do not provide it.

No touchscreen, no cloud module, and no second controller are in this list.
