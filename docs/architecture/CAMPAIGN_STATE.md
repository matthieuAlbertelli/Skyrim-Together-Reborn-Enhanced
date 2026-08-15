# Campaign State Model

> **Status:** fixed-roster/runtime core and live admission protocol implemented and automated-tested; gameplay projection and later phase/recovery wiring pending.

This document applies [ADR-0018](ADRs/ADR-0018-fixed-roster-coordinated-checkpoint-recovery.md).
The server is the persistent authority for shared STRE campaign state. A Session
Manager may administer the session and a client may hold temporary simulation
authority, but neither owns the persistent truth.

## Current implementation boundary

`Code/campaign_runtime` implements the pure aggregate, versioned core codec,
transition-policy boundary, exact full-roster predicate, and transactional
service over `ICampaignStore`. `GameServer` owns that service after the durable
store opens successfully. No second persistence layer is introduced: normalized
slots retain ownership while the versioned core payload stores phase, Session
Manager, and per-slot readiness. State-changing commands write current state,
the journal, and a canonical snapshot intent to the outbox atomically. Accepted
semantic no-ops reserve their `MutationId` in the same journal without advancing
`StateVersion` or emitting redundant outbox work; schema v2 is the minimal
transactional migration needed to represent this.

Transport connectivity and campaign admission are now wired as transient inputs
rather than durable socket state. They derive `WAITING_FOR_ROSTER` or `ACTIVE`
through one exact-roster predicate. The other runtime enum values are represented
for the accepted model but are not activated by this increment.

The implemented narrative mutation is the atomic
`Lobby -> CharacterCreation` campaign start/seal. It is server-authoritative at
the pure-domain boundary. A future session/network caller must first prove that
the requesting connection holds the host/Session Manager administrative role,
then issue the server-authorized command with an explicit target Session Manager
that belongs to the proposed roster. That transient proof is not persisted in
the aggregate, and roster membership alone never authorizes the seal. Policies
for the remaining phase edges define their source, target, actor authority,
common roster/readiness preconditions, and resulting intent, but deliberately
refuse execution until their feature-owned CK/Valen/build preconditions are
implemented. The live protocol now exposes the implemented start transition,
but no CEF/CK projection, coordinated native save, or recovery-lock behavior is
part of this increment.

## Live identity and admission boundary

The existing STR transport authentication request now also carries one durable,
opaque STRE `PlayerId`. Password, version, mod, and native-plugin checks remain
access control; the `PlayerId` is metadata registered only after those checks and
never authenticates a connection by itself. It is stored locally across game
restart and remains distinct from username, Discord/Steam identity,
`ConnectionId`, and transient STR `Player::GetId()`.

The server's `CampaignProtocolService` adapts `World`, `Player`, and
`PartyService` facts into a transport-independent `CampaignAdmissionService`.
The latter stores only live connection-to-identity/admission records and invokes
`CampaignRuntimeService` for every durable change. Party leadership and party
membership are evaluated live; neither party IDs nor transient player IDs are
written into campaign persistence.

The live protocol implements:

- leader-only campaign creation with server-generated campaign, slot, and
  character-binding identities; its journaled initial-roster command is also
  the durable idempotency receipt for an exact retry after server restart, but
  transient admission is restored only when that historical tuple still exists
  unchanged in the current canonical roster;
- pre-seal join/leave in the current party/session;
- pre-seal or sealed resume admission by canonical `PlayerId` plus cached
  binding, with the slot resolved only from server state and no roster/version
  mutation; a fresh join mutation for an existing member is rejected with an
  explicit resume-required result;
- leader-only campaign start/seal, deriving the durable Session Manager from
  the admitted requester's STRE `PlayerId`;
- admitted-member readiness whose actor tuple is derived server-side;
- public snapshots containing ordered slots, readiness, and transient presence,
  without other members' character bindings or secret narrative state.

Disconnect removes transient admission only. A sealed incomplete roster projects
`WAITING_FOR_ROSTER`; the exact admitted roster projects `ACTIVE`. This increment
does not turn disconnect into `RECOVERY_LOCK`; that remains #56. The local binding
cache is reconnect metadata only and never overrides the SQLite authority.

## Goals

- one authoritative and versioned source of shared campaign truth;
- fixed player/character identity for the lifetime of a v1 campaign;
- recoverable coordination between server state and each player's Skyrim runtime;
- explicit narrative-phase and runtime-recovery transitions;
- schema evolution and filtering of secret information;
- no partial-roster campaign divergence.

## Conceptual model

