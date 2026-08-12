# ADR-0003 — Alternate Start Remains Playable Without STRE

- **Status:** Accepted
- **Date:** 2026-07-19

## Context

The plugin bypasses Helgen and must also serve as a reference integration.

## Decision

CK content retains a complete single-player path. The STRE bridge is optional
and detected at runtime. No essential stage depends on a network response when
STRE is absent.

## Consequences

- wider adoption and better decoupling;
- two modes must be tested;
- some purely cooperative mechanics have a variant or remain inactive in
  single-player.

## Implementation state

The local Character Build path uses the same catalog without a server. The
complete new-game and Helgen-bypass flow remains unimplemented; the ADR is
therefore validated for the build, but not for the entire Alternate Start
experience.
