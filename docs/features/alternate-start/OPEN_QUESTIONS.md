# Alternate Start — Open questions

> **Status: Remaining decisions after New Game, MQ101/post-Helgen continuity, wounded-survivor, and standalone T+4 occupation validation**

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
  and is excluded from generic quest-stage synchronization;
- the v1 Helgen deadline rule is fixed: before four full Skyrim days from
  investigation start there are no occupation bandits; once the deadline is
  reached, the post-deadline physical projection is deferred for as long as any
  campaign player remains in the affected Helgen footprint;
- when that footprint becomes empty, the deferred transition may commit directly
  to bandit-occupied Helgen, and each survivor still in `WoundedInCave` becomes
  `CapturedInKeep` independently; `Freed` and `Departed` never regress.
- the standalone fallback now evaluates the relative four-day deadline from
  `InvestigationStartGameTime`, uses `HelgenLocation [00018A4A]` as its local
  presence predicate, and defers through `BanditOccupationPending` until the
  player leaves;
- connected clients use an ephemeral exact-roster start barrier and a
  server-evaluated outside-Helgen cache while retaining the T+4 timer and CK
  projection locally; no Helgen-specific persistent server state is required by
  the v1 checkpoint model;
- the post-deadline physical projection reuses vanilla
  `PostHelgenEncountersMarker [000F8240]`, retires
  `dunCGPostMajorFXMarker [000F829B]` plus the temporary STRE squeeze activators,
  preserves the collapsed bridge/debris projection, and does not create
  duplicate STRE occupation bandits;
- captured-survivor placement is implemented with independent STRE jail markers
  and conditional Sandbox packages for Hadvar and Ralof; the selected vanilla
  jail doors are referenced through aliases and are closed/locked during the
  `CapturedInKeep` projection.

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

- two-client runtime validation of the implemented ephemeral start barrier and
  exact-roster outside-Helgen predicate, including both exit orders,
  already-outside at T+4, and `HelgenKeep01`. It remains pending, with the
  merged #71 gameplay bootstrap now providing the supported two-PC
  Create/Join/Start precondition. No Helgen runtime result is recorded yet;
- coordinated checkpoint/recovery validation of the local native Helgen state
  once the general campaign recovery path exists;
- release behavior for each captured survivor and the rescue interaction from
  `WoundedInCave` or `CapturedInKeep` to `Freed`;
- physical `Freed` and `Departed` projections and their idempotent recovery;
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

## Post-v1 candidate: live Helgen occupation encounter

The v1 path intentionally uses an off-screen/deferred transition rather than a
scripted invasion. A possible post-v1 enhancement is to play the occupation in
real time when players are present at the four-day boundary:

- bandits approach and invest the exterior ruins;
- occupation progresses toward the Keep instead of appearing fully established;
- bandits enter the dungeon and advance through it;
- a survivor would only be captured when the encounter reaches that survivor,
  while an off-screen path would continue to fast-forward to the same canonical
  result when no player is present.

This idea is deliberately parked for post-v1. It must not add scope to the v1
deadline/occupation implementation and would require a separate design for AI,
navmesh, encounter progression, authority, recovery, and multiplayer projection.

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
