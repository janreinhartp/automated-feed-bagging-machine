# Machine sequence

The sequence below is the specification for a later phase. It is not implemented. `CycleStartRequested` has no legal transition, and the control task never emits `ChecksPassed`, so `ready` and every filling state are unreachable in this firmware.

The hopper sight glass is an operator inspection, not a step the controller can sense. There is no low-level probe.

## Intended automatic cycle

1. Readiness: settings valid, drivers up, motion not inhibited by an unverified interlock.
2. Safety circuit: auxiliary contact healthy, snapshot fresh.
3. Weighing communication healthy, using the verified map.
4. Tare the weighing hopper.
5. Command servo 1 open. Servo 2 stays at its closed command.
6. Start the vibrating feeder.
7. Coarse fill at the coarse feeder and gate settings.
8. Medium fill after the coarse-to-medium threshold.
9. Fine fill after the medium-to-fine threshold.
10. Stop the feeder at the fine cutoff, including overrun compensation.
11. Command servo 1 closed.
12. Wait for the stable-weight interval. Stability comes from the verified transmitter flag or from a measured dwell, not from an invented bit.
13. Accept the weight only inside the lower and upper limits.
14. Require the bag sensor, debounced.
15. Command servo 2 open. Servo 1 stays closed. The two gates are not commanded open together.
16. Hold the discharge interval.
17. Command servo 2 closed.
18. Confirm the hopper is under the empty threshold.
19. Increment the bag counter.
20. Wait until the bag sensor clears, or raise a fault at the removal timeout.
21. Return to `ready`. Do not start another bag without a new start request.

A process stop from any filling state goes to `stopping`: feeder command zero, servo 1 commanded closed, servo 2 left closed, then `idle` or `ready` after the motion timers. It does not discharge a partial batch. Emergency stop is a different event: the safety relay has already killed the feeder, and the state latches at `emergency_stop` until the contact is healthy and an operator resets. Reset goes to `idle`, never back into the fill step that was active.

Out-of-tolerance weight, lost weighing communication, lost I/O communication, and a stale snapshot are faults. They do not retry the cycle by themselves.

## Parameters

Defaults that are requirements from the brief:

| Parameter | Default |
|---|---:|
| Target | 2,000 g |
| Lower limit | 1,990 g |
| Upper limit | 2,010 g |
| Coarse / medium / fine feeder | 100% / 60% / 30% |
| Coarse / medium / fine gate | 100% / 60% / 20% |

Defaults that are placeholders, flagged in the settings page, and not measured with feed:

| Parameter | Placeholder |
|---|---:|
| Coarse to medium | 1,800 g |
| Medium to fine | 1,960 g |
| Fine cutoff | 1,995 g |
| Overrun compensation | 0 g |
| Stable-weight interval | 500 ms |
| Discharge duration | 2,000 ms |
| Empty hopper | 20 g |
| Servo move time | 500 ms |
| Bag presence debounce | 50 ms |
| Bag removal timeout | 30,000 ms |

The validator requires a monotonic feeder and gate profile, a target inside the acceptance band, and rising thresholds that end at or below the target. That keeps a stored profile internally consistent. It does not make the profile correct for a particular feed.

## States

| State | In this firmware |
|---|---|
| `booting`, `initializing` | Passed through at start |
| `idle` | Normal resting state |
| `ready` | Defined. Not entered |
| `tare` through `waiting_for_bag_removal` | Defined. Not entered |
| `stopping` | Defined for a later controlled stop. Not entered |
| `manual` | Defined. Not entered, because the selector input is not trusted |
| `fault` | Entered only if a driver fails to initialize |
| `emergency_stop` | Defined. Not entered, because DI6 polarity is not trusted |

`warning` is not a state. See `SAFETY_DESIGN.md`.

Manual mode, when it exists, still has to pass the safety manager. A selector switch will not jog a motor by itself, and the web page will not grow a direct PWM control.
