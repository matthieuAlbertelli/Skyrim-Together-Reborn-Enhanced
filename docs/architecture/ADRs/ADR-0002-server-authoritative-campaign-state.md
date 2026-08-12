# ADR-0002 — Server-Authoritative Campaign State

- **Status:** Accepted
- **Date:** 2026-07-19

## Context

Phases, classes, ready states, bindings, and narrative secrets can diverge if
clients decide independently.

## Decision

The server stores canonical campaign state. The Session Manager has
administrative permissions but is not the final technical source of truth.

## Consequences

- consistent reconnection and arbitration;
- cooperative transitions depend on the server;
- persistence and snapshots are required;
- CK scripts apply consequences, not canonical truth.
