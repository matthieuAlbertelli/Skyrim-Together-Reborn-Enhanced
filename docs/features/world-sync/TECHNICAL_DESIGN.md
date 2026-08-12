# World Sync — Technical Design

> **Status:** implemented for dropped items and lazy placed-reference adoption.

## Responsibilities

### Server

The server owns:

- `WorldEntityId` allocation and resolution;
- the `PlacedReferenceId -> WorldEntityId` index;
- lifecycle;
- manipulation authority;
- settlement state;
- session snapshot.

### Client

The client owns:

- `WorldEntityId <-> local Skyrim reference` binding;
- dropped-item materialization;
- resolution of an existing placed reference;
- observation of grab and release events;
- local Havok;
- stability sampling;
- application of point-in-time corrections.

## Identity

### Dynamic drop

A temporary reference created by Skyrim does not have the same FormID on every
client.

STRE therefore assigns it a session-stable `WorldEntityId`.

### Placed reference

The server deduplication key is:

```text
PlacedReferenceId = GameId of the TESObjectREFR
```

The server adopts the reference at the first relevant network interaction. Each
client then resolves its corresponding local reference.

## Drop and settlement

The authority client lets Havok evolve locally, then observes stability.

Settlement is bounded:

- periodic sampling;
- minimum duration before stability;
- maximum duration with fallback to the last known transform;
- one final transform;
- remote correction only when divergence exceeds the defined tolerance.

The goal is convergence without continuously calling `MoveTo` or an equivalent
during simulation.

## Manipulation

Logical state:

```text
FREE
  -> Start
MANIPULATED(authorityPlayerId)
  -> Release
SETTLING(authorityPlayerId)
  -> final transform
FREE
```

During `MANIPULATED`:

- Better Grabbing handles local movement;
- the server receives only lifecycle and heartbeat information;
- observers hide their representation;
- no intermediate transform is streamed to observers.

On release:

- a dynamic drop resumes its normal materialization and Havok path;
- a placed reference is repositioned through STR's existing
  `TESObjectREFR::MoveTo` path;
- the engine call runs through `RunnerService`/`OnUpdate`;
- final settlement then resumes.

## Engine safety

Do not reintroduce `SetPosition`/`SetAngle` wrappers based on an assumed address
signature.

The placed-reference observer crash demonstrated that this ABI was incorrect.
The validated path is STR's existing `MoveTo` primitive called in the appropriate
game context.

## Ownership

`Inventory::Entry` carries a server-space owner when it can be resolved.

Consequences:

- identical instances with different owners do not merge;
- the owner can survive supported inventory/world paths;
- `ExtraOwnership` is restored during appropriate reconstruction;
- grabbing an unauthorized placed reference triggers `StealAlarm`.

A simple `IsStolen` boolean is not used as the source of truth.

## Dialogue and forced release

Skyrim's `Dialogue Menu` alone does not reliably stop Better Grabbing.

When that menu opens during local manipulation, STRE requests a native grab end.
The existing `TESGrabReleaseEvent` then continues the normal network lifecycle,
including when adoption was still pending.

## Snapshot and late join

A client joining after creation or adoption must:

- receive current WorldEntity state;
- materialize required drops;
- bind existing placed references without duplication;
- apply the relevant canonical transform;
- never recreate an already consumed entity.

## Current non-goals

- server physics simulation;
- frame-by-frame rigid-body streaming;
- duplication of placed vanilla references;
- advance scanning of every movable reference;
- complete disk persistence;
- synchronization of every Skyrim ExtraData type without an explicit policy.
