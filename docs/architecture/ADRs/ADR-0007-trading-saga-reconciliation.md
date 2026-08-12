# ADR-0007 — Trading Is a Compensating Saga

- **Status:** Implemented
- **Date:** 2026-07-19

## Context

Two local Skyrim inventories and server state cannot be modified by one ACID
transaction.

## Decision

The server validates a plan, clients apply their portion and record the result,
then the server commits. Under uncertainty, reconciliation using absolute
quantities restores convergence.

## Consequences

- resilient retransmission;
- `ApplyId/ReconcileId` protocol complexity;
- atomicity must be described as logical, not ACID;
- failure tests are essential.