```cpp
struct CampaignState
{
    CampaignId Id;
    CampaignSchemaVersion SchemaVersion;
    StateVersion Version;
    CampaignPhase Phase;
    CampaignRuntimeState RuntimeState;

    PlayerId SessionManager;
    std::optional<PlayerId> Dragonborn;
    bool DragonbornRevealed;

    std::vector<CampaignPlayerState> Players;
    bool RosterSealed;
    std::optional<CheckpointId> LastCommittedCheckpoint;
    AdapterStateMap AdapterStates;

    bool IntroductionStarted;
    bool IntroductionCompleted;
    bool DepartureAuthorized;
};
```

```cpp
struct CampaignPlayerState
{
    CampaignSlotId Slot;
    PlayerId Player;
    CharacterBinding Character;
    ConnectionState Connection;
    CampaignRole Role;
    std::optional<ClassId> SelectedClass;
    bool Ready;
    bool IntroductionCompleted;
};
```

## Campaign phase

`CampaignPhase` describes durable narrative/product progression:

```text
Lobby (pre-campaign; roster mutable)
→ formal campaign start/commit; roster sealed
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

The `Lobby` → `CharacterCreation` transition is the formal campaign start. It
atomically sets `RosterSealed=true` before entering `CharacterCreation`. No
campaign progression phase may be entered while the roster is unsealed.
`Departure` and `OpenWorld` occur much later and have no roster-sealing effect.

Every phase transition defines its allowed source, preconditions, command actor,
authority policy, mutations, event, local consequence, and recovery strategy.
For example, `AuthorizeDeparture` requires a complete connected roster, valid
classes/builds, completed introduction, and satisfied ready rules. The server
then advances the canonical phase and emits the versioned projection intent.

## Campaign runtime state

`CampaignRuntimeState` is orthogonal to narrative phase:

```text
WAITING_FOR_ROSTER
  → ACTIVE
  → CHECKPOINTING
  → ACTIVE

ACTIVE or CHECKPOINTING
  → RECOVERY_LOCK
  → RESTORING_CHECKPOINT
  → ACTIVE
```

- `WAITING_FOR_ROSTER`: the complete sealed roster is not connected and
  validated; campaign progression is forbidden.
- `ACTIVE`: the full-roster predicate holds and normal phase mutations may be
  considered by the server.
- `CHECKPOINTING`: a coordinated checkpoint candidate is in flight; its exact
  mutation fence is an implementation decision, but commit is all-or-nothing.
- `RECOVERY_LOCK`: a required member is absent or recovery failed; persistent
  campaign mutations and new checkpoint commits are rejected.
- `RESTORING_CHECKPOINT`: the complete roster is restoring one server-selected
  committed checkpoint; normal progression remains rejected.

Runtime transitions never change roster ownership. Session Manager transfer also
does not change a slot owner or grant persistence authority.

## Fixed roster and identity

While the campaign remains in the pre-campaign `Lobby`, roster configuration may
add, remove, or replace proposed slots and bindings. The formal start/commit
validates the intended roster, atomically sets `RosterSealed=true`, and only then
transitions to `CharacterCreation`. After that transition, the ordered slots and
each slot's `PlayerId` and `CharacterBinding` are immutable for the lifetime of
the STRE v1 campaign.

The full-roster predicate holds only when every expected slot is connected with
its matching identity/binding and no unrecognized participant is admitted as a
campaign member. The server rejects:

- activation or progression with a missing roster member;
- an extra player attempting to join the campaign;
- replacement of a slot owner;
- a character bound to another slot or campaign;
- a stale or otherwise invalid binding.

There is no campaign late join or continue-without-player path after the seal.
WorldEntity late materialization from a snapshot is a separate synchronization
mechanism and remains valid.

## Character binding

A stable `CharacterBinding` identity is allocated or reserved for each slot in
the pre-campaign lobby before the roster is sealed. Character creation later
populates and validates character/build state against that identity; it cannot
replace the sealed binding. The binding uses campaign ID, slot ID, player ID,
character fingerprint, created/validated status, class, and bootstrap version.
An external character cannot enter without a separately designed import process;
no such process is part of v1.

## Campaign checkpoints

```cpp
enum class CampaignCheckpointState
{
    Candidate,
    Committed
};

struct CampaignCheckpointSlot
{
    CampaignSlotId Slot;
    PlayerId Player;
    CharacterBinding Character;
    NativeSaveIdentity Save;
    NativeSaveFingerprint Fingerprint;
    NativeSaveMetadata Metadata;
};

