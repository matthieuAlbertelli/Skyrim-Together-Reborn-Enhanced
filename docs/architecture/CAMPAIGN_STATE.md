# Campaign State Model

> **Status:** proposed specification; not implemented.

This document applies [ADR-0018](ADRs/ADR-0018-fixed-roster-coordinated-checkpoint-recovery.md).
The server is the persistent authority for shared STRE campaign state. A Session
Manager may administer the session and a client may hold temporary simulation
authority, but neither owns the persistent truth.

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
commit state require atomic, versioned, recoverable server persistence. Storage
schema, migration mechanics, and durable WorldEntity coverage remain open design
work. [ADR-0016](ADRs/ADR-0016-state-journal-outbox.md) continues to govern
current state, journal, and transactional outbox semantics.

The standalone solo Alternate Start path remains independent of this multiplayer
full-roster rule and continues to restore from its local Skyrim save.
