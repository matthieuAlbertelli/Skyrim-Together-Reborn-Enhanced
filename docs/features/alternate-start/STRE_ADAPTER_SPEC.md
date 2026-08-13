# Alternate Start — STRE integration specification

> **Status: First-party Character Build implemented; generic campaign adapter proposed**

## Identity

- required plugin: `STRE_AlternateStart.esp`;
- current integration: compiled first-party C++ services;
- catalog: `BuildVersion = 5`;
- target adapter ID: `stre.alternate-start`;
- target adapter version: `1`.

## Current capabilities

| Capability | Authority | Canonical state |
|---|---|---|
| `player.character-build/5` | server or local fallback | race, class, selections, inventory, spells, hashes |
| `player.character-build-state/1` | server | revision, Accepted/Applied |
| `player.targeted-buff/1` | engine/STRE MagicService | recognized ally buffs |

Current messages:

- `CharacterBuildRequest`;
- `CharacterBuildResponse`;
- `CharacterBuildAppliedRequest`;
- `NotifyCharacterBuildState`.

## Current semantics

The client sends only logical identifiers. The server constructs the canonical build. The client applies it, then confirms with hashes. The offline path reuses the same catalog without a server.

## Target campaign capabilities

| Capability | Authority | Canonical state |
|---|---|---|
| `campaign.bootstrap/1` | server | campaign, roster, roster seal, manager |
| `character.binding/1` | server | character authorized for each player |
| `campaign.phase/1` | server | phase and version |
| `campaign.runtime-state/1` | server | full-roster eligibility, checkpointing, recovery lock, restore |
| `campaign.checkpoint/1` | server | candidate/committed checkpoint, revision, expected slot/save metadata |
| `group.ready-check/1` | server | ready state for each player |
| `narrative.introduction/1` | server | started/completed |
| `campaign.departure/1` | server | authorization |
| `narrative.dragonborn/1` | server secret | identity/reveal |

## Future intents

- `CreateCampaign`
- `JoinCampaign` (before roster seal only)
- `BindCharacter`
- `CommitCampaignStart` (atomically seal the roster and enter `CharacterCreation`)
- `SetReady`
- `RequestIntroductionStart`
- `ReportLocalSceneCompleted`
- `RequestDeparture`
- `RequestCheckpoint`
- `AcknowledgeCheckpointSave`
- `RestoreCheckpoint`
- `AcknowledgeCheckpointRestore`

Class and build selection is already covered by the M7-specific protocol; migration to a generic envelope is not required before the generic runtime is stable.

## Local application

Currently:

- clean the character;
- add and equip items;
- add spells;
- verify hashes;
- display UI state.

Future:

- teleport to the assigned marker;
- start and stop the local scene;
- enable the door;
- present safe recovery state when the runtime is locked;
- save and load the dedicated native Skyrim save selected for a checkpoint;
- restore the phase only as part of collective checkpoint recovery.

## Roster and reconnection

Campaign creation/join and character binding are accepted only before the roster
seal in the pre-campaign lobby. `CommitCampaignStart` atomically fixes every
slot, `PlayerId`, and `CharacterBinding`, then enters `CharacterCreation`. No
campaign progression may occur before that commit. `Departure` and `OpenWorld`
do not seal the roster. After seal, the server rejects extra players, slot
replacement, and any identity or binding mismatch. The complete expected roster
is required before campaign runtime can be `ACTIVE`.

The durable campaign/checkpoint persistence substrate is implemented and
automated-tested, but this live adapter is not yet wired to campaign identity,
checkpoint coordination, or restore. Snapshots remain useful for idempotent
hydration and retransmission, but reconnect does not
catch one player up while the campaign continues. A required disconnect locks the
campaign. Once the exact roster returns, the server selects the last committed
`CampaignCheckpoint`; every client loads its slot's matching native save, the
server restores the matching revision, and all participants acknowledge before
the runtime returns to `ACTIVE`.

This specification does not allocate protocol opcodes. See
[ADR-0018](../../architecture/ADRs/ADR-0018-fixed-roster-coordinated-checkpoint-recovery.md)
and [Campaign State](../../architecture/CAMPAIGN_STATE.md).

## Failure behavior

- `BuildVersion` mismatch: explicit rejection;
- missing local plugin or FormID: explicit rejection;
- incorrect inventory or spell hash: explicit rejection;
- offline single-player mode: local fallback;
- incompatible future campaign adapter: reject entry instead of silently producing hybrid state.
- post-seal join, extra player, replacement player, or wrong binding: reject without
  mutating the roster;
- missing full roster: keep campaign progression inactive;
- wrong, stale, or unavailable checkpoint save: remain in recovery and report an
  actionable failure.
