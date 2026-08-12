# ADR-0015 — Host Skyrim Save as Canonical Checkpoint

- **Status:** Accepted
- **Date:** 2026-07-30

## Context

The world must follow single-player persistence, reset, and deletion rules while
retaining validated multiplayer mutations.

## Decision

The STRE server is authoritative during the session. The host's `.ess` save is
the canonical external checkpoint. An STRE journal retains mutations that are
not yet integrated or concern cells not materialized by the host.

## Consequences

- save/load must coordinate with STRE checkpoints;
- the host's vanilla rules drive deletions;
- a small STRE store remains necessary for crash recovery;
- guest saves have no authority over shared state.
