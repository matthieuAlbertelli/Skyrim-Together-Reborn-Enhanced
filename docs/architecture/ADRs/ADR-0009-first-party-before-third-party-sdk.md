# ADR-0009 — First-Party Adapters Before a Third-Party SDK

- **Status:** Accepted
- **Date:** 2026-07-19

## Context

Freezing a public ABI or protocol too early would be expensive to maintain.

## Decision

Implement Alternate Start as an adapter compiled into STRE. Stabilize the
concepts with at least two integrations before publishing an experimental
third-party SDK.

## Consequences

- rapid learning and freedom to refactor;
- delayed external access;
- internal interfaces must still be documented and tested.

## Implementation state

Character Build/Alternate Start is the first dedicated first-party integration.
Generic contracts and the third-party SDK intentionally remain unfrozen.
