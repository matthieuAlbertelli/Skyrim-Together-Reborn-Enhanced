# M7 — CK record integration into the authoritative build

> **Document status: dated historical evidence.** For current state, see
> [`docs/project/STATUS.md`](../../../project/STATUS.md) and the
> [Alternate Start README](../README.md).

## Status

> **Implemented, compiled, and smoke-tested in game on July 27, 2026.**

This milestone integrated clothing, enchantments, and spells from `STRE_AlternateStart.esp` into the shared catalog, network protocol, server, client, and UI.

```text
BuildVersion = 5
```

## Validation performed

- successful Windows/xmake build;
- strict manifest: 47 expected records conform;
- catalog/ESP: 41 references conform;
- Mage Character Build applied in game;
- inventory and spell hashes accepted by the server;
- targeted buffs applied to and synchronized with another player after correcting the STRE allowlist.

The initial CK anomalies (duplicate record, Water Breathing name, and Aegis effects) were corrected and are no longer open actions.

## Integrated equipment

### Warrior

- blacksmith outfit and boots;
- vanilla heavy equipment and weapon choices;
- weakly enchanted guard pendant for the corresponding option;
- smithing material kits.

### Mage

- Enchanting outfit and boots, purely visual;
- no Enchanting improvement carried by the robe;
- canonical spells based on Destruction × Alteration.

### Thief

- 10 vanilla lockpicks;
- crowd outfit;
- noble outfit;
- discreet outfit;
- weapons and equipment based on selections.

The apothecary outfit exists in the ESP and previews but is not yet granted by a currently exposed class.

## Integrated spells

The following spell names are the French localized display strings recorded at the milestone; their technical record IDs were not changed.

### Destruction

```text
Feu
- Flammes
- Explosion de braises
- Rune de braises

Froid
- Souffle de givre mineur
- Explosion de givre
- Rune de givre mineure

Foudre
- Étincelles mineures
- Décharge explosive
- Rune électrique mineure
```

### Alteration

```text
Protection et contrôle
- Peau minérale mineure
- Entrave mineure
- Rune de cendres mineure
- Égide minérale

Exploration et perception
- Lueur mineure
- Détection du vivant mineure
- Détection des morts mineure
- Souffle aquatique partagé

Manipulation de la matière
- Télékinésie mineure
- Transmutation mineure
- Équilibre mineur
- Allègement
```

Each combination produces 7 canonical spells: 3 Destruction plus 4 Alteration.

## Cooperative targeted buffs

All three ally buffs use these values in both `SPEL` and `MGEF`:

```text
Casting Type : Fire and Forget
Delivery     : Target Actor
```

`Contact` does not cast correctly for this type of manually cast spell.

Skyrim Together's `MagicTarget` hook permits effects on a remote player only for healing or spells recognized as buffs. `MagicItem::IsBuffSpell()` therefore resolves these records through `STRE_AlternateStart.esp`:

- `STRE_SPEL_Alteration_Protection_AllyMineralAegis`;
- `STRE_SPEL_Alteration_Exploration_AllyWaterbreathing`;
- `STRE_SPEL_Alteration_Matter_AllyFeather`.

The IDs are local to the plugin; no loaded load-order prefix is hard-coded.

## Authoritative spell protocol

`CharacterBuildSnapshotData` contains:

```text
CanonicalSpells
SpellHash
```

The server:

1. validates selections;
2. resolves the plugin and local FormID;
3. sorts and deduplicates spells;
4. computes a normalized FNV hash;
5. sends the canonical snapshot;
6. validates `SpellHash` in the acknowledgment.

The client:

1. removes imported spells;
2. resolves `GameId` values into loaded FormIDs;
3. verifies the `SpellItem` type;
4. runs `player.addspell` through `Script::CompileAndRun`;
5. verifies actual presence;
6. acknowledges inventory and spells.

The same catalog is applied offline.

## Automated checks

`Code/tests/character_build.cpp` covers:

- all nine Destruction × Alteration combinations;
- spell uniqueness and count;
- simplified Thief package;
- 10 lockpicks;
- hash normalization;
- snapshot and acknowledgment serialization.

Audits:

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

Validated results:

```text
Expected records: 47
Result: conforming.

Checked references: 41
Result: conforming.
```

## Success logs

```text
[STRE][CharacterBuild][Server] Build accepted ... spellCount=7 spellHash=...
[STRE][CharacterCreation] Spell grant applied form=...
[STRE][CharacterBuild][Client] Canonical spells applied ... count=7 spellHash=...
[STRE][CharacterBuild][Client] Applied acknowledgement sent ... spellHash=...
[STRE][CharacterBuild][Server] Build applied ... spellHash=... level=1
```

For a remote buff, the magic flow must produce the corresponding STRE target and synchronization events.

## Known limitations

- builds are not durably persisted after reconnecting;
- incomplete reset of skills, perks, and attribute history;
- Conjuration, Illusion, and Restoration are not granted;
- Enchanting kits are not materialized;
- shared elemental cloaks are deferred;
- buff allowlist is specific to the three M7 FormIDs;
- in-game validation remains at smoke-test level, not an exhaustive matrix of every class and combination.
