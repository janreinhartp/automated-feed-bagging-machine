# GPIO mapping

Target board: ESP32-S3-DevKitC-1. The module variant (flash size and octal PSRAM) still has to be read off the shield. This map avoids pins that are unsafe on both v1.0 and v1.1, and on both quad-flash and octal-PSRAM modules.

## Pins that are not used

| GPIO | Why it stays free |
|---|---|
| 0, 3, 45, 46 | Strapping pins sampled at reset |
| 19, 20 | Native USB D− and D+ |
| 43, 44 | UART0 console on the board's UART header |
| 26–37 | In-package flash, and 35–37 also octal PSRAM on N8R8 / N16R8 |
| 38 | RGB LED on DevKitC-1 v1.1 |
| 48 | RGB LED on the original DevKitC-1 |

GPIO15 and GPIO16 are the 32 kHz crystal pads. This design does not enable an external crystal, so they are available for UART2. Confirm `sdkconfig` never turns on an external 32 kHz crystal on those pins.

## Assigned pins

This table is the same assignment as `components/hardware_config/include/hardware_config.hpp`. A `static_assert` rejects duplicates and forbidden pins.

| GPIO | Function | Peripheral | Safe level after boot |
|---|---|---|---|
| 4 | Feeder PWM into 74AHCT125 channel 3 | LEDC timer 1, channel 2, 1 kHz, 10-bit | Duty 0 |
| 5 | Servo 1 PWM into channel 1 | LEDC timer 0, channel 0, 50 Hz, 14-bit | Duty 0, buffer disabled |
| 6 | Servo 2 PWM into channel 2 | LEDC timer 0, channel 1, 50 Hz, 14-bit | Duty 0, buffer disabled |
| 7 | Motor enable into channel 4 | GPIO output | Low |
| 8 | Servo buffer `/OE` for channels 1 and 2 | GPIO output | High (disabled). External pull-up required |
| 9 | Feeder buffer `/OE` for channels 3 and 4 | GPIO output | High (disabled). External pull-up required |
| 15 | N4D8B08 RS485 TX | UART2 TX | UART idle |
| 16 | N4D8B08 RS485 RX | UART2 RX | Input |
| 17 | QL-MT01 RS485 TX | UART1 TX | UART idle |
| 18 | QL-MT01 RS485 RX | UART1 RX | Input |

Reserved and not driven:

| GPIO | Reservation |
|---|---|
| 10 | Optional DE/RE for the weighing converter, only if automatic direction fails on the bench |
| 11 | Optional DE/RE for the I/O converter, on the same condition |

No firmware in this tree configures GPIO10 or GPIO11.

## Changes from the first proposal

The original list (GPIO4, 5, 6, 7, 15, 16, 17, 18) is kept. Two pins were added, and the enable path was moved behind the buffer.

GPIO8 and GPIO9 are separate output enables. A single `/OE` for every 74AHCT125 channel would present the servo lines whenever the feeder PWM was enabled. Duty 0 is a legal "motor off" PWM value and an illegal servo frame, so the servo channels need their own enable.

GPIO7 no longer wires straight to the BTS7960. It drives buffer channel 4. Several driver boards pull their logic inputs up to 5 V, which is above the ESP32-S3 GPIO absolute maximum. The buffer output, with a series resistor, is the pin that may meet that 5 V node. Channel 4 is therefore the motor enable rather than a spare. See ADR-002.

## LEDC

ESP32-S3 LEDC has low-speed mode only, eight channels and four timers. Both servos share timer 0 because both are 50 Hz. The feeder uses timer 1 at 1 kHz. `LEDC_AUTO_CLK` is selected for both timers so they resolve to one clock source, which is a requirement on this chip.

A 1,500 µs pulse is 1,228 counts at 14-bit resolution over a 20 ms frame (`1500 * 16384 / 20000`). Phase 1 never loads that duty. It loads zero and leaves the buffer disabled. Zero duty must not be connected to a powered servo: it is not a mid-frame pulse, it is a constant low.

## Boot order

External resistors define the pin level from reset until `app_main` runs. The first call is `boot_safe_outputs()`, which drives GPIO7 low and GPIO8/GPIO9 high before LEDC or UART setup. Servo and feeder initialization then program duty 0 and drive those same levels again.
