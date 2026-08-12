# ADR-0016 — Current State, Journal, and Transactional Outbox

- **Status:** Accepted
- **Date:** 2026-07-30

## Context

Full Event Sourcing would greatly increase complexity. However, a state write
followed by a non-atomic network send would create loss or duplication windows.

## Decision

Future storage combines:

- normalized current state;
- an append-only journal of validated mutations;
- a replication outbox written in the same transaction.

Commands are idempotent and guarded by optimistic revision checks.

## Consequences

- direct current-state reads;
- crash recovery without rebuilding the entire world;
- safe network retransmission;
- schema migrations and fault-injection tests are required.