struct CampaignCheckpoint
{
    CampaignId Campaign;
    CheckpointId Id;
    StateVersion ServerRevision;
    ServerSnapshotIdentity Snapshot;
    std::vector<CampaignCheckpointSlot> ExpectedRoster;
    CampaignCheckpointState State;
};
```

The server snapshot is the authority for shared STRE state. Each slot's native
`.ess` restores that player's local Skyrim, Papyrus, quest, alias, scene, and
Creation Engine runtime. Native saves are required recovery components but do not
author shared state. The server records their identity/fingerprint/metadata; v1
does not upload `.ess` files.

### Commit rules

1. The server allocates a unique checkpoint candidate tied to one exact revision
   and complete expected roster.
2. Every slot produces a dedicated, identifiable STRE checkpoint save and
   acknowledges its matching metadata.
3. The server snapshot/revision and every required per-slot acknowledgement must
   refer to the same candidate.
4. Only then does the server atomically mark the candidate `Committed` and update
   `LastCommittedCheckpoint`.
5. A failed or incomplete candidate never replaces the previous committed
   checkpoint. Duplicate requests and acknowledgements are idempotent.

Checkpoint cadence, triggers, safe points, autosave/manual-save interaction,
combat/dialogue/cell-transition behavior, and exact fingerprint are deliberately
not fixed by this architecture.

## Snapshots and audience

The server produces at least a public campaign snapshot and a private snapshot
for restricted data such as Dragonborn identity before reveal. Unauthorized data
is not sent merely because a UI could hide it.

Snapshots and versioned events support idempotent hydration, retransmission,
restart, and convergence against the selected checkpoint. Events do not permit an
absent roster member to catch up after the campaign has progressed without them,
because such progression is forbidden.

## Disconnect, crash, and restore

On a required-member disconnect during `ACTIVE` or `CHECKPOINTING`, the server
enters `RECOVERY_LOCK`, fences persistent campaign mutations, and refuses new
checkpoint commits. A partially acknowledged candidate remains uncommitted.

Recovery proceeds as follows:

1. authenticate the returning client;
2. verify its expected `PlayerId`, slot, and `CharacterBinding`;
3. wait until the full-roster predicate holds;
4. enter `RESTORING_CHECKPOINT`;
5. select the exact `LastCommittedCheckpoint`;
6. restore the matching server snapshot/revision;
7. instruct every slot to load its own save recorded for that checkpoint;
8. verify every slot's checkpoint, binding, and save acknowledgement;
9. enter `ACTIVE` only after the server and all clients agree.

All players roll back collectively, including those that remained connected. A
wrong or unavailable save, wrong binding, incomplete roster, failed load, stale
acknowledgement, or incompatible snapshot keeps the campaign out of `ACTIVE` and
produces an actionable rejection. Retry is idempotent. If a member never returns,
the campaign remains suspended or the session ends.

A client or server restart follows the same committed-checkpoint selection rule;
no participant chooses the latest local save independently. The exact engine-safe
local freeze/pause presentation is delegated to implementation audit.

## Persistence

Campaign state, checkpoint metadata, roster/bindings, snapshots, revisions, and
commit state use the persistence port and proposed SQLite adapter described by
[ADR-0019](ADRs/ADR-0019-sqlite-durable-server-persistence.md). The implemented
substrate provides a versioned multi-campaign schema, explicit transactional
migration, normalized current state, immutable snapshots, optimistic revisions,
an append-only validated-mutation journal, and a transactional outbox. Its
default locked server path is `state/stre-server.sqlite3`. Schema v2 keeps that
model and permits accepted no-op journal records to share the unchanged
canonical revision; schema-v1 stores migrate transactionally without a second
receipt system.

Checkpoint rollback never rewinds the durable mutation sequence. A checkpoint
permanently retains its exact `SourceRevision`. Restoring it is a new validated
`RestoreCheckpoint` mutation at a revision greater than every mutation already
recorded for that campaign. The transaction materializes the exact snapshot into
current state, records the source checkpoint/revision and new restore revision in
the journal, supersedes obsolete pending outbox work, and emits a canonical full
snapshot at the new revision. Clients then consume only events newer than that
restore revision, preserving [ADR-0004](ADRs/ADR-0004-snapshot-plus-events.md).

The server-owned #28 runtime and live protocol now call this persistence
substrate for Lobby roster configuration, the campaign start/seal, Session
Manager transfer, readiness, journal entries, and outbox snapshot intents.
Durable Character Build binding and client/CK/UI gameplay projection remain
pending #28 integration. Issue #55 owns coordinated native-save creation,
identity/fingerprinting, acknowledgement, and `CampaignCheckpoint`
coordination. Issue #56 owns disconnect recovery lock and collective checkpoint
restore/reload gameplay. Durable WorldEntity persistence remains separate
future work rather than part of #55.

The standalone solo Alternate Start path remains independent of this multiplayer
full-roster rule and continues to restore from its local Skyrim save.
