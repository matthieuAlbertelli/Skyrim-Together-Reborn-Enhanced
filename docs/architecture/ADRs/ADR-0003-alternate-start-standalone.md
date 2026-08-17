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

The local Character Build path uses the same catalog without a server. Fresh
New Game interception and the MQ101/post-Helgen world-state projection are
implemented through local CK/Papyrus/native boundaries that do not require
server authority, and the current pre-Departure flow is runtime-validated.
Valen, Departure, the neutral MQ102/MQ103 continuation, and an explicit
end-to-end standalone regression still require validation before the entire
Alternate Start experience is complete.
