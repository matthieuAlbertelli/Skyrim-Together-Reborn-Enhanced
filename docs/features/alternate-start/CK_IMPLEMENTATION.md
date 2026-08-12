# Alternate Start — Creation Kit implementation

> **Status: Bootstrap and M7 records implemented; introduction and Helgen skip remain**

## Versioned files

```text
GameFiles/Skyrim/STRE_AlternateStart.esp
GameFiles/Skyrim/Source/Scripts/QF_STRE_QUEST_AlternateStart_02001AF9.psc
GameFiles/Skyrim/Scripts/QF_STRE_QUEST_AlternateStart_02001AF9.pex
GameFiles/STRE_AlternateStart.manifest.txt
```

PSC files alone are not executed by Skyrim: the compiled PEX must be retrieved and deployed.

## Confirmed primary records

- `STRE_CELL_AlternateStart`
- `STRE_CELL_DevSandbox`
- `STRE_QUEST_AlternateStart`
- `STRE_FURN_PlayerSeat01`
- `STRE_FURN_PlayerSeat02`

Aliases used:

- `Alias_Player`
- `Alias_PlayerSeat01`

Quest stages:

- `0` — initialization;
- `10` — move and seat the player;
- `20` — trigger Character Creation.

The quest is intentionally excluded from generic quest synchronization.

## Current flow

```text
setstage 10
→ MoveTo the seat through its alias
→ wait for the seated state
→ advance to stage 20
→ CharacterCreationService receives TESQuestStageEvent
→ RaceMenu, then Angular UI
```

Never hard-code a loaded FormID that depends on load order. CK references use aliases and properties; the native catalog uses plugin name plus local FormID.

## M7 records

The `CK_RECORDS_M7_IMPLEMENTED.json` manifest covers 47 expected records:

- cells, quest, and seat references;
- outfits and boots;
- weak enchantments;
- Destruction and Alteration spells;
- targetable magic effects for ally buffs.

The three ally buffs must retain compatible values in both `SPEL` and `MGEF`:

```text
Casting Type : Fire and Forget
Delivery     : Target Actor
```

`Contact` is not appropriate for these manually cast spells.

## Navmesh

The cell contains several navmesh fragments. Avoid relying on complex NPC pathfinding until the cell has received a complete CK audit. Every furniture or door change must be followed by a navigation test.

## Remaining implementation

- clean new-game interception;
- Helgen skip and associated vanilla state;
- Valen, scenes, dialogue, and aliases;
- exit door and main-quest resumption;
- markers and placements for more players;
- campaign scripts and generic bridge;
- automated Papyrus compilation.

## Audits

```powershell
py -3 .\Tools\Scripts\audit_stre_plugin_records.py `
  .\GameFiles\Skyrim\STRE_AlternateStart.esp `
  --manifest .\docs\features\alternate-start\CK_RECORDS_M7_IMPLEMENTED.json `
  --output .\_audit\STRE_AlternateStart.records.m7.tsv `
  --strict `
  --reject-unexpected
```

```powershell
py -3 .\Tools\Scripts\audit_character_build_catalog.py `
  .\GameFiles\Skyrim\STRE_AlternateStart.esp `
  .\Code\common\CharacterCreation\CharacterBuildCatalog.cpp `
  --client-source .\Code\client\Services\Generic\CharacterCreationService.cpp
```

Reports under `_audit/*.tsv` and logs are generated locally and must not be committed.

## Local test

```text
resetquest STRE_QUEST_AlternateStart
startquest STRE_QUEST_AlternateStart
setstage STRE_QUEST_AlternateStart 10
```
