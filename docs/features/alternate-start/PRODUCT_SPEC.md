# Alternate Start — Product specification

> **Status: Product target accepted; New Game campaign gate, character bootstrap, and MQ101/post-Helgen projection implemented; the campaign happy path is runtime-validated while negative coverage and the full departure journey remain**

## Pitch

Start a campaign in an inn without importing an already-advanced character. Each player creates their appearance, chooses a class and kits, then receives a clean build consistent with the campaign.

## Implemented today

1. Teleport to an inn seat through the CK quest.
2. Open RaceMenu.
3. Select Warrior, Mage, or Thief in the Angular UI.
4. Select available kits.
5. Use the 3D preview and summary screen.
6. Submit the final “Seal your destiny” choice.
7. Apply anti-import cleanup, level 1, canonical inventory, and canonical spells.
8. Use the local offline path or server validation in multiplayer.
9. Apply functional targeted Alteration buffs to a remote companion.
10. Intercept a fresh New Game before vanilla MQ101 stage 10.
11. Project MQ101 and Helgen to the validated post-attack boundary while leaving
    MQ102/MQ102A/MQ102B untouched for the later handoff.
12. Before RaceMenu, choose Solo or create/join a campaign through the focused
    CEF lobby; multiplayer Character Creation opens only after the server seals
    the current roster and reports the complete roster `ACTIVE`.

## Complete product goals

- skip Helgen cleanly;
- prevent unvalidated external characters from entering;
- create characters at the same narrative point;
- work in single-player without an STRE server;
- provide a readable hub for a small group;
- serve as the first reference first-party integration;
- restore an interrupted multiplayer campaign collectively from one committed
  checkpoint;
- leave the inn with coherent vanilla progression.

## v1 class roster

STRE v1.0.0 targets the 21 classes defined in [`CLASS_ROSTER_V1.md`](CLASS_ROSTER_V1.md). That canonical document defines each class's stable identity, canonical English name, French localized display name, and two major/four minor skills.

Warrior, Mage, and Thief form the first implemented vertical slice. The kits, items, quantities, spells, cooperative abilities/perks, and personal quests for all 21 classes remain governed by their dedicated design and implementation sources; they are not inferred from old documents under `history/`.

## Target experience

1. Redirect a new game to the inn.
2. In multiplayer, configure the pre-campaign lobby: create/join the campaign,
   assign slots, and reserve each `PlayerId`/`CharacterBinding`.
3. Formally start/commit the multiplayer campaign, atomically sealing the roster
   before entering `CharacterCreation`.
4. Create appearance at the table.
5. Choose class and kits.
6. Validate every build.
7. Introduce Valen.
8. Complete the ready check.
9. Depart for Skyrim together without changing the sealed roster.

The appearance/build flow, automatic New Game redirect, MQ101/post-Helgen
projection, and gameplay-facing Solo/Create/Join lobby are present. Four-character
codes are ephemeral aliases for server-owned campaigns, and the current mutable
roster is sealed only by the existing authoritative start path. This native/CEF
slice asks every creator/joiner for a transient lobby pseudo, including when the
transport is already connected. It is shown to lobby members only and is not a
Skyrim character name or durable campaign identity. The slice is automated-tested
and its Solo/two-player Create/Join happy path is runtime-validated. Negative
runtime coverage remains pending. Valen, integrated
ready/departure behavior, durable Character Build binding, and the neutral
MQ102/MQ103 handoff remain to be completed.

## Helgen investigation v1 boundary

The v1 occupation rule is intentionally deterministic and off-screen:

- no occupation bandits are present before four full Skyrim days have elapsed
  from the investigation start;
- if the deadline arrives while at least one campaign player is still inside the
  affected Helgen footprint, the post-deadline physical transition is deferred;
- while deferred, Helgen and the survivor projection remain unchanged;
- once the affected footprint is empty, Helgen may project directly to the
  bandit-occupied state;
- each survivor still left behind in `WoundedInCave` becomes
  `CapturedInKeep` independently, while an already `Freed` or `Departed`
  survivor remains unchanged.

A staged real-time bandit invasion when players are present is a post-v1
enhancement candidate, not part of the v1 product target.

## Single-player

- the same creation flow in the inn;
- the same local catalog;
- a mandatory Solo/Create/Join choice before RaceMenu on a fresh New Game;
- no mandatory roster or ready check;
- no mandatory server dependency;
- state retained in the Skyrim save.

## Current STRE behavior

- server validation of race, class, and selections;
- canonical inventory and spells;
- hashes and application acknowledgment;
- Pending/Applied state broadcast;
- build state is still session-owned by `CharacterBuildService` and is not yet
  bound to the admitted durable campaign slot/character identity.

## Target STRE behavior

- canonical campaign;
- roster and bound characters configured in the pre-campaign lobby, then sealed
  by the formal start/commit before `CharacterCreation` and immutable thereafter;
- full roster required for multiplayer campaign progression;
- shared phases;
- secretly assigned Dragonborn;
- server-authorized departure;
- coordinated `CampaignCheckpoint` creation using one server revision and one
  dedicated native Skyrim save per roster slot;
- campaign suspension on disconnect and collective restore of every roster member
  from the same committed checkpoint;
- rejection of campaign late join, player replacement, and wrong bindings after
  the roster seal.

Departure and `OpenWorld` never seal or mutate roster ownership.

The STRE server is authority for shared campaign state. Each native `.ess`
restores its player's local Skyrim/Papyrus/quest runtime and is not shared-state
authority. See [ADR-0018](../../architecture/ADRs/ADR-0018-fixed-roster-coordinated-checkpoint-recovery.md)
and the [Campaign State model](../../architecture/CAMPAIGN_STATE.md).

## Out of immediate scope

- dynamic Dragonborn assignment based on accomplishments;
- post-seal campaign late join or player replacement in v1;
- partial-roster campaign progression and catch-up;
- intentional import of an existing character;
- complete rewrite of the vanilla campaign;
- stable third-party SDK before several first-party integrations exist.
