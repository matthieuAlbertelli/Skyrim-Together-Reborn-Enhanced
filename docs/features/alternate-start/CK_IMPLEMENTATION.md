# Alternate Start — Creation Kit implementation

> **Status: New Game/CK bootstrap and MQ101/post-Helgen projection smoke-tested; native campaign gate automated-tested and manually validated on the Solo/two-player happy path; negative runtime coverage, MQ102/MQ103 handoff, introduction, and Departure remain**

## Versioned files

```text
GameFiles/Skyrim/STRE_AlternateStart.esp
GameFiles/Skyrim/Source/Scripts/QF_STRE_QUEST_AlternateStart_02001AF9.psc
GameFiles/Skyrim/scripts/QF_STRE_QUEST_AlternateStart_02001AF9.pex
GameFiles/Skyrim/Source/Scripts/QF_MQ101_0003372B.psc
GameFiles/Skyrim/scripts/QF_MQ101_0003372B.pex
GameFiles/Skyrim/Source/Scripts/QF_STRE_HelgenCleanup_0302D022.psc
GameFiles/Skyrim/scripts/QF_STRE_HelgenCleanup_0302D022.pex
GameFiles/Skyrim/Source/Scripts/STRE_HelgenContinuityController.psc
GameFiles/Skyrim/scripts/STRE_HelgenContinuityController.pex
GameFiles/Skyrim/Source/Scripts/STRE_HelgenCollapseLoadAlias.psc
GameFiles/Skyrim/scripts/STRE_HelgenCollapseLoadAlias.pex
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

Intentional Skyrim master overrides include:

- `MQQuickstart [0004679E]` with value `5`;
- `MQ101 [0003372B]`, adding the STRE stage-0 and continuity branches while
  keeping the audited vanilla branches intact;
- `Tamriel [0000003C]` and `HelgenKeep01 [0005DE24]` structural parents retained
  by the CK for persistent continuity references;
- `MQ101SetStage360 [0005CEE3]`, `MQ101SetStage400 [000BA032]`,
  `MQ101SetStage267 [000BAC16]`, `MQ101SetStage368 [000F778E]`,
  `MQ101SetStage485 [000FDA33]`, and `MQ101SetStage210 [00103AF4]`, promoted as
  required by CK property binding;
- three anonymous approved master-backed records validated by exact signature
  and FormID: `NAVI [00012FB4]`, `CELL [00000D74]`, and `REFR [0010FDE3]`.

The strict manifest is the authority for this allowlist; do not infer approval
for any additional master-backed record from this prose summary.

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
→ CharacterCreationService locks controls and opens the campaign-bootstrap CEF gate
→ Solo authorizes locally, or canonical sealed CharacterCreation + full-roster ACTIVE authorizes multiplayer
→ CharacterCreationService opens RaceMenu
→ Angular Character Creation
```

This gate is entirely native/CEF. No ESP, PSC, PEX, quest stage, or generic
quest-synchronization change was required for the lobby slice.

Runtime prerequisite: `Alternate Start - Live Another Life` must be disabled
when `STRE_AlternateStart.esp` is active. Running both was observed to be
incompatible during validation. Compatibility work is outside this slice.

The vanilla `MQQuickstart == 0` fragment remains unchanged and still calls `SetStage(10)`. The STRE branch must never call MQ101 stage 10.

`CharacterCreationService` also observes the Alternate Start quest Start/Stop lifecycle. A fresh quest `Start()` rearms the native flow so a second New Game in the same Skyrim process can recover stage 20 even when Skyrim does not emit a second quest-stage event.

Never hard-code a loaded FormID that depends on load order. CK references use aliases and properties; the native catalog uses plugin name plus local FormID.

## M7 records and continuity helper

The `CK_RECORDS_M7_IMPLEMENTED.json` manifest covers 49 expected STRE-owned records:

- cells, quest, and seat references;
- outfits and boots;
- weak enchantments;
- Destruction and Alteration spells;
- targetable magic effects for ally buffs;
- the `STRE_REFR_NewGameStartMarker` placed reference used only for the initial world transition;
- `STRE_QUEST_HelgenNPCCleanup`, which owns the skipped-Helgen cleanup aliases
  and the continuity controller used by the post-Helgen projection.

The same strict manifest allows only the explicit named and anonymous
Skyrim-master records listed in its allowlists. Any additional master-backed
record is rejected by `--reject-unexpected`.

The three ally buffs must retain compatible values in both `SPEL` and `MGEF`:

```text
Casting Type : Fire and Forget
Delivery     : Target Actor
```

`Contact` is not appropriate for these manually cast spells.

## MQ101 / post-Helgen continuity

The current STRE continuity path deliberately stops at a neutral boundary:

```text
MQ101  -> stage 1000 / stopped
MQ102  -> untouched
MQ102A -> untouched
MQ102B -> untouched
```

The Alternate Start generated fragment advances the audited MQ101 continuity
stages, starts `STRE_QUEST_HelgenNPCCleanup`, removes or repositions the skipped
Helgen actors, then delegates complex world-reference projection to
`STRE_HelgenContinuityController`.

The controller owns the validated destroyed-Helgen enable/disable projection
and collapse-trigger neutralization. `STRE_HelgenCollapseLoadAlias` applies the
already-collapsed rubble visual when `HelgenKeep01` attaches. The implementation
does not call the vanilla collapse trigger and does not select an Imperial or
Stormcloak MQ102 branch.

This is a local Skyrim projection. The server must never synchronize raw
`MQ101.SetStage` or future `MQ102.SetStage` calls as campaign protocol.

## Navmesh

The cell contains several navmesh fragments. Avoid relying on complex NPC pathfinding until the cell has received a complete CK audit. Every furniture or door change must be followed by a navigation test.

## Remaining implementation

- neutral MQ102/MQ103 vanilla-continuity handoff and its Riverwood/Alduin/Civil
  War semantics;
- Valen, scenes, dialogue, and aliases;
- real Departure/exit flow and main-quest resumption;
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
py -3 .\Tools\Scripts\audit_mq101_quickstart5.py `
  .\GameFiles\Skyrim\STRE_AlternateStart.esp
```

```powershell
py -3 .\Tools\Scripts\audit_mq101_generated_invariants.py `
  .\GameFiles\Skyrim\Source\Scripts\QF_MQ101_0003372B.psc
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
- during the character-creation bootstrap, MQ101 remains at stage 0 while
  `STRE_QUEST_AlternateStart` reaches stage 20;
- RaceMenu and Character Creation open normally;
- returning to the main menu and starting a second New Game in the same process works;
- loading an ordinary existing save does not retrigger the bootstrap.

Post-Helgen continuity validated on 16 August 2026 after xEdit Quick Auto Clean:

- MQ101 reaches stage 1000 and stops;
- MQ102, MQ102A, and MQ102B remain untouched;
- skipped Helgen Keep victims are cleaned up;
- Hadvar/Ralof and the residual Imperial guard no longer remain in their skipped
  intro positions;
- Helgen exterior is projected to the destroyed post-attack state;
- Helgen Keep rubble is already collapsed, proximity does not replay the
  collapse, and the vanilla dragon/collapse roar is suppressed;
- the known minor limitation is a brief rubble sound while `HelgenKeep01` loads.

## Local test

```text
resetquest STRE_QUEST_AlternateStart
startquest STRE_QUEST_AlternateStart
setstage STRE_QUEST_AlternateStart 10
```
