# ADR-006: Motion inhibit and the web command path

Date: 2026-11-11
Status: Accepted

## Context

A local panel is part of the machine, and it is also a way to bypass a physical interlock if a route is allowed to write GPIO. Factory passwords and an open access point make that worse. NVS is plaintext. There is no TLS on the access point.

The filling sequence is specified and should not be half-implemented behind a hidden start button.

## Decision

Motion permission is a compile-time constant defaulting to false, checked by the safety manager, and the control task calls `force_safe()` every cycle regardless. Servo and feeder command methods return `Denied`. The web server's only actuator API is `submit_command`. `start_cycle` is rejected with HTTP 409 before it can change state. Settings and calibration writes store drafts; they do not arm PWM.

Status and diagnostics are readable by anyone on the access point. Changes and commands require HTTP basic authentication as `operator`. Factory passwords are printed in the README and flagged on the panel until the web password changes. Wi-Fi changes apply on the next boot rather than dropping the session mid-request.

Diagnostic history stays in RAM. Flash wear is not spent on a log that does not yet have a retention policy. Timestamps are milliseconds since boot because no RTC was selected.

## Consequences

The panel is useful for settings and for proving that start is rejected. It cannot jog the machine. Anyone who joins the access point can read status until a later phase puts authentication on those GET routes as well. Password changes and a locked enclosure are operational requirements, not optional polish.
