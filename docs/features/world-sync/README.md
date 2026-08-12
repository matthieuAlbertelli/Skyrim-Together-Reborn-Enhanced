# World Sync

> **Status:** implemented and validated in game for the documented scope.
> **Primary validation:** August 10, 2026.

World Sync provides network identity and lifecycle to physical objects that must
exist consistently for several players while allowing Skyrim to simulate physics
locally.

## Current user behavior

### Object dropped by a player

```text
local drop
→ create WorldEntity
→ remote materialization
→ local Havok on every client
→ settlement by the authority
→ point-in-time correction only for significant divergence
```

### Object already present in the world

```text
first relevant network interaction
→ lazy adoption through PlacedReferenceId
→ unique server WorldEntityId
→ bind to the existing local TESObjectREFR
```

No global cell scan is required.

### Grab with Better Grabbing

```text
local grab
→ server authority
→ object hidden for observers
→ local movement only
→ release
→ remote reappearance/repositioning
→ local Havok
→ final settlement
```

STRE does not redistribute Better Grabbing. The SKSE plugin is an external
multiplayer dependency controlled through the generic `NativePlugins` policy.

### Ownership and theft

Ownership is carried as provenance (`ExtraOwnerId`) through supported paths.
Grabbing an owned reference without authorization triggers Skyrim's theft
behavior.

If dialogue opens during a grab, STRE cleanly ends the grab so controls and
dialogue remain usable.

## Sources of truth

- [Technical design](TECHNICAL_DESIGN.md)
- [Protocol reference](PROTOCOL_REFERENCE.md)
- [Better Grabbing integration](BETTER_GRABBING_INTEGRATION.md)
- [Test plan](TEST_PLAN.md)
- [ADR-0017](../../architecture/ADRs/ADR-0017-world-entity-authority-local-havok.md)

## Principles

- `WorldEntityId` is the network identity of a world instance.
- A temporary local FormID is never the network identity.
- A placed reference retains its local Skyrim reference and is not duplicated.
- The server arbitrates lifecycle and authority.
- Havok remains local.
- The network enforces final convergence, not frame-by-frame physics simulation.
- Network-triggered Skyrim mutations run in a safe engine context.
- An operation that cannot preserve required metadata fails explicitly.

## Known limitations

- custom names (`ExtraTextDisplayData`) are not synchronized;
- durable persistence after restart or save branch is not implemented;
- in-game coverage still needs expansion for quest objects and heavily scripted
  references;
- the model is not yet generalized to every world-entity type.
