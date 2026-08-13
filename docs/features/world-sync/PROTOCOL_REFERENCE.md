# World Sync — Protocol Reference

> **Status:** implemented for the current WorldEntity scope.

This document defines World Sync-specific contracts. General protocol rules are
in [`docs/architecture/NETWORK_PROTOCOL.md`](../../architecture/NETWORK_PROTOCOL.md).

## Shared types

### `WorldEntityId`

Server-stable identifier for a world instance during a session.

### `WorldEntityTransform`

Groups the position and rotation used by WorldEntity messages.

### `PlacedReferenceId`

Server-space `GameId` of the placed Skyrim **reference**, distinct from the
object's `BaseForm`.

It supports lazy adoption and server deduplication.

## Inventory/world lifecycle

Existing inventory messages carry required World Sync extensions.

### `RequestInventoryChanges`

Relevant World Sync fields:

- item data;
- drop/pickup indication according to the existing flow;
- creation or finalization transform where applicable;
- `PlacedReferenceId` when adopting a non-temporary reference.

### `NotifyInventoryChanges`

May carry:

- `WorldEntityId`;
- `PlacedReferenceId`;
- physical lifecycle;
- `LifecycleOnly`.

`LifecycleOnly` removes or updates the physical representation without applying
a second inventory mutation when the vanilla/activation flow already owns that
mutation.

## Manipulation

### `RequestWorldEntityManipulation`

Carries, among other fields:

- `WorldEntityId` (0 during first adoption of a placed reference);
- `PlacedReferenceId` where necessary;
- `Action`;
- `WorldEntityTransform`.

Current actions:

```text
Start
Update
Release
Rejected
```

`Update` is used as a private authority heartbeat; it does not imply visual pose
streaming to observers.

### `NotifyWorldEntityManipulation`

Broadcasts transitions required by observers and the authority.

For a placed reference, `PlacedReferenceId` lets the client bind the
`WorldEntityId` to its existing local reference.

## Server authority

```text
FREE
  -> Start accepted
MANIPULATED(authorityPlayerId)
  -> Release
SETTLING(authorityPlayerId)
  -> final TransformUpdate
FREE
```

Rules:

- reject a concurrent second `Start`;
- accept heartbeat and release only from the authority;
- timeout or disconnect releases authority;
- the final settlement transform becomes the convergence reference.

## Snapshot

A WorldEntity snapshot reconstructs:

- identity;
- required item and instance metadata;
- dynamic origin or placed reference;
- relevant canonical transform;
- current lifecycle.

A snapshot must not create a second local reference for an existing
`PlacedReferenceId`.

## Ownership metadata

The owner is serialized in `Inventory::Entry` through a stable `GameId` where
available.

Ownership is instance metadata that must be preserved; it must not be replaced
by a simple “stolen” boolean.

## Compatibility

Every change to these structures requires at least:

- encode/decode round trip;
- one authority-client and observer test;
- lazy-adoption test;
- snapshot/late-materialization test when serialized state changes;
- fail-closed behavior when an old path cannot retain new metadata.
