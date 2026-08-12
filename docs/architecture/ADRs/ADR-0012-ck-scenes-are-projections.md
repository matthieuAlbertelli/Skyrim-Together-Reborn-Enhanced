# ADR-0012 — CK Scenes Are Projections of Canonical State

- **Status:** Accepted
- **Date:** 2026-07-19

## Context

A scene can be interrupted, invisible, or missed by a client.

## Decision

Campaign phase remains in STRE. A CK scene renders that phase locally and
reports its completion; it is never the sole source of truth.

## Consequences

- recovery after reconnection;
- scenes must be idempotent and support skip/resume;
- phase and local stage require explicit coordination.

## Implementation state

The multiplayer build is canonical on the server, and the CK stage only triggers
the UI and local application. Future Valen scenes and campaign phases are not yet
implemented.
