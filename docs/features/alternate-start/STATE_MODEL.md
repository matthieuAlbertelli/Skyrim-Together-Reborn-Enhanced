# Alternate Start — State model

> **Status: Authoritative build, campaign admission and automatic Helgen entry implemented; full departure progression pending. Validation evidence lives in STATUS.**

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
- a durable campaign/checkpoint persistence substrate exists; the session
  Character Build component captures its canonical campaign/slot/player/binding
  identity as volatile provenance, but is not restored from that store.

## Individual post-build presentation

Individual Applied authorizes that player's local MarkerXX -> SeatXX approach and
final canonical appearance publication. Observer native rematerialization and
STR binding replay preserve the logical entity/ownership; posture is presentation,
not authoritative build completion or collective readiness. The durable sealed
PlayerId rank selects the existing marker/seat pair. This accepted roster-2
seating slice introduces no collective seating barrier or new campaign phase.
STATUS owns its human validation. The separate Helgen gate below does not change
seating eligibility.

## Automatic Helgen start after Applied

The server publishes each validated Applied immediately. It then derives a
separate collective predicate from canonical admission: sealed nonempty roster,
ACTIVE runtime, exact complete presence, live expected character/owner, matching
build admission identity, and Applied on every required component. Only the
first success sets the existing ephemeral Helgen start latch and broadcasts
`NotifyCampaignHelgenState`. Duplicate Applied acknowledgements validate revision
and both hashes, then reply without resetting level or invoking the barrier again.

The client latches the campaign-scoped event, waits for its own finalized creation,
and requests the local investigation entry once. Papyrus executes the shared
post-attack consequence before initializing investigation/T0. Duplicate entry
preserves MQ101, T0 and Hadvar/Ralof. Standalone uses local finalization without
server authorization. See CK_IMPLEMENTATION for the audited stage mapping and
why the stopped AlternateStart quest must not be restarted.

The readiness fallback is restricted to checkpoint sessions with no volatile
builds and readiness from the entire ACTIVE roster. Disconnect invalidates its
readiness set and local projection request; the server start latch remains
session-scoped. Native `.ess` state and the existing recovery protocol restore
local Papyrus runtime, never quest-stage reconstruction or partial-roster catch-up.
This increment adds no phase transition, Character Build persistence, checkpoint,
wire message or timestamp. Valen, Departure and MQ102/MQ103 remain separate work.

## Target campaign state

The server-side fixed-roster aggregate, readiness model, exact-roster runtime
eligibility, and atomic `Lobby -> CharacterCreation` seal are implemented in
`Code/campaign_runtime`. The STR transport now carries durable player identity,
live create/join/leave/resume/start/readiness commands, and canonical public
snapshots. The structure below remains the future Alternate Start gameplay
projection beyond the implemented bootstrap and automatic Helgen slice; the
full phase/progression model is not implied by those narrow integrations.

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
