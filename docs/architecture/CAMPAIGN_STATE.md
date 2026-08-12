# Campaign State Model

> **Status:** proposed specification.

## Goals

- one source of truth for the group;
- recovery after reconnection;
- separate Session Manager and Dragonborn roles;
- explicit transitions;
- schema evolution;
- filtering of secret information.

## Conceptual model

```cpp
struct CampaignState
{
    CampaignId Id;
    CampaignSchemaVersion SchemaVersion;
    StateVersion Version;
    CampaignPhase Phase;

    PlayerId SessionManager;
    std::optional<PlayerId> Dragonborn;
    bool DragonbornRevealed;

    std::vector<CampaignPlayerState> Players;
    AdapterStateMap AdapterStates;

    bool IntroductionStarted;
    bool IntroductionCompleted;
    bool DepartureAuthorized;
};
```

```cpp
struct CampaignPlayerState
{
    PlayerId Player;
    CharacterBinding Character;
    ConnectionState Connection;
    CampaignRole Role;
    std::optional<ClassId> SelectedClass;
    bool Ready;
    bool IntroductionCompleted;
};
```

## Phases

```text
Lobby
→ CharacterCreation
→ Arrival
→ Gathering
→ ValenIntroduction
→ ClassSelection
→ ReadyCheck
→ Departure
→ OpenWorld
```

A phase may have internal substates, but the public model must remain readable.

## Transitions

Every transition defines:

- allowed source phase;
- preconditions;
- command actor;
- authority policy;
- mutations;
- event;
- local consequence;
- recovery strategy.

Example, `AuthorizeDeparture`:

- source: `ReadyCheck`;
- preconditions: every required player is ready, classes are valid, and the
  introduction is complete;
- authority: server;
- mutation: `Phase=Departure`, `DepartureAuthorized=true`, `Version++`;
- event: `CampaignPhaseChanged`;
- local effect: activate the door.

## Snapshots and audience

The server produces at least two views:

- **public campaign snapshot**;
- **private snapshot** for restricted data, such as Dragonborn identity before
  its reveal.

An unauthorized client must not receive secret data merely hidden by the UI; the
data must not be sent.

## Character binding

A character is bound to the campaign through an identity created during
bootstrap. The model contains:

- campaign identifier;
- player identifier;
- character fingerprint;
- created/validated status;
- class;
- bootstrap version.

An external character cannot join without an explicitly designed import process.

## Reconnection

1. authenticate;
2. verify the binding;
3. negotiate adapters;
4. receive public and private snapshots;
5. apply idempotently;
6. resume the local phase;
7. consume only post-snapshot events starting at `Version+1`.

## Late join

Recommended MVP policy: allowed through `ReadyCheck`, rejected after `Departure`
unless a later catch-up mechanism is provided.

## Persistence

The campaign snapshot must be serializable from the MVP onward. Recommended
initial storage is one atomic server file per campaign, with integrated database
storage later if needed. A local Skyrim save is a consequence, not the sole
canonical source.
