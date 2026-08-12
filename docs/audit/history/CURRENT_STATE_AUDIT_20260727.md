# Current-state audit

> **Status: Historical snapshot as of July 27, 2026; non-canonical for current
> state.** See [`docs/project/STATUS.md`](../../project/STATUS.md).
> **Historical baseline:** source audit from July 19, 2026
> **Declared version:** `0.1.0-alpha.1`

## Executive summary

At the snapshot date, STRE had two active technical verticals:

1. an authoritative player-to-player Trading system with reconciliation saga;
2. an Alternate Start character bootstrap combining a Creation Kit plugin, Angular/CEF UI, shared catalog, and server validation.

The second vertical was absent from the July 19 archive. It was subsequently integrated into the repository, compiled on Windows, and smoke-tested in Skyrim, including targeted buffs between two PCs.

## Trading

Trading still included:

- an independent business domain;
- an authoritative server service;
- a dedicated bounded protocol;
- deterministic mutation plans;
- idempotent client application;
- reconciliation to absolute quantities;
- an Angular/CEF UI;
- modular native 3D preview;
- domain and serialization tests.

The model must be described as an **authoritative compensated saga**, not a distributed ACID transaction.

### Trading limitations

- session state was not persisted across a server restart;
- reconnect during a trade still required complete validation;
- no stack splitting or gold trading;
- preview was not published as a stable third-party SDK.

## Item Preview

Internal components included:

- `ItemPreviewController`;
- `ItemPreviewNativeSession`;
- `ItemPreviewHostSession`;
- `ItemPreviewHostBridge`;
- `ItemPreviewFitSolver`;
- `ItemPreviewRasterMeasurer`;
- `TradePreviewHostMenu`.

The platform had a second first-party consumer in the Character Creation screen. This validated internal reuse without resolving the need for a multi-consumer lease manager or stable third-party API.

## Alternate Start — Implemented state

### Creation Kit plugin

Authored files were versioned under `GameFiles/Skyrim`:

- `STRE_AlternateStart.esp`;
- `Source/Scripts/QF_STRE_QUEST_AlternateStart_02001AF9.psc`;
- `Scripts/QF_STRE_QUEST_AlternateStart_02001AF9.pex`.

Confirmed elements:

- `STRE_CELL_AlternateStart` and `STRE_CELL_DevSandbox` cells;
- `STRE_QUEST_AlternateStart` quest;
- stages `0`, `10`, and `20`;
- player and seat aliases;
- player movement, then seating;
- RaceMenu and Character Creation trigger;
- custom equipment, enchantment, spell, and magic-effect records.

The strict `CK_RECORDS_M7_IMPLEMENTED.json` manifest validated 47 expected records. The catalog/ESP audit validated 41 references used by code.

### Character Creation

The client exposed `UiSurface::CharacterCreation` and a `CharacterCreationService` orchestrating:

- RaceMenu;
- Warrior, Mage, or Thief selection;
- loadout groups;
- 3D preview of real Skyrim objects;
- summary;
- final submission;
- local offline or authoritative server path.

### Authoritative build

The current catalog was:

```text
BuildVersion = 5
```

The server derived from logical identifiers:

- canonical inventory;
- inventory hash;
- canonical spell list;
- spell hash;
- equipment metadata.

The client cleaned the imported character, applied the canonical snapshot, verified items and spells actually present, then sent `CharacterBuildAppliedRequest`. The server validated both hashes before marking the build `Applied` and setting server level to 1.

The same catalog was used offline with no mandatory server dependency.

### Mage spells

The Destruction and Alteration choices were materialized:

- 3 Destruction branches with 3 spells each;
- 3 Alteration branches with 4 spells each;
- 7 canonical spells for every Mage combination.

The following French localized buff names were explicitly recognized by the STRE magic hook and smoke-tested on a remote player:

- Égide minérale;
- Souffle aquatique partagé;
- Allègement.

### Anti-import cleanup

The flow removed imported inventory and magic, dispelled temporary effects, restored level 1, and applied the canonical build. It did not yet reset:

- levels and XP for all 18 skills;
- perks and perk points;
- Health/Magicka/Stamina increase history.

## Alternate Start limitations

- a new game was not yet redirected end to end to the inn automatically;
- Helgen skip and exhaustive vanilla-quest resumption remained to implement and test;
- Valen, the introduction scene, and narrative exit were incomplete;
- roster, ready check, Campaign State, late join, and secret Dragonborn were not implemented;
- builds were not durably persisted after reconnect or server restart;
- Conjuration, Illusion, and Restoration remained visible in the UI without canonical rewards;
- Enchanting and several skill kits remained to be materialized;
- completed in-game tests were smoke tests, not exhaustive validation of all nine Mage combinations and every class.

## Architecture actually validated

At the snapshot date, the repository demonstrated two authoritative first-party patterns:

- Trading saga with reconciliation;
- Character Build with canonical snapshot and application acknowledgment.

The generic Mod Integration Framework, public Papyrus bridge, and Campaign State remained proposed architectures. Character Build must not be presented as an already-stable generic SDK.

## Immediate recommendations at the snapshot date

1. Automate the nine Mage-combination tests in the native build and complete in-game testing.
2. Implement build persistence and reconnect before extending authority to the complete campaign.
3. Complete remaining kits from the catalog and V2 workbook.
4. Implement Helgen skip and vanilla resumption before announcing a complete Alternate Start.
5. Add Valen, departure, and Campaign State through small testable milestones.
6. Replace the name-based buff allowlist with a more extensible classification before a third-party SDK.
7. Continue evolving preview toward a lease manager.

## Historical traceability

The July 19, 2026 audit found only Trading/Preview and no Alternate Start in the archive. That finding remains true for that historical archive but is superseded by the state described above. Upstream-baseline details remain in `UPSTREAM.md`.
