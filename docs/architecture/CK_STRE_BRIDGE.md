# Creation Kit / Papyrus ↔ STRE Bridge

> **Status:** partial first-party integration implemented; generic API proposed.

## Current state

Alternate Start currently uses a dedicated integration:

```text
CK quest and TESQuestStageEvent
→ native CharacterCreationService
→ Character Creation UI
→ local application or CharacterBuildRequest
→ canonical result applied in Skyrim
```

Stage `20` of `STRE_QUEST_AlternateStart` triggers the native/Angular flow.
Objects, spells, and effects are authored in `STRE_AlternateStart.esp`. The
catalog and server remain the source of truth for multiplayer rewards.

This integration validates the bridge principles, but it does **not yet expose**
a generic Papyrus `STREBridge` API for third-party mods.

## Target flow rule

```text
Papyrus observes an interaction
→ sends an intent to the bridge
→ STRE validates and changes canonical state
→ STRE broadcasts an event
→ the bridge triggers a local Papyrus event
→ the script applies the visual or Skyrim consequence
```

## Illustrative minimum API

```papyrus
Bool Function IsAvailable() Global
String Function GetAdapterStatus(String adapterId) Global
Int Function GetCampaignPhase() Global
Bool Function SubmitIntent(String adapterId, String capability, String payloadJson) Global
Int Function GetCanonicalVersion(String adapterId) Global
String Function GetSnapshotJson(String adapterId) Global
```

Proposed events:

```papyrus
Event OnSTREAdapterReady(String adapterId, Int version)
Event OnSTRECanonicalEvent(String adapterId, String capability, Int version, String payloadJson)
Event OnSTREIntentRejected(String requestId, String errorCode, String details)
Event OnSTRESnapshotApplied(String adapterId, Int version)
```

JSON may be used for prototyping, but payloads must remain bounded and validated.

## Single-player mode

The Alternate Start plugin must continue to work without a server. The M7 flow
uses the same catalog locally; a future generic bridge must report unavailable
state without blocking Papyrus.

Target pattern:

```papyrus
If STREBridge.IsAvailable()
    STREBridge.SubmitIntent(...)
Else
    ApplySoloTransition()
EndIf
```

## Threading and cadence

- schedule Papyrus callbacks on a safe game context;
- do not poll every frame;
- use event-driven or bounded periodic observation;
- never block Papyrus on a network call.

## CK references

- prefix Editor IDs with `STRE_`;
- use properties and aliases for quest references;
- use a stable plugin and local FormID for the native catalog;
- never hard-code a load-order prefix;
- version PSC and PEX files while Papyrus compilation is not automated;
- treat local quest stages as triggers and projections, not canonical campaign
  state.
