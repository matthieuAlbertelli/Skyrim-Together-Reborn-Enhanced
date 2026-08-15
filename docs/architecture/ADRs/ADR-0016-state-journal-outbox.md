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

### Implementation clarification — accepted semantic no-ops (2026-08-15)

Acceptance at the canonical command boundary durably reserves the command's
`MutationId`, even when its requested value already matches canonical state.
Such a semantic no-op appends its command identity to the same validated-mutation
journal with equal expected and resulting revisions. It does not advance
`StateVersion`, rewrite current state, or enqueue a redundant replication
intent. An exact replay remains idempotent; reuse of the identifier for different
command content fails closed.

SQLite schema v2 permits multiple ordered journal records to name the same
resulting state revision while retaining uniqueness of `(CampaignId,
MutationId)`. This is a refinement of the existing journal/outbox transaction,
not a second receipt store.

## Consequences

- direct current-state reads;
- crash recovery without rebuilding the entire world;
- safe network retransmission;
- schema migrations and fault-injection tests are required.
