# ADR-0019 — SQLite-backed durable server persistence

- **Status:** Accepted
- **Date:** 2026-08-13
- **Decision makers:** STRE maintainers
- **Issue / discussion:** [#27 — Persist durable campaign state and CampaignCheckpoint metadata](https://github.com/matthieuAlbertelli/Skyrim-Together-Reborn-Enhanced/issues/27)
- **Supersedes:** None
- **Superseded by:** None

## Context

[ADR-0002](ADR-0002-server-authoritative-campaign-state.md) makes the STRE
server authoritative for shared campaign state,
[ADR-0016](ADR-0016-state-journal-outbox.md) requires normalized current state,
an append-only validated-mutation journal, and a transactional replication
outbox, and
[ADR-0018](ADR-0018-fixed-roster-coordinated-checkpoint-recovery.md) requires
durable fixed-roster identity plus exact Candidate/Committed checkpoint
selection. The server currently has no durable implementation of those
boundaries.

The dedicated server already links SQLite. STRE needs an embedded store that
survives restart and crash without promoting the host, Session Manager, network
packets, or native Skyrim saves to persistence authority. One server process
must also be able to store more than one campaign without identity overlap.

## Decision forces

- Current campaign state must be directly readable without replaying the full
  history.
- State, journal, and required replication intent must not have partial commit
  windows.
- Schema changes and failed upgrades must have deterministic startup behavior.
- A committed checkpoint must permanently identify its exact source revision
  and immutable server snapshot.
- Restore must preserve append-only history and must not reuse revisions.
- Persistence types must remain independent from SQLite handles, EnTT entity
  identifiers, connection IDs, host status, local FormID prefixes, and network
  opcodes.
- The store must remain testable without constructing a Skyrim server `World`.

## Options considered

1. Flat files with an application-defined rewrite/journal protocol.
2. A remote database service required by every dedicated server.
3. SQLite behind a campaign persistence port.
4. Persist the current network messages or native `.ess` payloads directly.

## Decision

### Boundary and location

Use SQLite as STRE's embedded durable server-state backend behind
`ICampaignStore`. `SqliteCampaignStore` is an adapter for that port; SQLite is
not the campaign domain model. Domain identities and persistence DTOs remain
ordinary C++ types, and no service outside the adapter manipulates `sqlite3*` or
prepared-statement handles.

The database supports multiple campaigns. Its path is a locked dedicated-server
setting:

```text
STRE:sStateDatabasePath=state/stre-server.sqlite3
```

The parent directory is created when needed. The store opens and migrates before
the server constructs `World` and campaign-dependent services. Open, migration,
unsupported schema, and mandatory integrity failures produce actionable
diagnostics and fail startup closed.

### SQLite policy

Every connection enables and verifies:

```sql
PRAGMA foreign_keys = ON;
PRAGMA journal_mode = WAL;
PRAGMA synchronous = FULL;
```

Use a bounded busy timeout. Check every SQLite preparation, bind, step, commit,
and close result that can affect correctness. Use prepared statements for all
data. Dynamic or user-derived values are never concatenated into SQL.

The adapter owns RAII wrappers for the connection, prepared statements, and
transactions. Public operations return structured failures rather than throwing
through existing server `noexcept` boundaries.

### Logical data model

Schema version 1 contains the equivalent of:

- schema/migration metadata;
- `campaigns`, with current durable revision, roster-seal metadata, selected
  committed checkpoint, and versioned core state;
- `campaign_slots`, with unique `(CampaignId, SlotId)`, `PlayerId`, and opaque
  stable `CharacterBinding` records;
- versioned durable Character Build state that retains logical selections,
  canonical inventory/equipment, canonical spells, build/catalog version, and
  hashes without treating the network encoding as the database codec;
- versioned current adapter state with public/private audience metadata;
- immutable, versioned campaign snapshots with an integrity checksum;
- Candidate/Committed checkpoints and exact per-slot roster, native-save
  identity, fingerprint-algorithm/version/value, and versioned save metadata;
- an append-only validated-mutation journal;
- a transactional replication outbox whose pending entries can be delivered,
  or explicitly superseded by restore.

Core current state is normalized around campaign, slot, character-build, and
adapter records. Historical checkpoint snapshots are immutable and use an
independently versioned persistence codec. Persistence codecs do not depend on
numeric network opcodes or raw `ClientMessage`/`ServerMessage` bytes.

No native `.ess` payload is stored in this database in v1. The database records
only the identity/fingerprint/metadata needed to select and validate each
roster member's dedicated save. The fingerprint algorithm remains owned by the
coordinated-save work rather than this decision.

### Atomic mutation contract

Every validated canonical mutation uses one SQLite transaction:

```text
BEGIN IMMEDIATE
  verify MutationId/idempotency digest
  verify ExpectedRevision
  update normalized current state
  allocate the next monotonic revision
  append one journal record
  append every required outbox intent
COMMIT
```

Any failure rolls the whole transaction back. A repeated `MutationId` with the
same command is an idempotent replay and does not duplicate current state,
journal, or outbox rows. Reusing it for different content fails closed. A stale
`ExpectedRevision` leaves all durable tables unchanged. Database triggers also
reject journal update/delete and snapshot update/delete.

### Checkpoints and restore revisions

Creating a Candidate captures the exact current server revision in one immutable
snapshot and copies the expected sealed roster into checkpoint metadata. It may
remain incomplete while coordinated-save work records per-slot metadata, and it
never replaces `LastCommittedCheckpoint`.

Commit verifies the exact campaign/checkpoint/snapshot relationship and complete
required slot metadata, marks the Candidate Committed, and updates
`LastCommittedCheckpoint` in one transaction. Candidate N+1 failure leaves
committed N unchanged.

Restore never rewinds or reuses the durable mutation sequence. For example, if
checkpoint N's immutable `SourceRevision` is 100 and the campaign later reaches
110, restore materializes N through a new validated `RestoreCheckpoint`
mutation at revision 111:

- the checkpoint and snapshot continue to record source revision 100;
- normalized current state is replaced with the exact decoded snapshot;
- the append-only journal records checkpoint N, source revision 100, and result
  revision 111;
- pending outbox work superseded by the rollback is fenced;
- a canonical full snapshot is queued at revision 111;
- clients apply that full snapshot, then only events newer than 111, preserving
  [ADR-0004](ADR-0004-snapshot-plus-events.md).

The store selects a specifically named committed checkpoint. It never guesses
from whichever checkpoint or local save appears newest.

### Schema and migration policy

Migrations are explicit and transactional. A supported older schema migrates in
order. Failure rolls back and aborts startup. A database newer than the running
server fails closed. An incompatible or unknown database is never silently
deleted, recreated, truncated, or reset.

This issue introduces no automatic destructive retention or pruning policy.
Future durable `WorldEntity` state may reuse the storage substrate after its own
domain and migration design, but schema version 1 does not persist
`WorldEntity` records.

## Positive consequences

- Server authority survives process restart and transactional failure.
- Campaign identity, Character Build state, checkpoints, journal, and outbox
  share one enforceable consistency boundary.
- Persistence and recovery semantics can be tested with temporary file-backed
  databases independently of Skyrim runtime construction.
- Exact snapshot provenance and globally monotonic revisions make rollback safe
  for retransmission and append-only audit history.
- The port can support #28, #55, and #56 without pre-implementing their network,
  gameplay, native-save, or recovery responsibilities.

## Negative consequences

- SQLite schema and codec compatibility become long-lived maintenance duties.
- `synchronous=FULL` favors durability over maximum write throughput.
- Operators must preserve and back up the configured database path.
- Snapshot storage grows until a separately reviewed retention policy exists.

## Migration plan

Schema version 1 creates a fresh database transactionally. Version 0 is the only
older supported bootstrap source in the initial implementation. Unknown tables,
newer versions, malformed metadata, migration failure, and integrity failure all
leave the original file in place and refuse startup.

Future migrations increment the database schema independently of network
protocol and Character Build versions. They require file-backed migration,
rollback, reopen, and incompatible-newer-schema tests.

## Validation

- fresh database creation and reopen;
- supported migration, transactional migration failure, and newer-schema
  rejection without destructive reset;
- multi-campaign isolation and durable slot/binding/Character Build round trips;
- public/private adapter-state filtering;
- optimistic revision, MutationId, journal, outbox, constraint-failure, and
  injected crash-window tests;
- Candidate/Committed restart, partial N+1, identity mismatch, and exact
  checkpoint selection tests;
- exact snapshot restore at a new monotonic revision with obsolete outbox
  fencing;
- malformed persisted payload, bounded input, prepared-statement, and diagnostic
  tests;
- Windows and Linux server/test builds.

## Validation evidence

Human runtime/server validation completed successfully on August 13, 2026. A
real dedicated server created `stre-server.sqlite3` with schema version 1 and
the expected schema-v1 tables, accepted a real Skyrim client connection, shut
down cleanly, and reopened the same physical database after restart. The Vortex
staging and deployed Skyrim paths were confirmed as NTFS hardlinks to that same
file. Setting `schema_version` to unsupported value 999 correctly refused server
startup; restoring version 1 allowed normal startup again. Business tables were
empty before #28/#55 wiring, as designed.

## Implementation notes

The accepted adapter and automated persistence tests are tracked by #27. Live
fixed-roster creation/phase flow remains #28, coordinated native-save creation
and acknowledgement remains #55, and disconnect/reload recovery gameplay remains
#56.
