# ADR-001: Repository root and component boundaries

Date: 2026-11-11
Status: Accepted

## Context

The brief suggested an `animal-feed-bagging/` directory inside the workspace. The workspace itself is the project. A nested ESP-IDF tree would leave the real firmware one folder down from the repository root and invite a second copy of the docs.

The suggested component list matches the hardware boundaries. A few of those components would be empty wrappers if they also owned storage or GPIO.

## Decision

The ESP-IDF project lives at the repository root. GPIO numbers exist only in `hardware_config`. NVS access exists only in `settings`. `calibration` is the API that may store servo and weighing drafts, and it forces `calibrated` to 0. Device protocol files do not include the state machine. The web server reaches actuators only by submitting a command to `machine_controller`.

`common` exists for the status enum and the one-slot mailbox. That is the only extra component. It avoids copying the status enum into every header.

## Consequences

Build commands run in the repository root. A new GPIO has to land in one header, which also `static_assert`s that the pin is legal on a DevKitC-1. Calibration cannot grow a second NVS layout.
