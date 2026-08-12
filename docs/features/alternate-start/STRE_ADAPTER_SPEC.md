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
| `campaign.bootstrap/1` | server | campaign, roster, manager |
| `character.binding/1` | server | character authorized for each player |
| `campaign.phase/1` | server | phase and version |
| `group.ready-check/1` | server | ready state for each player |
| `narrative.introduction/1` | server | started/completed |
| `campaign.departure/1` | server | authorization |
| `narrative.dragonborn/1` | server secret | identity/reveal |

## Future intents

- `CreateCampaign`
- `JoinCampaign`
- `BindCharacter`
- `SetReady`
- `RequestIntroductionStart`
- `ReportLocalSceneCompleted`
- `RequestDeparture`

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
- restore the phase after reconnecting.

## Reconnection

Build reconnection is not implemented. A future snapshot must contain the phase, roster, binding, build, classes, ready states, and narrative flags, then apply them without replaying already-consumed events.

## Failure behavior

- `BuildVersion` mismatch: explicit rejection;
- missing local plugin or FormID: explicit rejection;
- incorrect inventory or spell hash: explicit rejection;
- offline single-player mode: local fallback;
- incompatible future campaign adapter: reject entry instead of silently producing hybrid state.
