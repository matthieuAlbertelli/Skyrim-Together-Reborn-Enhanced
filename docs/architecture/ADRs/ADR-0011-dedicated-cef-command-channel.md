# ADR-0011 — Dedicated CEF Command Channel for STRE Features

- **Status:** Proposed
- **Date:** 2026-07-19

## Context

Trading currently multiplexes its actions through
`toggleDebugUI('__trade__', ...)`.

## Decision

Create a typed, versioned native channel, such as
`streCommand(namespace, action, payload)`, with corresponding events.

## Consequences

- readable, extensible contract;
- centralized validation;
- frontend migration;
- temporary compatibility must be planned.
