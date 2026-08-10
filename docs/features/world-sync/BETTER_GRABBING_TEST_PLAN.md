# Better Grabbing bridge test plan

## Prerequisites

- Install Better Grabbing independently on both clients.
- Confirm `BetterGrabbing.dll` is loaded by SKSE.
- Keep Better Grabbing's `DisableCollisionWithItemsWhileGrabbing = true` setting enabled.
- Server config contains:

```ini
[Gameplay]
bEnableItemDrops = true

[ModPolicy]
sRequiredNativePlugins = BetterGrabbing.dll
```

## Gate tests

1. Client without Better Grabbing: connection must be rejected with a missing native plugin error.
2. Both clients with Better Grabbing: connection must succeed.
3. Confirm logs contain `[STRE][NativePluginPolicy]` and BetterGrabbing.dll.

## Manipulation tests

For each direction J1 -> J2 and J2 -> J1:
1. Drop a simple clutter item and wait for settlement.
2. Grab it and move it slowly for 3 seconds.
3. Rotate/translate it using Better Grabbing controls.
4. Observer must see continuous movement.
5. While held, pass it through/near other loose objects: those objects must not be knocked by the streamed representation.
6. Release it above the floor.
7. Authority's local Havok must resume.
8. Observer must converge to the final settled location and regain normal collision.
9. Pick the item up; it must disappear for both players.

## Recovery tests

- Two players attempt to grab the same WorldEntity simultaneously: one authority wins; no permanent divergence after both release.
- Authority disconnects while holding: observer must not retain a permanently non-collidable object.
- Observer disconnects/reconnects during manipulation: snapshot/state recovery must not duplicate the item.
- Late join after settlement: one copy only, at authoritative final transform.

## Log markers

Expected:
- `manipulation_start_requested`
- `manipulation_granted`
- `manipulation_released`
- `settled_transform_send`
- `manipulation_settlement_received`
- `manipulation_collision_restored`

Failure/recovery markers:
- `manipulation_collision_restore_retry`
- `manipulation_collision_restore_fallback`
- `manipulation_collision_fallback`


## Held-object visibility regression

1. J1 drops a WorldEntity and waits for settlement.
2. J1 grabs it with Better Grabbing.
3. J2 must see the object disappear as soon as the grab is granted.
4. J1 may move/rotate/hold it for at least 5 seconds; J2 must never see intermediate poses.
5. J1 releases it.
6. J2 must see the object reappear at the release position, with normal Havok.
7. After authoritative settlement, both clients must converge as with a normal drop.
8. Disconnect the authority while holding: observers must recover a visible object and no client may keep a permanently hidden reference.

## Placed-reference lazy adoption

### P1 - First grab of a vanilla placed object
1. Pick a bottle/plate/book-like movable reference already present in the cell (not previously dropped by a player).
2. J1 grabs it with Better Grabbing.
3. Verify it disappears for J2.
4. J1 moves it and releases it.
5. Verify it reappears for J2 at the release position.
6. Wait for settlement and verify both clients converge.

Expected log markers:
- client authority: `manipulation_adoption_requested`
- server: `placed_adopted`
- both clients: `placed_bound`
- observer: `manipulation_hidden ... placed=true`
- observer: `manipulation_released ... mode=placed-reference`

### P2 - Re-grab the same placed object
1. After P1, J2 grabs the same object.
2. Verify no second `placed_adopted` entry is created for the same reference.
3. Verify the same WorldEntityId is reused.

### P3 - Direct pickup before any grab
1. Choose a different vanilla placed item.
2. Pick it directly into J1 inventory without grabbing it first.
3. Verify the server logs `placed_adopted` immediately followed by `pickup_committed` for the same WorldEntityId.
4. Verify the physical reference disappears for J2 and no duplicate inventory delta is applied.

### P4 - Pickup after prior grab
1. Grab/release a placed item once so it is adopted.
2. Pick it up later.
3. Verify the existing WorldEntityId is consumed and removed on all clients.

### P5 - Late join
1. J1 moves a placed object and lets it settle.
2. J2 joins afterwards.
3. Verify J2 binds its existing placed reference (no duplicate spawn) and sees it at the authoritative moved transform.
