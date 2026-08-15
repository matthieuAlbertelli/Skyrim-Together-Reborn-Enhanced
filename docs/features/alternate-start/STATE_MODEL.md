# Alternate Start — State model

> **Status: Authoritative build, server campaign core, and live admission protocol implemented; Alternate Start gameplay projection pending**

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
- a durable campaign/checkpoint persistence substrate exists, but the current
  session Character Build service is not yet bound to a live campaign identity
  or restored from that store.

## Target campaign state

The server-side fixed-roster aggregate, readiness model, exact-roster runtime
eligibility, and atomic `Lobby -> CharacterCreation` seal are implemented in
`Code/campaign_runtime`. The STR transport now carries durable player identity,
live create/join/leave/resume/start/readiness commands, and canonical public
snapshots. The structure below remains the future Alternate Start gameplay
projection of that core; it is not yet wired to CEF, CK, or the live
`CharacterBuildService`.

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

- roster slots, `PlayerId` values, and `CharacterBinding` identities are
  configured in the pre-campaign lobby;
- the formal start/commit atomically seals them before the phase enters
  `CharacterCreation`; no later phase, including `Departure` or `OpenWorld`, is a
  seal point;
- after the seal, every slot, `PlayerId`, and `CharacterBinding` is immutable in
  v1 for the campaign lifetime; campaign late join and player replacement are
  rejected;
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
