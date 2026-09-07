# Headquarters Inn

> **Status:** Accepted v1 design contract.

## Purpose

The headquarters inn is the Alternate Start gathering point, the cooperative
campaign headquarters, Messire Valen's introduction and coordination location,
and the physical foundation for v1 player housing.

## v1 implementation boundary

For v1.0.0:

- `STRE_CELL_AlternateStart` is the physical headquarters cell;
- the headquarters remains an instanced Skyrim interior;
- exterior access uses normal Skyrim cell-transition and load-door mechanics
  where applicable;
- architecture and layout are created in the Creation Kit with Skyrim-compatible
  and vanilla-compatible content;
- bespoke Blender- or AI-generated architectural meshes are not required for v1
  acceptance;
- navmesh, NPC pathing, collision, lighting, visual readability, and acceptable
  runtime performance are required for v1 acceptance;
- room bounds, portals, and explicit visibility partitioning are conditional
  optimizations, added only when profiling or runtime validation demonstrates a
  concrete visibility or performance need.

This is a deliberate product-scope decision, not a statement of engine
limitation.

These are target acceptance requirements, not claims about the current
checkpoint. [`STATUS.md`](../../project/STATUS.md) owns implementation truth:
the current architecture remains provisional, decoration is only a minimal
first pass, and the existing empty, doorless room spaces are not yet the ten
usable rooms required for v1.

## Functional v1 program

Public and common-area functions must include:

- a tavern/common gathering area;
- tables and seating appropriate for a functioning inn;
- a serving counter and kitchen area;
- a Messire Valen work/study area;
- circulation sufficient for group use.

Player and housing functions must include:

- exactly ten stable player-room identities;
- ten usable rooms;
- the minimum approved room-usability contract owned by
  [#24](https://github.com/matthieuAlbertelli/Skyrim-Together-Reborn-Enhanced/issues/24);
- persistent player-to-room assignment owned by
  [#25](https://github.com/matthieuAlbertelli/Skyrim-Together-Reborn-Enhanced/issues/25).

Furniture beyond the accepted room-usability and hub requirements is not fixed
by this document.

## Multiplayer and solo constraints

- The standalone solo path must remain usable without an STRE server.
- The primary multiplayer validation target is two to four players.
- Targeted ten-player circulation, scene-position, and room-scale validation is
  still required.
- CK scenes and projected content are not campaign-state authority.
- Room ownership persistence remains server/campaign-owned wherever the existing
  architecture defines shared authoritative state.

## Performance and CK requirements

Acceptance requires audits and in-game checks for:

- navmesh and NPC pathing;
- collision;
- lighting and visual readability;
- clear circulation and controlled clutter;
- acceptable runtime performance, with profiling or runtime evidence used to
  decide whether room bounds, portals, or other explicit visibility
  partitioning are needed;
- save/load behavior;
- ten-player stress, circulation, scene, and room-scale behavior.

## Stable room identities

The housing contract uses ten stable logical identities, conventionally
`Room01` through `Room10`. These identities must not be coupled to:

- load-order-prefixed FormIDs;
- a particular architectural mesh;
- a particular physical headquarters implementation.

The logical room contract must survive a future headquarters replacement. The
assignment timing, persistence schema, reassignment policy, and other semantics
that remain open in #25 are not decided here.

## Explicitly post-v1

The following are not v1 acceptance requirements:

- a seamless exterior/interior headquarters;
- a no-load transition between Tamriel and the inn interior;
- a custom architectural construction kit or bespoke building shell;
- exterior/interior geometric identity;
- custom building LOD;
- an exterior-worldspace weather-shielding solution;
- seamless openable windows directly exposing Tamriel;
- advanced housing customization;
- other custom architectural work not needed for the v1 foundation.

## Future seamless headquarters

A seamless headquarters remains a desired post-v1 replacement or enhancement.
The current candidate direction is a lakeside headquarters in the Lake Ilinalta
region. Preproduction exploration identified approximate coordinates
`X=-34086, Y=-64730`.

These coordinates are **post-v1 preproduction evidence only**. They are not a v1
site, implementation requirement, or accepted technical architecture. A future
physical implementation should reuse or migrate the logical `Room01` through
`Room10` identities without redefining player ownership.

Future design and implementation are tracked by
[#80 — Build a seamless open-world headquarters inn](https://github.com/matthieuAlbertelli/Skyrim-Together-Reborn-Enhanced/issues/80).

## References

- [STRE Roadmap](../../../ROADMAP.md)
- [Current STRE Status](../../project/STATUS.md)
- [Art direction](../../art/ART_DIRECTION.md)
- [Creation Kit implementation](CK_IMPLEMENTATION.md)
- [ADR-0002 — Server-authoritative campaign state](../../architecture/ADRs/ADR-0002-server-authoritative-campaign-state.md)
- [ADR-0003 — Alternate Start remains playable without STRE](../../architecture/ADRs/ADR-0003-alternate-start-standalone.md)
- [ADR-0006 — No hard-coded plugin FormIDs](../../architecture/ADRs/ADR-0006-no-hardcoded-formids.md)
- [ADR-0012 — CK scenes are projections of canonical state](../../architecture/ADRs/ADR-0012-ck-scenes-are-projections.md)
- [ADR-0018 — Fixed roster and coordinated checkpoint recovery](../../architecture/ADRs/ADR-0018-fixed-roster-coordinated-checkpoint-recovery.md)
- [#22 — Complete the headquarters inn and v1 housing foundation](https://github.com/matthieuAlbertelli/Skyrim-Together-Reborn-Enhanced/issues/22)
- [#23 — Finish the headquarters inn as a ten-player campaign hub](https://github.com/matthieuAlbertelli/Skyrim-Together-Reborn-Enhanced/issues/23)
- [#24 — Build ten usable player rooms in the headquarters inn](https://github.com/matthieuAlbertelli/Skyrim-Together-Reborn-Enhanced/issues/24)
- [#25 — Implement persistent player-to-room assignment and restoration](https://github.com/matthieuAlbertelli/Skyrim-Together-Reborn-Enhanced/issues/25)
