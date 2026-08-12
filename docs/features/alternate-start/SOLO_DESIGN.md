# Alternate Start — Single-player design

> **Status: Local build fallback implemented; complete new-game bootstrap proposed**

## Principle

The CK plugin and Character Creation must remain usable without an STRE server. The same catalog of classes, items, and spells is applied locally.

## Implemented

- Alternate Start quest and cell;
- movement to the table;
- RaceMenu;
- class and loadout UI;
- character cleanup;
- local application of inventory, spells, and equipment;
- level 1;
- no mandatory server call when STRE is disconnected.

## Required for a complete single-player new game

- intercept startup before the cart sequence;
- cleanly neutralize or advance the vanilla states associated with Helgen;
- initialize the introduction;
- open the exit door after validation;
- guarantee a route back to the main quest.

## Local state

Current elements:

- `STRE_QUEST_AlternateStart`;
- player and seat aliases;
- local Character Creation state in the client service.

Possible future elements:

- phase global;
- Valen alias;
- introduction completed;
- departure completed.

## Classes and packages

The behavior actually applied is defined by:

- `CharacterBuildCatalog.*`;
- `character-loadouts.ts`;
- `CK_RECORDS_M7_IMPLEMENTED.json`.

Expanded design remains in [`SKILL_EQUIPMENT_KITS_V2.xlsx`](SKILL_EQUIPMENT_KITS_V2.xlsx). [`SKILL_LOADOUTS_v0.1_fr.md`](history/SKILL_LOADOUTS_v0.1_fr.md) is an archived French V0.1 design and must not replace the current catalog.

## Exit and vanilla resumption

The future door must activate only after local build validation. Resumption must be tested against:

- the main quest;
- dragons and shouts;
- progression toward Whiterun;
- the Civil War and quests sensitive to Helgen progression.

## Saving

Single-player states must remain in the Skyrim save. No blocker should occur if STRE is installed and later unavailable when the save is loaded.
