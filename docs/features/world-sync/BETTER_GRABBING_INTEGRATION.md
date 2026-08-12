# Better Grabbing multiplayer integration

> **Status: Implemented and validated for the current World Sync scope**

## Scope

STRE **does not distribute** or **link against** Better Grabbing code.

The user installs the SKSE plugin separately. In multiplayer, `BetterGrabbing.dll` is required by default through the generic native-plugin policy.

## Responsibilities

### Better Grabbing

- owns local grab input;
- computes local translation and rotation;
- applies its local Skyrim behavior;
- continues to work independently in single-player.

### STRE

- detects loaded native SKSE plugins during the handshake;
- applies `ModPolicy:sRequiredNativePlugins`;
- observes the lifecycle through available Skyrim events;
- assigns and resolves the `WorldEntityId`;
- arbitrates authority;
- hides the object for observers during the grab;
- manages release, settlement, timeout, disconnect, and snapshots;
- supports lazy adoption of placed references;
- preserves ownership through supported paths;
- forces a grab to end if a Skyrim dialogue opens during manipulation.

STRE does not depend on Better Grabbing's internal `Manager`.

## Native-plugin policy

Default configuration:

```ini
[Gameplay]
bEnableItemDrops = true

[ModPolicy]
sRequiredNativePlugins = BetterGrabbing.dll
```

The value is a list of loaded SKSE DLL names. The mechanism is generic and is not a dependency manager specific to Better Grabbing.

## Remote representation

During an accepted grab:

```text
authority
  Better Grabbing local motion

observer
  local WorldEntity representation hidden
```

Intermediate transforms are not broadcast to simulate remote movement.

On release:

- the observer restores and repositions the representation;
- local Havok resumes;
- final authoritative settlement corrects the result only when necessary.

## Placed references

The first interaction can send:

```text
WorldEntityId = 0
PlacedReferenceId = stable GameId of the TESObjectREFR
```

The server atomically resolves or creates a `WorldEntityId`.

Each client then binds that ID to its existing local reference. **No duplicate spawn.**

On remote release of a placed reference, STRE uses STR's existing `MoveTo` path on the game thread (`RunnerService`) rather than speculative `SetPosition` or `SetAngle` wrappers.

## Ownership and theft

If the placed reference has an owner and Skyrim does not consider the player an authorized owner, grabbing triggers Skyrim's theft primitive.

Penalties, witnesses, and guards remain managed by vanilla systems.

## Dialogue safety

The `Dialogue Menu` can open while Better Grabbing still holds an object.

STRE then forces the grab to end through the native player primitive. The normal `TESGrabReleaseEvent` continues the WorldEntity lifecycle, preventing:

- blocked dialogue controls;
- an object remaining grabbed after an arrest;
- network state diverging from local state.

## Failure recovery

- heartbeat timeout: release authority;
- authority disconnect: release and recovery;
- observer disconnect: snapshot and rebinding on reconnect;
- pending adoption plus release: defer release until resolution;
- dialogue during grab: forced local release followed by the normal lifecycle.

## Non-goals

- reimplement Better Grabbing controls;
- stream held-object physics frame by frame;
- redistribute Better Grabbing;
- depend on its internal classes;
- guarantee every Better Grabbing configuration that substantially changes collision or physics without testing.
