# World Sync — Test plan

> **Status: Primary two-player scenarios executed; hardening matrix remains ongoing**

## Prerequisites

On both clients:

- the same STRE build;
- Better Grabbing installed and loaded for manipulation tests;
- configuration compatible with the tested build.

Server:

```ini
[Gameplay]
bEnableItemDrops = true

[ModPolicy]
sRequiredNativePlugins = BetterGrabbing.dll
```

Record for every test campaign:

- Git SHA;
- server configuration;
- Skyrim runtime;
- Better Grabbing and Address Library versions;
- client P1, client P2, and server logs;
- test direction;
- observed result.

## A — Native-plugin policy

### A1 — Missing plugin

Client without Better Grabbing → connection rejected with an explicit native-plugin error.

### A2 — Plugin present

Two clients with Better Grabbing loaded → connection accepted.

## B — Dynamic dropped WorldEntities

Run P1→P2, then P2→P1.

### B1 — Simple drop

1. Drop a simple object.
2. The observer sees exactly one reference.
3. Havok runs locally.
4. After settlement, the clients converge.

### B2 — Drop above a surface, obstacle, or water

Verify that no continuous correction fights Havok and that the final transform converges.

### B3 — Pickup

After settlement, the other player picks up the object. It disappears for everyone and exactly one inventory mutation is applied.

## C — Better Grabbing on a dynamic WorldEntity

1. Grab a synchronized object.
2. The observer sees it disappear.
3. The player can move and rotate it locally for several seconds.
4. The observer must not see any intermediate pose.
5. Release it.
6. The object reappears and resumes its remote state.
7. Local Havok runs, followed by final settlement.
8. Pickup remains possible afterward.

## D — Lazy adoption of a placed reference

### D1 — First grab

1. Choose a movable bottle, book, or plate already present in the cell.
2. P1 grabs it.
3. P2 sees the existing local reference disappear.
4. P1 moves and releases it.
5. P2 sees **the same reference**, not a duplicate, at the release transform.
6. Final settlement completes.

### D2 — Opposite-player re-grab

P2 then grabs the same reference. The server must reuse the same `WorldEntityId`.

### D3 — Direct pickup without a prior grab

Pick up another placed reference. Adoption and consumption must be atomic from the server's perspective, with no duplicate inventory delta.

### D4 — Late join

Move a placed reference, let it settle, then connect the other client. It must bind its existing local reference without creating a duplicate.

## E — Ownership and theft

### E1 — Unowned object

Grab → no theft alarm.

### E2 — Owned object with a witness

Grab → immediate theft alarm; vanilla guard behavior is expected.

### E3 — Authorized object

If Skyrim considers the player an authorized owner, no false theft occurs.

### E4 — Provenance after inventory and drop

Steal → inventory → drop → remote pickup. Ownership must remain associated with the supported instance.

### E5 — Owned container

Take an object from an owned container, then drop it. Verify provenance.

## F — Dialogue and arrest

1. Grab an owned object in front of a witness.
2. Let guards initiate dialogue.
3. When the Dialogue Menu opens, the object must be released automatically.
4. Dialogue choices must be usable immediately.
5. After the dialogue, Better Grabbing must work normally on another object.
6. Verify that no network release remains blocked.

## G — Instance metadata

### G1 — Vanilla enchantment

Drop and pick up an enchanted weapon with a partially depleted charge.

### G2 — Player enchantment

Test a player-created enchantment, including effects and charge.

### G3 — Custom name

**Known limitation:** do not consider this scenario validated until `ExtraTextDisplayData` is supported.

## H — Concurrency and recovery

- two players attempt to grab the same WorldEntity;
- the authority disconnects during the grab;
- the observer disconnects and reconnects during the grab;
- release occurs very quickly before lazy adoption finishes;
- cell change after settlement;
- late join after several drops;
- alternating P1/P2 repetitions on the same reference.

## I — High-risk references

Validate before extending support:

- scripted references;
- enable-parent references;
- quest references;
- complex faction ownership;
- activatable objects with side effects;
- cells that reset.

These cases must not be advertised as guaranteed before specific validation.
