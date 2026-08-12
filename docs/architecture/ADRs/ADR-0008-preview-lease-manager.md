# ADR-0008 — Native Preview Becomes a Leased Resource

- **Status:** Proposed
- **Date:** 2026-07-19

## Context

The current bridge accepts only one client and remains nominally tied to Trading.

## Decision

Introduce an `ItemPreviewRuntime` that grants a lease to an owner, arbitrates
priority, and fully encapsulates the host menu and native session.

## Consequences

- genuine reuse by several features;
- testable preemption and ownership;
- migration of `TradeItemPreviewService`;
- a possible external API without exposing native pointers.
