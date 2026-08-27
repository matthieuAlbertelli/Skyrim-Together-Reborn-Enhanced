# Campaign State Model

> **Status:** fixed-roster/runtime core, live admission protocol, focused New
> Game lobby projection, and coordinated checkpoint implementation are
> automated-tested. The lobby happy path is manually validated on Solo/two PCs;
> the nominal coordinated checkpoint path is manually validated on two PCs,
> while its remaining resilience matrix is tracked by
> [#72](https://github.com/matthieuAlbertelli/Skyrim-Together-Reborn-Enhanced/issues/72).

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
through one exact-roster predicate. Issue #55 also activates `CHECKPOINTING`
while one server-owned Candidate is in flight. Issue #56 activates
`RECOVERY_LOCK` on sealed-roster loss and `RESTORING_CHECKPOINT` while the exact
committed checkpoint passes its two collective restore barriers.

The implemented narrative mutation is the atomic
`Lobby -> CharacterCreation` campaign start/seal. It is server-authoritative at
the pure-domain boundary. The live protocol proves that the requesting
connection holds the current PartyService leader role and that the complete
candidate roster shares that exact transient party before issuing the
server-authorized command with an explicit target Session Manager that belongs
to the proposed roster. That transient proof is not persisted in the aggregate,
and roster membership alone never authorizes the seal. Policies
for the remaining phase edges define their source, target, actor authority,
common roster/readiness preconditions, and resulting intent, but deliberately
refuse execution until their feature-owned CK/Valen/build preconditions are
implemented. A native/CEF bootstrap now gates fresh New Game at the existing
stage-20 boundary and applies Character Creation locally only after canonical
sealed/full-roster evidence. No new CK stage or record was required. Coordinated
native save is implemented separately by the #55 flow described below;
recovery-lock behavior is not.

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

The gameplay lobby automates that transient PartyService ceremony. Campaign
creation ensures a campaign-managed party, while join-by-code aligns the joining
connection with the resolved lobby party before invoking the existing canonical
admission path. Alignment introduced by a failed admission is rolled back, and
legacy automatic party joining skips campaign-managed parties. This preserves
the accepted transient authority proof without exposing party mechanics to the
player.

Exactly four-character join codes are server-owned ephemeral aliases from
`ABCDEFGHJKLMNPQRSTUVWXYZ23456789`; they map to canonical `CampaignId` values,
are collision-safe and bounded, and expire when the lobby seals. They are not
persisted identity or authentication. Each creator/joiner supplies a lobby-only
display name that is trimmed, valid UTF-8, free of control characters, and
bounded to 24 Unicode code points and 96 UTF-8 bytes. The server lobby directory
keys it internally by `PlayerId`, but Angular receives only the name/presence
projection. It is neither the Skyrim character name nor identity,
authorization, ownership, binding, save, or checkpoint data, and the complete
name directory is invalidated when the campaign leaves `Lobby`/seals.

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

Disconnect removes transient admission only; it never changes the durable
roster. An incomplete roster projects `WAITING_FOR_ROSTER`; disconnecting again
from that state never creates `BeginRecovery`. A required-member disconnect
opens #56 `RECOVERY_LOCK` only from `ACTIVE` or `CHECKPOINTING`, while a
disconnect during an already-open recovery replays its current barrier. Exact
resume restores the same canonical slot before collective recovery advances.
The local binding cache is reconnect metadata only and never overrides the
SQLite authority.

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
  server snapshot is fixed and the runtime rejects unrelated durable mutations
  for that campaign. Only canonical per-slot acknowledgements and the final
  all-slots commit may advance it. Other campaigns remain independent.
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

The implemented native artifact is the path-independent codec-v1
`.ess`/`.skse` manifest with per-member SHA-256 hashes and a SHA-256 fingerprint
over the exact metadata. The client persists that completed artifact before
acknowledgement. An exact retransmission reopens and re-hashes both existing
files against the cached artifact; it never calls Skyrim Save again or
overwrites the bundle. A mismatch fails closed.

The server keeps `stre_checkpoint <CampaignId>` and
`stre_checkpoint_resend <CampaignId>` as development/validation triggers. The
production client intercepts Skyrim `Save*` and `QuickSave*` native-save
families during an admitted `ACTIVE` campaign and sends only a Manual/Quick
intent. Server admission derives the campaign and exact member, validates the
full sealed roster, allocates the checkpoint identity/revision, and dispatches
the existing all-slot barrier. Concurrent requests during `CHECKPOINTING`
coalesce with that same Candidate. Autosaves, unknown native-save families, and
Manual/Quick attempts while waiting, recovering, restoring, resume-required,
or otherwise fenced are blocked fail-closed. Outside campaigns, Skyrim save
behavior remains vanilla.

The internally dispatched `stre-<CheckpointId>` save crosses the same native
hook using scoped internal provenance; a filename is never authorization. The
runtime hook remains authoritative independently of UI. STR Settings only
projects the four Skyrim autosave preference families (rest, wait, travel, and
character menu) as localized informational disabled controls while in campaign
and never changes the player's stored preferences. The real `Journal Menu` /
`quest_journal.swf` Gameplay rows remain vanilla: the audited movie has no
per-row disabled/help contract, and STRE has no typed GFx seam for its private
ActionScript controls. A safe implementation would require owning a replacement
SWF or similarly fragile movie/native hooks, so this remains an explicit UX
limitation while the native save-policy continues to fail closed. Scheduler
cadence and combat/dialogue/cell-transition safe-point policy remain
deliberately undecided.

On client failure during checkpoint creation, transient `CHECKPOINTING` is
abandoned, the durable row remains `Candidate`, and
`LastCommittedCheckpoint` is unchanged. With issue #56 integrated, a required
member disconnect additionally opens collective `RECOVERY_LOCK` after
abandoning that Candidate. Old and failed native saves and Candidate rows are
not deleted. A server restart does not resume an unfinished Candidate.

## Snapshots and audience

The server produces at least a public campaign snapshot and a private snapshot
for restricted data such as Dragonborn identity before reveal. Unauthorized data
is not sent merely because a UI could hide it.

Snapshots and versioned events support idempotent hydration, retransmission,
restart, and convergence against the selected checkpoint. Events do not permit an
absent roster member to catch up after the campaign has progressed without them,
because such progression is forbidden.

## Disconnect, crash, and restore

The recovery sequence below is implemented for issue #56, automated/build
tested, and live validated for nominal N=1/N=2, successive recovery, and durable
incomplete-attempt rehydration without a second restore revision.

On a required-member disconnect during `ACTIVE` or `CHECKPOINTING`, the server
enters `RECOVERY_LOCK`, fences persistent campaign mutations, and refuses new
checkpoint commits. A partially acknowledged candidate remains uncommitted.

Recovery proceeds as follows:

1. authenticate the returning client;
2. verify its expected `PlayerId`, slot, and `CharacterBinding`;
3. wait until the full-roster predicate holds;
4. select the exact `LastCommittedCheckpoint` and enter
   `RESTORING_CHECKPOINT`;
5. instruct every slot to load its own save recorded for that checkpoint;
6. verify the first full-roster barrier of exact checkpoint, binding, artifact,
   and successful native-load acknowledgements;
7. restore the matching server snapshot as one new monotonic revision;
8. publish that exact correlated snapshot and verify every slot's
   `SnapshotApplied` acknowledgement;
9. durably complete the attempt and enter `ACTIVE` only after the server and all
   clients agree.

All players roll back collectively, including those that remained connected. A
wrong or unavailable save, wrong binding, incomplete roster, failed load, stale
acknowledgement, or incompatible snapshot keeps the campaign out of `ACTIVE` and
produces an actionable rejection. Retry is idempotent. If a member never returns,
the campaign remains suspended or the session ends.

A client or server restart follows the same committed-checkpoint selection rule;
no participant chooses the latest local save independently. The durable journal
reconstructs the exact attempt, checkpoint, restore revision, and stable
per-slot `Loaded`/`SnapshotApplied` receipts. Those receipts are accepted no-op
journal mutations: they never advance canonical state, but make partial barrier
evidence and duplicate replay durable without another schema or protocol.

After server restart, even an attempt with a durable `RestoreCheckpoint` first
replays its exact native-load request. A fresh client process thereby acquires
the authoritative correlation and re-proves its local checkpoint; a survivor
responds idempotently. The current admitted roster must cross a new volatile
full-roster barrier before the server reuses the existing restore revision and
replays the snapshot. The second volatile barrier follows the same rule, while
the durable Applied receipts retain exact idempotency evidence. A client
disconnect during snapshot application uses this same replay path. If completion
was durable before a server crash, replaying an exact `SnapshotApplied` ACK
resends the completion message. The client runtime gate freezes Skyrim before
load and releases only on that exact message. Separately, a transport-only
provisional lock created by a hard server crash while `ACTIVE` may be released
by the same campaign's authoritative `ACTIVE` snapshot only while no recovery
attempt is correlated.

Cold-session **Load Campaign** uses the same rule and recovery machine. A
#55-managed native checkpoint carries only a versioned local sidecar hint for
its exact campaign/binding/native identity. Loading that marked save arms a
distinct resume-required client lock before Skyrim's native load. The client
looks up only the marker's campaign binding and requires exact campaign, slot
hint, and character-binding equality, yielding zero or one opaque target; it
does not enumerate unrelated cached campaigns. Ordinary F2 Resume retains its
explicit multi-candidate selection. The existing Resume
request carries a restore-intent bit, but the server still validates the exact
durable identity and chooses `LastCommittedCheckpoint`. While the roster is
incomplete the intent is transient and the public state remains
`WAITING_FOR_ROSTER`. Once exact admission makes the canonical state `ACTIVE`,
the server opens the existing durable recovery and runs both barriers. Thus a
new `BeginRecovery` is still never created from `WAITING_FOR_ROSTER`, and an
uncorrelated `ACTIVE` projection cannot release the resume-required lock. See
[`CAMPAIGN_LOAD_CAMPAIGN.md`](../development/CAMPAIGN_LOAD_CAMPAIGN.md).

After the matching recovery completion releases that lock, ResumeRequired is
terminal: its exact local target is cleared, its UI state returns to idle, and
the STR surface closes. It never falls through into the ordinary Resume
candidate model; that model is populated only by a later explicit player action.

## Persistence

Campaign state, checkpoint metadata, roster/bindings, snapshots, revisions, and
commit state use the persistence port and accepted SQLite adapter described by
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
The opaque runtime core is encoded at the checkpoint's exact source revision
when the immutable snapshot is created, then re-encoded at the new restore
revision during materialization. A restored revision is consequently a normal
canonical base for later mutation and checkpoint creation; revision integers
never stand in for missing payload ownership.

The server-owned campaign runtime and live protocol implemented in the #28
workstream call this persistence substrate for Lobby roster configuration, the
campaign start/seal, Session Manager transfer, readiness, journal entries, and
outbox snapshot intents. The focused native/CEF New Game lobby projection is
implemented, automated-tested, and manually validated for Solo plus a two-PC
Create/Join, Character Creation, and inn-arrival happy path. Negative narrative
runtime scenarios remain pending; #56 recovery is live validated for N=1/N=2,
successive recovery, and restart rehydration. Durable
Character Build binding,
CK/Valen projection,
feature-owned later narrative phase execution, and Departure validation remain
incomplete. Issue #28 remains open and its tracking state does not supersede
those implementation gaps. Issue #55 coordinated native-save creation,
identity/fingerprinting, acknowledgement, and `CampaignCheckpoint` coordination
are implemented, automated/build-tested, and nominally validated with two real
Skyrim clients. Failure/disconnect, exact ACK replay, no-overwrite replay, and
commit-boundary interruption remain tracked by
[#72](https://github.com/matthieuAlbertelli/Skyrim-Together-Reborn-Enhanced/issues/72).
Issue #56 disconnect recovery lock and collective checkpoint restore/reload,
including the reviewed crash/reconnect blockers, are implemented,
automated/build-tested, and live validated for nominal N=1/N=2, successive
checkpoint/recovery, restart rehydration, and both disconnect incident UX
branches. See
[`CAMPAIGN_COLLECTIVE_RECOVERY.md`](../development/CAMPAIGN_COLLECTIVE_RECOVERY.md).
Durable WorldEntity persistence remains separate future work rather than part
of #55.

The standalone solo Alternate Start path remains independent of this multiplayer
full-roster rule and continues to restore from its local Skyrim save.
