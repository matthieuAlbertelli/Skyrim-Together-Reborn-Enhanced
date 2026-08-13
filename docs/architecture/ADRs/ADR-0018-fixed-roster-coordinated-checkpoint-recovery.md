# ADR-0018 — Fixed roster and coordinated Skyrim-save checkpoint recovery

- **Status:** Proposed
- **Date:** 2026-08-13
- **Decision makers:** STRE maintainers
- **Issue / discussion:** [#26 — Implement v1 cooperative campaign continuity](https://github.com/matthieuAlbertelli/Skyrim-Together-Reborn-Enhanced/issues/26)
- **Supersedes:** [ADR-0015](ADR-0015-host-save-checkpoint.md)
- **Superseded by:** None

## Context

STRE v1 must restore both shared multiplayer state and the parts of a Skyrim
runtime that STRE does not serialize. The STRE server can persist campaign state,
but it is not a headless Skyrim engine and does not reproduce complete vanilla
quest, Papyrus, alias, scene, or Creation Engine runtime state. A quest stage
alone is not an adequate Skyrim save.

[ADR-0015](ADR-0015-host-save-checkpoint.md) made the host's `.ess` the canonical
external checkpoint. That gives one session role an accidental persistence
privilege and cannot restore the distinct local runtime of every player. It also
conflicts with the server-authoritative shared-state boundary established by
[ADR-0002](ADR-0002-server-authoritative-campaign-state.md).

## Decision forces

- Shared campaign mutations need one durable authority and one recoverable
  revision.
- Every roster member needs its own native Skyrim runtime restored coherently.
- A partially completed group save must not replace a known-good recovery point.
- Network loss must not allow the connected subset to create a divergent campaign.
- Local simulation authority, including the Havok model in
  [ADR-0017](ADR-0017-world-entity-authority-local-havok.md), remains distinct
  from persistent authority.
- Alternate Start must retain its standalone solo path.

## Options considered

1. Keep the host's `.ess` as the canonical checkpoint.
2. Reconstruct every Skyrim runtime entirely from server state.
3. Let connected players continue and catch a returning player up with events.
4. Seal the roster and coordinate one native save per slot with a matching server
   snapshot, then restore the complete group after an interruption.

## Decision

### Authority and roster

The STRE server is the persistent authority for shared state controlled by STRE.
The Session Manager is an administrative/logical role, not a persistence
authority, and the host has no special save privilege. Simulation authority may
still be delegated temporarily when a local engine must perform simulation.

A multiplayer campaign defines its roster before start. Starting the campaign
seals that roster. For STRE v1, sealed slots and their owners are immutable:

- no new player or slot may join;
- no player may replace another slot owner;
- no character from another campaign may occupy a slot;
- reconnecting players must present the expected `PlayerId`, slot, and
  `CharacterBinding`.

The campaign may progress only while the complete sealed roster is connected and
validated. There is no partial-roster continuation in v1. Roster configuration
before the seal is not campaign late join.

### CampaignCheckpoint

A `CampaignCheckpoint` atomically identifies one logical recovery point:

- `CampaignId` and unique `CheckpointId`;
- canonical server revision and matching STRE snapshot;
- the complete expected sealed roster;
- for every slot, its `PlayerId`, `CharacterBinding`, and dedicated native Skyrim
  save identity/fingerprint/metadata;
- lifecycle state `Candidate` or `Committed`.

Each roster member retains its own native `.ess`. That save restores the member's
local Skyrim, Papyrus, quest, alias, scene, and Creation Engine runtime. It is not
authority for shared STRE state. The server stores enough metadata to select and
verify the expected local save; STRE v1 does not upload `.ess` files to the server.

Checkpoint creation uses a coordinated barrier. A candidate commits only after
the matching server snapshot/revision and every required per-slot save
acknowledgement have succeeded. Failure of candidate N+1 never replaces the last
committed checkpoint N. Requests and acknowledgements must be idempotent and
retryable. The server always selects an exact committed checkpoint for recovery;
clients never independently load whichever save appears newest.

### Disconnect and collective restore

The campaign runtime state is separate from its narrative phase. A required
roster disconnect during `ACTIVE` moves the runtime to `RECOVERY_LOCK`.
Persistent campaign mutations and new checkpoint commits are fenced, and the
remaining players cannot continue campaign progression. Clients present a safe
local pause/freeze/recovery experience; this decision does not prescribe an
unverified engine call such as `PauseGame()`.

When the expected identity returns and the complete roster is present, the
runtime moves to `RESTORING_CHECKPOINT`. The server selects the last committed
checkpoint, restores its matching revision/snapshot, and instructs every roster
member—not only the returning player—to load that checkpoint's save for their
slot. The runtime returns to `ACTIVE` only after every client acknowledges the
same checkpoint and expected local save.

Reconnect is therefore collective rollback and restore, not event catch-up while
the rest of the campaign advances. Snapshots, versioned events, the journal, and
the transactional outbox remain useful for idempotent hydration, retransmission,
restart, convergence within the restored checkpoint, and WorldEntity late
materialization. They must not bypass the full-roster invariant.

The standalone solo Alternate Start path is outside this multiplayer roster and
recovery constraint and continues to rely on its local Skyrim save.

### Compatibility with existing decisions

This decision is compatible with
[ADR-0002](ADR-0002-server-authoritative-campaign-state.md): the server remains
the shared-state authority; with
[ADR-0004](ADR-0004-snapshot-plus-events.md): snapshots and newer events still
hydrate and converge the selected recovery point; with
[ADR-0016](ADR-0016-state-journal-outbox.md): current state, journal, and outbox
provide durable/idempotent mutation delivery; and with
[ADR-0017](ADR-0017-world-entity-authority-local-havok.md): temporary local
simulation authority remains separate from persistent authority.

## Positive consequences

- Persistent authority remains with the STRE server rather than a host role.
- Every player's distinct Skyrim runtime can be restored without pretending the
  server is a Skyrim engine.
- Partial saves and failures cannot silently replace a coherent recovery point.
- Disconnect behavior is deterministic and prevents partial-roster divergence.
- ADR-0004 snapshot-plus-events, ADR-0016 state/journal/outbox, and ADR-0017 local
  simulation authority remain compatible with the recovery model.

## Negative consequences

- A multiplayer campaign cannot progress while any sealed-roster member is absent.
- Every checkpoint requires coordination across all clients and the server.
- Recovery rolls every participant back, including players who remained connected.
- Players must retain the exact dedicated local save selected by the server.
- Save failure, unavailable saves, or an indefinitely absent member can leave the
  campaign suspended.

## Migration plan

ADR-0015 is superseded; no host save is promoted to the new canonical model.
Before runtime implementation, define versioned persistence and checkpoint
metadata, sealed-roster identity, checkpoint coordination, recovery state, local
save management, and player-facing failure behavior. Existing experimental state
must fail closed or undergo an explicit migration rather than being inferred as a
committed checkpoint.

A future ADR may permit roster mutation or partial-roster progression after the
server can persist enough Skyrim-relevant state and provide a validated catch-up
model. That evolution is explicitly outside STRE v1.

## Validation

- State-machine tests for seal, exact-roster eligibility, checkpoint commit, and
  recovery transitions.
- Persistence and crash tests before and after the checkpoint commit boundary.
- 2–4 player tests for missing, extra, replacement, wrong-binding, wrong-save,
  disconnect, reconnect, restart, retransmission, and collective restore cases.
- Evidence that no persistent campaign mutation or checkpoint commits during
  `RECOVERY_LOCK` and that `ACTIVE` resumes only after every acknowledgement.
- Solo Alternate Start regression tests proving the no-server path is unchanged.

## Implementation notes

Checkpoint cadence, triggers, safe points, autosave/manual-save interaction,
combat/dialogue/cell-transition behavior, exact save fingerprint, engine-safe
freeze mechanism, save-failure UX, and storage schema/migrations remain
implementation questions. No numeric protocol opcode or checkpoint cadence is
allocated by this ADR.
