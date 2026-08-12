# ADR-0004 — Full Snapshot Plus Incremental Events

- **Status:** Accepted
- **Date:** 2026-07-19

## Context

A reconnecting client cannot depend on events it missed.

## Decision

Every adapter exposes a versioned canonical snapshot. After applying the
snapshot, the client consumes only events strictly newer than that version.

## Consequences

- deterministic recovery;
- serialization and migration cost;
- idempotent event handlers;
- mandatory snapshot tests.
