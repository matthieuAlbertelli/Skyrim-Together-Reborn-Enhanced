# Alternate Start — Single-player design

> **Status: Local build fallback, New Game interception, and MQ101/post-Helgen projection implemented; MQ102/MQ103 departure handoff remains**

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
- no mandatory server call when STRE is disconnected;
- fresh New Game interception before the cart sequence;
- MQ101/post-Helgen world-state projection without selecting MQ102A/MQ102B.

## Required for a complete single-player new game

The early New Game interception and MQ101/post-Helgen projection are now
implemented. Remaining work is:

- initialize the introduction;
- complete the local ready/departure flow;
- open the exit door after validation;
- project the proven neutral MQ102/MQ103 handoff;
- guarantee the intended route back to the main quest.

## Local state

Current elements:

- `STRE_QUEST_AlternateStart`;
- `STRE_QUEST_HelgenNPCCleanup`;
- player, seat, and Helgen continuity aliases/properties;
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
