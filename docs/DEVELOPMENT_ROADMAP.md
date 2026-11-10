# Development roadmap

Phase 1 is this repository: the architecture, the safe skeleton, and a firmware image that boots inhibited. Do not skip to a live fill.

## Phase 2 — Bench, still unpowered at the actuators

- Read the WROOM flash and PSRAM marking. Adjust `sdkconfig.defaults` if it is not a 4 MB part.
- Confirm DevKitC-1 v1.0 versus v1.1. Neither RGB-LED pin is used; this is identification only.
- Build the 5 V and buffer circuit. Measure GPIO7 low, GPIO8 high, GPIO9 high, and high-impedance buffer outputs before any servo or driver logic supply is connected.
- Prove the two RS485 converters are isolated: logic ground and bus ground must not be continuous.
- Obtain the QL-MT01 manual and the exact N4D8B08 manual. Transcribe registers into the device files. Do not copy a map from a different module that shares the same marketing name.
- Identify the bag sensor as NPN or PNP and the module input rating. Add an interposing relay if they disagree.
- Inspect the BTS7960 PCB for a real fault output. Leave DI7 spare if there is none.
- Decide the unpowered gate position with the mechanical design, and record it against `POWER_DISTRIBUTION.md`.

## Phase 3 — One bus at a time

Enable transmission only after the map is in source and reviewed. Talk to the weighing transmitter with the load cell connected and no feeder power. Then talk to the I/O module with lamps only. Confirm automatic direction with a scope. Add a DE GPIO only if that test fails.

## Phase 4 — Actuators, unloaded

- One servo, mechanically disconnected from the gate, on the 7.4 V rail. Find the pulse that is closed and the pulse that is open by eye. Store those as a draft. Commit `calibrated` only after the horn travel is inside the gate's hard stops with margin.
- Feeder driver with the motor unplugged: enable and PWM at a low duty, current near zero.
- Motor connected, no feed, fused, with a current measurement and a thermal check. Record stall or start current and resize the fuse and the supply if the provisional 15 A branch is wrong.

## Phase 5 — Process

Commission DI6 polarity and only then allow the safety manager to latch `emergency_stop`. Add manual jog through the controller, still under the safety manager. Implement the sequence in `MACHINE_SEQUENCE.md` with the feeder fuse pulled for a dry run of the state trace, then with feed. Replace every placeholder threshold. The acceptance band stays 1,990–2,010 g unless the product requirement changes.

## Phase 6 — Panel hardening

Change factory passwords. Decide whether status should also require authentication. Persist a short fault history with an explicit wear limit. Still no cloud and no external CDN.

## Exit criteria before the first bag

- Safety relay kills the motor with the ESP32 held in reset.
- Both gates have measured closed pulses and a documented unpowered position.
- Weight on the panel matches the QLMT display across the acceptance band.
- A lost weighing bus or a lost I/O bus inhibits motion.
- Power cycle ends in `idle`, not in a fill state.
