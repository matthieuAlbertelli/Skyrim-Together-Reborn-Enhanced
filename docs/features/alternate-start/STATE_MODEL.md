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

## Individual post-build presentation

Applied authorizes only that player's local MarkerXX -> SeatXX approach and
final canonical appearance publication. Observer native rematerialization and
STR binding replay preserve the logical entity/ownership; posture is presentation,
not authoritative build completion or collective readiness. The durable sealed
PlayerId rank selects the existing marker/seat pair. This accepted roster-2
slice introduces no all-Applied barrier or new campaign phase. STATUS owns its
human validation; the future collective state below remains separate.

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

## Individual completion projection (ADR-0024)

NotifyCharacterBuildState::Applied remains server-authoritative after matching
revision and canonical inventory/spell hashes. Each Applied projects seating
independently; there is no aggregate all-Applied prerequisite. The existing
numeric PlayerId identifies the connection, not the durable roster identity.

The notification appends a seating identity tail after Build: version uint8=1,
campaign length uint8 + bytes, durable PlayerId length uint8 + bytes. Each string
is capped at 128 bytes; invalid/truncated/version-unknown tails leave both
identity fields empty and cannot request seating. Empty fields indicate that
no admitted campaign identity was supplied. Existing build/appearance payloads
and opcodes are unchanged. Matching updated clients/server are required for the
seating feature: old senders provide no usable identity; old receivers ignore
the new tail. This is not a general protocol negotiation guarantee.

Server identity comes from CampaignProtocolService admission, never from a
client-provided rank. Clients validate the current sealed campaign and resolve
the durable PlayerId rank locally. Up to ten session-scoped intentions retain
serverId, transport PlayerId, durable identity and revision. Duplicates do not
rearm; contradictory identities/revisions reject. Recovery lock suspends; fresh
creation/disconnect clears intentions. Reconnection needs fresh Applied evidence;
this slice does not claim checkpoint persistence/replay of seat intentions.

Applied can precede native availability: observers wait for the matching committed
final representation. Seat posture is a visual projection, not canonical authority.
No next collective phase is introduced.
