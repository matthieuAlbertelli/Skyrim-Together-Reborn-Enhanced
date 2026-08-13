# Alternate Start — State model

> **Status: Authoritative build implemented; campaign state proposed**

## Currently implemented state

`CharacterBuildSnapshotData` represents a player's canonical build:

```cpp
struct CharacterBuildSnapshotData
{
    uint32_t BuildVersion;
    GameId RaceId;
    String ClassId;
    Vector<CharacterBuildSelectionData> Selections;
    Inventory CanonicalInventory;
    uint64_t InventoryHash;
    Vector<GameId> CanonicalSpells;
    uint64_t SpellHash;
};
```

Network state:

```cpp
enum class CharacterBuildNetworkState : uint8_t
{
    Accepted = 1,
    Applied = 2
};
```

Current invariants:

- `BuildVersion = 5`;
- the catalog validates classes and options;
- the server derives inventory and spells;
- local plugin/FormID resolution does not use a load-order prefix;
- a build becomes `Applied` only after both hashes are validated;
- once applied, the build cannot be replaced during the session;
- no durable persistence currently exists.

## Target campaign state

```cpp
struct AlternateStartState
{
    StateVersion Version;
    AlternateStartPhase Phase;
    CampaignRuntimeState RuntimeState;
    bool RosterSealed;
    std::optional<CheckpointId> LastCommittedCheckpoint;
    bool IntroductionStarted;
    bool IntroductionCompleted;
    bool DepartureAuthorized;
    std::vector<PlayerBootstrapState> Players;
};
```

```cpp
struct PlayerBootstrapState
{
    CampaignSlotId Slot;
    PlayerId Player;
    CharacterBindingState Binding;
    bool CharacterCreated;
    std::optional<ClassId> Class;
    bool Ready;
    bool LocalIntroductionComplete;
    ArrivalSlot Arrival;
};
```

Future invariants:

- roster slots may be configured only before the campaign seal;
- after the seal, every slot, `PlayerId`, and `CharacterBinding` is immutable in
  v1; campaign late join and player replacement are rejected;
- one arrival slot and one validated character per expected roster member;
- the complete sealed roster is required for campaign progression;
- no class changes after departure without an explicit migration;
- `DepartureAuthorized` requires a completed introduction and satisfied ready rules;
- monotonically increasing version;
- stale events are ignored;
- Dragonborn secrets are absent from public state;
- a required-member disconnect moves campaign runtime into recovery lock;
- multiplayer recovery selects one committed `CampaignCheckpoint` and restores
  every slot's matching native save plus the corresponding server revision;
- campaign progression resumes only after every expected member acknowledges the
  same restore.

These fields are the Alternate Start projection of the canonical
[Campaign State model](../../architecture/CAMPAIGN_STATE.md), not a separate
recovery architecture. Roster, checkpoint, authority, and collective-restore
semantics are governed by
[ADR-0018](../../architecture/ADRs/ADR-0018-fixed-roster-coordinated-checkpoint-recovery.md).
The standalone solo path remains outside the multiplayer full-roster invariant.
