# ADR-0017 — WorldEntity authority with local Havok

> **Status:** Accepted
> **Date:** 2026-08-10

## Context

Skyrim's Havok simulation is local and non-deterministic enough that continuously streaming rigid-body poses between clients creates jitter, collisions that fight each other and fragile engine interactions.

Dropped objects and movable placed references still need a stable shared identity, arbitration and eventual convergence.

## Decision

STRE separates **logical authority** from **local physics simulation**:

- the server owns `WorldEntityId`, lifecycle and manipulation authority;
- each client runs local Havok for its materialized reference;
- the current authority sends a final settled transform;
- observers reconcile only when divergence exceeds the defined tolerance;
- grabbed objects are not continuously simulated/streamed on observers;
- pre-placed references are lazily adopted and bound to the existing local reference;
- network-triggered Skyrim mutations run on the game update path;
- STR's validated engine primitives are preferred over speculative custom ABI wrappers.

## Consequences

### Positive

- no continuous fight between remote transforms and local Havok;
- simpler bandwidth/lifecycle model;
- local Skyrim behavior remains natural;
- dynamic FormIDs remain client-local implementation details;
- placed references can be reused without duplicate spawning.

### Trade-offs

- observers do not see continuous remote held-object motion;
- final convergence is eventual rather than frame-identical;
- durable persistence requires a later storage/checkpoint layer;
- object classes with special scripts/quest semantics require dedicated policies.

## Rejected alternatives

- continuous authoritative transform streaming for every held/dropped rigid body;
- duplicating pre-placed references on each adoption;
- treating local runtime FormIDs as stable network identities;
- direct network-thread engine mutation;
- custom `SetPosition`/`SetAngle` wrappers without verified ABI.
