# Alternate Start

> **Status: Character Build, New Game bootstrap, and MQ101/post-Helgen projection implemented and smoke-tested; MQ102/MQ103 handoff and full campaign remain**

## Current outcome

A fresh Skyrim **New Game** is intercepted before vanilla MQ101 stage 10, moves the player into the STRE inn, starts the existing Alternate Start quest, opens RaceMenu, then continues through class/loadout selection and build sealing.

The current continuity step also projects the required MQ101/post-Helgen state without replaying the vanilla intro: MQ101 reaches its post-chargen completion boundary, skipped Helgen actors/references are cleaned up, destroyed Helgen is projected, and MQ102/MQ102A/MQ102B remain untouched for the next dedicated vanilla-continuity increment.

The flow works:

- from the main-menu **New Game** action without starting the Helgen cart flow;
- twice in the same Skyrim process after returning to the main menu;
- without retriggering when an ordinary save is loaded;
- locally without a server;
- in multiplayer with server validation of inventory and spells;
- with Warrior, Mage, and Thief;
- with the Mage Destruction/Alteration vertical slice;
- with functional targeted buffs on another player;
- with the validated MQ101/post-Helgen projection applied without starting
  MQ102, MQ102A, or MQ102B.

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
- `NEW_GAME_BOOTSTRAP_SPIKE.md` — New Game interception investigation plus the validated implementation result and remaining continuity work;
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
