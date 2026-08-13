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
- the STRE server, not the host/Session Manager or a host save, is persistent
  authority for shared campaign state;
- the formal multiplayer campaign start/commit seals the pre-campaign lobby
  roster before `CharacterCreation`; `Departure`/`OpenWorld` do not seal it;
- slots, `PlayerId` values, and `CharacterBinding` identities are then immutable
  for that v1 campaign's lifetime;
- campaign late join and player replacement are not supported in v1;
- campaign progression requires the complete sealed roster;
- a required disconnect suspends progression and collective rollback restores
  every member from the same committed `CampaignCheckpoint`;
- every roster member retains a native Skyrim `.ess` for local
  Skyrim/Papyrus/quest runtime restoration; it is not shared-state authority.

## Persistence and checkpoints

- versioned server storage schema and migrations for campaign/checkpoint state;
- checkpoint cadence, triggers, and safe points;
- interaction with autosaves, manual saves, combat, dialogue, and cell transitions;
- exact native-save identity/fingerprint and dedicated-save management;
- failure and retry policy when one player cannot save or load;
- restoration ordering around character spawn;
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

- ready-check rules after the already-sealed campaign has started;
- duplicate classes;
- changing or respeccing before and after departure.

## Recovery presentation

- engine-safe pause/freeze mechanism during recovery lock;
- UI treatment for missing members, checkpoint progress, retry, and unavailable
  expected saves;
- diagnostics and privacy boundaries for roster/recovery information;
- post-v1 partial-roster progression, roster mutation, and catch-up, if a future
  persistence model can support them safely.

## Dialogue and Valen

- identical local scene or time synchronization;
- local responses, voting, or leader choice;
- group skip;
- checkpoint-safe interruption and collective scene restore;
- limits at 2, 4, and 10 players.

## Cooperative spells

- replace the name-based `MagicItem::IsBuffSpell` allowlist with a keyword or capability;
- define friendly fire and stacking;
- shared elemental cloaks;
- authority and replication for future scripted effects.
