# Alternate Start — Creation Kit implementation

> **Status: New Game bootstrap and M7 records implemented and smoke-tested; vanilla Helgen continuity and introduction remain**

## Versioned files

```text
GameFiles/Skyrim/STRE_AlternateStart.esp
GameFiles/Skyrim/Source/Scripts/QF_STRE_QUEST_AlternateStart_02001AF9.psc
GameFiles/Skyrim/Scripts/QF_STRE_QUEST_AlternateStart_02001AF9.pex
GameFiles/Skyrim/Source/Scripts/QF_MQ101_0003372B.psc
GameFiles/Skyrim/scripts/QF_MQ101_0003372B.pex
GameFiles/STRE_AlternateStart.manifest.txt
```

PSC files alone are not executed by Skyrim: the compiled PEX must be retrieved and deployed.

## Confirmed primary records

- `STRE_CELL_AlternateStart`
- `STRE_CELL_DevSandbox`
- `STRE_QUEST_AlternateStart`
- `STRE_FURN_PlayerSeat01`
- `STRE_FURN_PlayerSeat02`
- `STRE_REFR_NewGameStartMarker`

Intentional Skyrim master overrides:

- `MQQuickstart [0004679E]` with value `5`;
- `MQ101 [0003372B]`, adding the STRE stage-0 branch while keeping Bethesda's normal branch intact.

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
Main menu: New Game
→ MQQuickstart = 5
→ MQ101 stage 0 selects the STRE branch
→ Player.MoveTo(STRE_REFR_NewGameStartMarker)
→ STRE_QUEST_AlternateStart.Start()
→ Start Up Stage 10
→ MoveTo the seat through its alias
→ wait for the seated state
→ advance to stage 20
→ CharacterCreationService opens RaceMenu
→ Angular Character Creation
```

The vanilla `MQQuickstart == 0` fragment remains unchanged and still calls `SetStage(10)`. The STRE branch must never call MQ101 stage 10.

`CharacterCreationService` also observes the Alternate Start quest Start/Stop lifecycle. A fresh quest `Start()` rearms the native flow so a second New Game in the same Skyrim process can recover stage 20 even when Skyrim does not emit a second quest-stage event.

Never hard-code a loaded FormID that depends on load order. CK references use aliases and properties; the native catalog uses plugin name plus local FormID.

## M7 records

The `CK_RECORDS_M7_IMPLEMENTED.json` manifest covers 48 expected STRE-owned records:

- cells, quest, and seat references;
- outfits and boots;
- weak enchantments;
- Destruction and Alteration spells;
- targetable magic effects for ally buffs;
- the `STRE_REFR_NewGameStartMarker` placed reference used only for the initial world transition.

The same strict manifest also allows exactly the two intentional EditorID-bearing Skyrim overrides (`MQQuickstart`, `MQ101`) and the pre-existing anonymous `NAVI [00012FB4]` baseline record. Any additional master-backed record is rejected by `--reject-unexpected`.

The three ally buffs must retain compatible values in both `SPEL` and `MGEF`:

```text
Casting Type : Fire and Forget
Delivery     : Target Actor
```

`Contact` is not appropriate for these manually cast spells.

## Navmesh

The cell contains several navmesh fragments. Avoid relying on complex NPC pathfinding until the cell has received a complete CK audit. Every furniture or door change must be followed by a navigation test.

## Remaining implementation

- Helgen skip / vanilla continuity adapter and associated world state;
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

## New Game acceptance

Validated on 15 August 2026:

- New Game enters the inn without running the Helgen cart sequence;
- MQ101 remains at stage 0 while `STRE_QUEST_AlternateStart` reaches stage 20;
- RaceMenu and Character Creation open normally;
- returning to the main menu and starting a second New Game in the same process works;
- loading an ordinary existing save does not retrigger the bootstrap.

## Local test

```text
resetquest STRE_QUEST_AlternateStart
startquest STRE_QUEST_AlternateStart
setstage STRE_QUEST_AlternateStart 10
```
