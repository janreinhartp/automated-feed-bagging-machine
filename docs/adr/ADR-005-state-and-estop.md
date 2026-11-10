# ADR-005: State model, hardwired E-stop, no automatic resume

Date: 2026-11-11
Status: Accepted

## Context

The first state list mixed a process step (`coarse_fill`) with conditions (`warning`, `emergency_stop`). A warning that replaces the process state throws away the step the machine was in. The emergency stop was correctly described as hardwired, but DI6 was also listed as if software participation were required for the stop.

Filling states also need a controlled stop that is not a fault and not an emergency stop: feeder off, flow gate closed, hopper not discharged.

## Decision

`warning` is a status flag and a diagnostic event, not a state. `stopping` is a state between a filling step and `idle`. `emergency_stop` can be entered from any other state and can leave only by reset, and only to `idle`. The transition table has no edge from emergency stop back into the interrupted fill. Boot always ends in `idle` or `fault`. Nothing starts a cycle because power returned.

The safety relay, not DI6 and not a relay on the N4D8B08, drops the feeder. DI6 is a monitor whose polarity is unverified, so phase 1 does not enter `emergency_stop` from it. The hardware stop still happens. The panel is required to say that the feedback is not commissioned, so `idle` is not read as a passed safety test.

## Consequences

The sequence document and the enum stay aligned for the later implementation, while this firmware cannot occupy a filling state. Operators need a lamp or a panel banner for emergency stop until DI6 is commissioned; darkness of the motor supply is not a sufficient indication. The unpowered position of the servo gates remains a mechanical decision and is not papered over by a software close command.
