# Better Grabbing multiplayer integration

## Scope

STRE does not ship or redistribute Better Grabbing. The user installs the SKSE plugin separately.
Multiplayer requires the loaded module `BetterGrabbing.dll` by default.

## Responsibilities

### Better Grabbing
- Owns local grab input and object placement.
- Updates the grabbed reference through Skyrim's SetAngle/SetPosition/Update3DPosition path.
- Keeps grabbed items non-collidable when its default `DisableCollisionWithItemsWhileGrabbing = true` option is enabled.
- Continues to work independently in solo.

### STRE
- Detects loaded SKSE plugin DLLs during authentication.
- Enforces required native plugins through ModPolicy.
- Maps local dynamic FormIDs to stable WorldEntityIds.
- Arbitrates one manipulation authority per WorldEntity.
- Broadcasts Start and Release lifecycle events while connected.
- Sends private authority heartbeats to the server while held; observers never receive held-object transforms.
- Hides the remote WorldEntity representation for the duration of the grab.
- Reuses the existing authoritative settlement/recreate path after release.

## Native plugin policy

The generic server setting is:

```ini
[ModPolicy]
sRequiredNativePlugins = BetterGrabbing.dll
```

The value is a comma-separated list of loaded SKSE plugin DLL filenames. Presence means loaded in the Skyrim process, not merely present on disk.

The handshake carries each loaded native plugin filename and its Windows file version when available. Version constraints are deliberately not enforced in this first milestone.

## WorldEntity manipulation state

```text
FREE
  -> Start accepted
MANIPULATED(authorityPlayerId)
  -> Release
SETTLING(authorityPlayerId)
  -> authoritative settled transform
FREE
```

A second Start while MANIPULATED is rejected. Private heartbeat Updates and Release are accepted only from the current authority. Heartbeats refresh authority state but are not broadcast.

## Remote representation

While MANIPULATED, the observing client disables its local WorldEntity reference.
There is no remote interpolation and no remote collision participation while held.

On Release, the observer recreates the WorldEntity immediately at the release transform.
Normal local Havok resumes from that point. The later authoritative settlement transform
uses the existing one-shot recreate/reconcile path if local simulation diverged.

## Failure recovery

- Authority heartbeat timeout: server releases authority.
- Authority disconnect: server broadcasts a release/final known transform.
- Recreate failure: client retries for a bounded window.
- Network disconnect while a remote item is hidden: client re-enables the original local reference so it cannot remain permanently invisible.

## Non-goals

- STRE does not implement Better Grabbing input, raycasts, placement, rotation controls or collision avoidance.
- STRE does not distribute Better Grabbing.
- STRE does not synchronize collision cascades caused by a configuration that re-enables collisions while grabbing.
- Native plugin version constraints are future work.

## Lazy adoption of placed references

Placed Skyrim/plugin references are not pre-registered as WorldEntities. STRE adopts them only when a multiplayer interaction needs a stable network identity.

### Grab

1. Better Grabbing starts holding a non-temporary `TESObjectREFR`.
2. If the local FormID has no WorldEntity binding, the client resolves the reference FormID through `ModSystem` to a server-space `GameId`.
3. `RequestWorldEntityManipulation(Start)` carries `WorldEntityId = 0` plus that `PlacedReferenceId`.
4. The server atomically resolves or creates one WorldEntity for that `GameId`.
5. Start notification returns the assigned WorldEntityId and the placed reference identity to every observer.
6. Each client binds the WorldEntityId to its own already-existing local `TESObjectREFR`; no duplicate object is spawned.
7. Observers hide that reference until release.
8. Release moves the same placed reference once to the release transform, re-enables it, and uses the normal settlement authority flow.

### Pickup from the world

If a non-temporary placed reference is picked up before any grab adoption, `RequestInventoryChanges` carries its `PlacedReferenceId`. The server performs `resolve/adopt -> consume` in the same packet handler, so two players cannot create two WorldEntity identities for the same placed reference.

For vanilla non-temporary pickups, activation sync already owns the inventory delta. STRE therefore emits a `LifecycleOnly` WorldEntity notification to retire the physical binding without applying the inventory delta twice.

### Identity invariant

For a placed reference, the authoritative uniqueness key is:

`PlacedReferenceId = GameId(ModId, BaseId of TESObjectREFR)`

The server keeps a reverse index `PlacedReferenceId -> WorldEntityId`. Once adopted, later grabs reuse the same WorldEntityId until the reference is consumed.

Dynamic dropped references continue to use their existing runtime WorldEntity creation/materialization path and are not affected by lazy adoption.
