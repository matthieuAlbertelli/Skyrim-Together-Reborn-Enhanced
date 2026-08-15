# Alternate Start — Test plan

> **Status: Automated audits and M7 smoke tests executed; exhaustive coverage remains**

## Completed checks

- successful Windows xmake build;
- conforming strict audit of 47 CK records;
- conforming audit of 41 catalog/ESP references;
- compiled `Code/tests/character_build.cpp` tests;
- in-game Mage bootstrap test;
- targeted buffs tested between two PCs;
- single-player fallback present in the service and tested through the build flow.

These checks do not constitute exhaustive validation of every combination.

## Static audits

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

## Local smoke test

```text
resetquest STRE_QUEST_AlternateStart
startquest STRE_QUEST_AlternateStart
setstage STRE_QUEST_AlternateStart 10
```

Verify RaceMenu, UI, preview, summary, level 1, exact inventory, equipment, exact spells, cleanup, and absence of hash rejection.

## Mage matrix

Test all nine combinations:

- Fire × Protection;
- Fire × Exploration;
- Fire × Matter;
- Frost × Protection;
- Frost × Exploration;
- Frost × Matter;
- Shock × Protection;
- Shock × Exploration;
- Shock × Matter.

Each build must produce exactly 7 canonical spells: 3 Destruction plus 4 Alteration.

## Priority multiplayer tests

- identical client/server/plugin versions on both PCs;
- independent creations with different choices;
- Accepted, then Applied states;
- no `RejectedInventoryHash` or `RejectedSpellHash`;
- remote appearance and equipment;
- Mineral Aegis: `DamageResist` increases, then returns;
- Water Breathing: effect active, then expires;
- Lighten Burden: `CarryWeight` increases, then returns;
- no application to the wrong target;
- controlled recasting and stacking.

The three spell names above are translated descriptions of currently localized French display strings; runtime IDs remain unchanged.

## Class regression tests

- Warrior: heavy equipment, weapons, smithing, pendant;
- Thief: outfits, weapons, 10 lockpicks;
- Mage: visual outfit, 7 spells;
- build change before acknowledgment;
- reject a second build after the Applied state.

## New Game bootstrap regression

Validated on 15 August 2026:

- first New Game enters the inn and reaches Character Creation;
- MQ101 remains at stage 0 and the STRE Alternate Start quest reaches stage 20;
- a second New Game works after returning to the main menu without restarting Skyrim;
- an ordinary existing save loads without retriggering the bootstrap.

## Tests still blocked by missing features

- Helgen/world-state continuity and vanilla main-quest handoff;
- Valen and scene;
- exit and vanilla resumption;
- save/load at every phase;
- sealed roster, coordinated checkpoints, disconnect recovery, and collective
  build/campaign restoration;
- 4 and 10 players.

## Future campaign continuity matrix

Roster and activation:

- extra player after roster seal is rejected;
- replacement player and wrong `CharacterBinding` are rejected;
- activation with one missing roster member is rejected;
- full roster activates only with every expected slot/binding.

Disconnect boundaries:

- disconnect during open world;
- disconnect during combat;
- disconnect during dialogue or a scene;
- disconnect around a quest-stage change;
- disconnect during checkpoint creation;
- no persistent campaign mutation or new checkpoint is accepted while in
  `RECOVERY_LOCK`.

Checkpoint failure and restore:

- one player save failure;
- client crash during a candidate checkpoint;
- server interruption before and after the checkpoint commit boundary;
- a failed candidate leaves the previous committed checkpoint valid;
- all clients load the exact checkpoint and per-slot save selected by the server;
- wrong, stale, missing, or mismatched save metadata is rejected;
- restored server revision matches the selected checkpoint;
- resume occurs only after all roster members acknowledge the same restore;
- retry and duplicate/delayed acknowledgements are idempotent;
- no duplicate build grants occur after collective restore.

## Log collection

For every test, retain the date, runtime, load order, BuildVersion, client/server versions, logical choices, and `CharacterBuild`/`CharacterCreation`/`MagicService` lines. `_audit` reports, TSV files, and logs remain local and are not versioned.
