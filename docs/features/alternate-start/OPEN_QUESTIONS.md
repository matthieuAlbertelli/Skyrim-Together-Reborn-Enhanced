# Alternate Start — Open questions

> **Status: Decisions remaining after M7**

## Resolved decisions

- the plugin must work without an STRE server;
- the client sends logical selections, never the final inventory;
- the server constructs canonical inventory and spells;
- FormIDs are resolved by plugin plus local ID;
- anti-import cleanup is destructive by design;
- Lockpicking grants 10 vanilla lockpicks;
- outfits for the same skill differ visually, not through unrelated bonuses;
- Enchanting receives a visual outfit and disenchantable magic items;
- ally buffs use `Target Actor`, not `Contact`.

## Persistence and reconnection

- server save format for the build;
- relationship between the Skyrim save and STRE snapshot;
- restoration before or after character spawn;
- migration between `BuildVersion` values;
- policy when the plugin or catalog changes.

## Character reset

- levels and XP for all 18 skills;
- acquired perks and perk points;
- Health/Magicka/Stamina history;
- policy for racial powers and permanent effects from mods.

## New game and Helgen skip

- exact interception point;
- vanilla stages and globals to modify;
- unlocking dragons and shouts;
- compatibility with other alternate starts;
- route back to the main quest.

## Remaining kits

- exact contents of the three Enchanting kits;
- Conjuration, Illusion, and Restoration;
- outfits and kits for utility skills not yet materialized;
- balance after in-game tests;
- disenchantment policy and stacking of weak enchantments.

## Campaign

- roster and character-binding model;
- ready check;
- late-join cutoff;
- absent or disconnected Dragonborn;
- duplicate classes;
- changing or respeccing before and after departure.

## Dialogue and Valen

- identical local scene or time synchronization;
- local responses, voting, or leader choice;
- group skip;
- summary for late join;
- limits at 2, 4, and 10 players.

## Cooperative spells

- replace the name-based `MagicItem::IsBuffSpell` allowlist with a keyword or capability;
- define friendly fire and stacking;
- shared elemental cloaks;
- authority and replication for future scripted effects.
