# Carte des dépendances

> **Statut : relations structurelles courantes**

Ce document décrit **les dépendances entre sous-systèmes**, pas leur avancement.

## World Sync

```text
Skyrim reference / inventory event
  → client InventoryService / WorldEntity lifecycle
  → WorldEntity protocol
  → server InventoryService authority
  → remote binding/materialization
  → local Havok
  → authoritative settlement
```

Manipulation :

```text
Better Grabbing (external SKSE plugin)
  → Skyrim grab/release behavior/events
  → STRE WorldEntity manipulation lifecycle
  → server authority
  → remote hide/release
```

Dependency gate:

```text
Authentication
  → generic NativePlugins inventory
  → ModPolicy:sRequiredNativePlugins
  → BetterGrabbing.dll required by default
```

## Trading

```text
Trade domain
  → protocol
  → server TradeService
  → client TradeService
  → Angular UI
  → Item Preview
```

Inventory metadata limits feed the trade eligibility policy; unsupported instance metadata must fail closed.

## Item Preview

```text
NativeSession + Controller + Solver + RasterMeasurer + Host
  → Trading
  → Character Creation
```

Future lease arbitration sits between consumers and the native host.

## Alternate Start / Character Build

```text
CK quest + RaceMenu
  → Character Creation UI
  → logical selections
  → CharacterBuildCatalog
  → server CharacterBuildService OR local fallback
  → canonical inventory/spells
  → local application + acknowledgement
```

CK record/catalog coupling:

```text
STRE_AlternateStart.esp
  ↔ CK_RECORDS_M7_IMPLEMENTED.json
  ↔ CharacterBuildCatalog
  ↔ character-loadouts.ts
  ↔ tests/audits
```

## Campaign

```text
Character binding/persistence
  → Campaign State
  → roster/ready
  → shared introduction/departure
  → late join/reconnect
```

## v1 release-gate coverage

This table maps durable Roadmap outcomes to their actionable GitHub path. It
does not mirror status; the Project and Milestone own live state.

| `ROADMAP.md` v1 outcome | Actionable path |
|---|---|
| Complete Alternate Start and character creation | [#9](https://github.com/matthieuAlbertelli/Skyrim-Together-Reborn-Enhanced/issues/9), decomposed into #10–#12 plus integrated domain parents |
| All 21 canonical classes | [#13](https://github.com/matthieuAlbertelli/Skyrim-Together-Reborn-Enhanced/issues/13), with roster decision #14 and class issues #15–#17 / #32–#49 |
| Starting kits/loadouts for every class | Acceptance criteria of each of the 21 class issues under #13 |
| Cooperative ability/perk for every class | Acceptance criteria of each of the 21 class issues under #13 |
| One personal quest per class | [#18](https://github.com/matthieuAlbertelli/Skyrim-Together-Reborn-Enhanced/issues/18), integrated by the owning class issue |
| Messire Valen | [#19](https://github.com/matthieuAlbertelli/Skyrim-Together-Reborn-Enhanced/issues/19) with #20–#21 |
| Headquarters inn | [#22](https://github.com/matthieuAlbertelli/Skyrim-Together-Reborn-Enhanced/issues/22) with #23 |
| Ten player rooms | #22 with #24 |
| Persistent player-room assignment | #22 with #25, backed by campaign persistence #27 |
| Campaign continuity needed by Alternate Start | [#26](https://github.com/matthieuAlbertelli/Skyrim-Together-Reborn-Enhanced/issues/26) with #27–#28 |
| End-to-end release evidence | [#30](https://github.com/matthieuAlbertelli/Skyrim-Together-Reborn-Enhanced/issues/30) |
| No known P0 multiplayer campaign blocker | [#51](https://github.com/matthieuAlbertelli/Skyrim-Together-Reborn-Enhanced/issues/51), grouping inherited symptoms #2–#6 without pre-assigning priority |

No extra issue is needed for kits or cooperative abilities: each is an explicit
acceptance dimension of every class contract. Personal quests remain one program
issue until individual quest outlines become independently executable; creating
21 empty quest placeholders would add hierarchy without actionable content.

## Major v1 issue dependencies

GitHub's native `blocked by` relationships represent only completion blockers,
not every integration touchpoint:

```text
#27 campaign persistence
  ├─blocks→ #28 roster/readiness/phases
  ├─blocks→ #25 persistent room assignment
  └─blocks→ #18 persistent personal-quest program

#28 phase contract
  ├─blocks→ #20 Valen collective behavior
  ├─blocks→ #21 Valen CK/actor integration
  ├─blocks→ #23 headquarters phase-ready layout
  └─blocks→ #12 authoritative departure

#20 Valen behavior ─blocks→ #21 and #23
#23 headquarters layout ─blocks→ #24 room construction
#24 room identities ─blocks→ #25 assignment/restoration

#10 Helgen continuity ─┐
#19 Valen             ├─block→ #12 departure/vanilla continuity
#28 campaign phases   ─┘

#10, #11, #12, #13, #18, #19, #22 and #26
  └─block→ #30 end-to-end v1 Alternate Start evidence
```

Parent/sub-issue relationships express decomposition, so they are not duplicated
as blocker edges. Design can proceed in parallel where only part of a contract is
needed; add a dependency only when the blocked issue cannot satisfy its acceptance
criteria before the blocker is complete.

## Release dependency

```text
v1 product issues + #30 end-to-end evidence + #51 stabilization audit/disposition
  → release-candidate evidence review
  → no open known priority:P0 campaign blocker
  → immutable STRE tag + GitHub Release
```

## Mod Integration

```text
first-party feature contracts
  → generalized capability model
  → adapter registry/version negotiation
  → Papyrus/C++ SDK
```

Do not invert this dependency by freezing a generic SDK before enough first-party contracts are proven.
