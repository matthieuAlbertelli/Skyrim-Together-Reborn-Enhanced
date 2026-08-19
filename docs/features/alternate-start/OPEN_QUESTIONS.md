# Alternate Start — Open questions

> **Status: Remaining decisions after New Game and MQ101/post-Helgen continuity validation**

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
  Skyrim/Papyrus/quest runtime restoration; it is not shared-state authority;
- before the four-day transition, Hadvar and Ralof are independently projected
  as wounded survivors in the final `HelgenKeep01` cave section;
- the recent-post-attack route keeps the validated collapsed rubble intact and
  exposes a local bidirectional `Se faufiler` interaction through an opening
  explained by a dead bandit and abandoned pickaxe;
- `STRE_QUEST_HelgenInvestigation` is a local CK orchestration/projection quest
  and is excluded from generic quest-stage synchronization.

## Persistence and checkpoints

- migration policy between future campaign persistence schema/codec versions;
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

## Vanilla handoff after Helgen

Resolved:

- New Game interception is the dedicated `MQQuickstart == 5` MQ101 stage-0
  branch before vanilla stage 10;
- MQ101/post-Helgen projection now reaches the validated stage-1000/destroyed
  Helgen boundary while leaving MQ102/MQ102A/MQ102B untouched;
- the first pre-deadline investigation slice can project both Hadvar and Ralof
  independently into wounded positions without choosing MQ102A or MQ102B;
- the collapsed Keep route remains intact and the survivors are reachable
  through the STRE-owned squeeze interaction rather than a navmesh/collision
  rewrite.

Remaining:

- exact four-day deadline evaluation and the safe boundary for physically
  changing Helgen while players may be nearby;
- exact vanilla/STRE bandit-occupation projection after the deadline;
- prison markers, AI packages, and release behavior for each unsaved survivor;
- rescue interaction and the exact transition from `WoundedInCave` or
  `CapturedInKeep` to `Freed`;
- whether the current vanilla corpse ActorBase used for the rubble excavator
  actually respawns after a relevant cell reset and whether it needs an
  STRE-owned non-respawning replacement;
- exact neutral MQ102/MQ103 handoff stages and quest-running/completion state;
- Alduin marker/reference state and Riverwood/Whiterun dialogue dependencies;
- neutral Civil War initialization without implicit Imperial/Stormcloak
  allegiance;
- unlocking dragons and shouts only through intended vanilla progression;
- compatibility policy for other alternate starts and supported MQ101/QF
  conflicts;
- exact Departure destination and the route back to the main quest.

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
