# ADR-0005 — Session Manager and Dragonborn Are Separate Roles

- **Status:** Accepted
- **Date:** 2026-07-19

## Context

The technical host must not automatically become the canonical hero.

## Decision

The Session Manager administers the campaign. Dragonborn is an assigned
narrative role revealed according to campaign rules.

## Consequences

- fairer group narrative;
- explicit authority policies;
- handling an absent Dragonborn remains to be defined;
- secrets are filtered by the server.

[ADR-0018](ADR-0018-fixed-roster-coordinated-checkpoint-recovery.md) later
resolves the v1 absence case through the same full-roster recovery lock and
collective checkpoint restore used for any required roster member.
