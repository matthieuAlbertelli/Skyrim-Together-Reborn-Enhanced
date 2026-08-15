# Alternate Start

> **Status: Character Build vertical slice implemented and smoke-tested; full campaign in progress**

## Current outcome

The player can be placed in the inn, open RaceMenu, choose a class and kits in the Angular interface, review the summary, and seal the build.

The flow works:

- locally without a server;
- in multiplayer with server validation of inventory and spells;
- with Warrior, Mage, and Thief;
- with the Mage Destruction/Alteration vertical slice;
- with functional targeted buffs on another player.

The current catalog is `BuildVersion = 5`.

For the current project state and global limitations, see [`docs/project/STATUS.md`](../../project/STATUS.md).

## Feature sources of truth

Priority order for implemented behavior:

1. `Code/common/CharacterCreation/CharacterBuildCatalog.*` — canonical rules;
2. `Code/skyrim_ui/src/app/data/character-loadouts.ts` — UI presentation and choices;
3. `CK_RECORDS_M7_IMPLEMENTED.json` and `STRE_AlternateStart.esp` — CK records;
4. `CK_IMPLEMENTATION.md` — maintained CK behavior;
5. `CLASS_ROSTER_V1.md` — canonical product roster of 21 v1 classes and their major/minor assignments;
6. `PRODUCT_SPEC.md` and `STATE_MODEL.md` — functional target;
7. `SKILL_EQUIPMENT_KITS_V2.xlsx` — kit design matrix to continue.

Documents under `history/` are dated evidence and milestones; they do not replace these sources.

## Current documents

- `PRODUCT_SPEC.md` — product target;
- `CLASS_ROSTER_V1.md` — canonical roster of 21 v1 classes;
- `SOLO_DESIGN.md` — operation without a server;
- `STRE_ADAPTER_SPEC.md` — current and target cooperative semantics;
- `CK_IMPLEMENTATION.md` — CK records and flow;
- `NEW_GAME_BOOTSTRAP_SPIKE.md` — proposed CK/Papyrus New Game interception architecture and validation plan;
- `STATE_MODEL.md` — build and campaign state;
- [`../../architecture/CAMPAIGN_STATE.md`](../../architecture/CAMPAIGN_STATE.md)
  and [ADR-0018](../../architecture/ADRs/ADR-0018-fixed-roster-coordinated-checkpoint-recovery.md)
  — canonical multiplayer roster/checkpoint recovery contract;
- `TEST_PLAN.md` — validation;
- `OPEN_QUESTIONS.md` — remaining decisions;
- `CK_RECORDS_M7_IMPLEMENTED.json` — strict manifest;
- `SKILL_EQUIPMENT_KITS_V2.xlsx` — kit design.

## History

- `history/M7_CK_CODE_INTEGRATION_20260727.md` — M7 milestone validation;
- `history/SKILL_LOADOUTS_v0.1_fr.md` — archived French V0.1 design retained as historical evidence.

## Local test commands

```text
resetquest STRE_QUEST_AlternateStart
startquest STRE_QUEST_AlternateStart
setstage STRE_QUEST_AlternateStart 10
```

## Dependencies

- `Skyrim.esm`, `Update.esm`, and `Dragonborn.esm` as required by the records;
- the STRE client for the native/Angular UI and multiplayer path;
- the STRE server for the authoritative path;
- no server required for the local fallback.
