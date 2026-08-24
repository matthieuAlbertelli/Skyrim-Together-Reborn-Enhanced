# Coordinated campaign checkpoints

> **Status:** issue #55 implementation, automated validation, and Windows
> client/launcher/server builds are complete. The nominal coordinated checkpoint
> path was validated end-to-end with two real Skyrim clients on 24 August 2026.
> Remaining live resilience validation is tracked by
> [#72](https://github.com/matthieuAlbertelli/Skyrim-Together-Reborn-Enhanced/issues/72);
> recovery and restore remain issue #56.

## Authority and lifecycle

The production path reuses the existing layers:

```text
CampaignProtocolService
  -> CampaignAdmissionService
  -> CampaignRuntimeService
  -> ICampaignStore
  -> SqliteCampaignStore
```

An explicit server command, `stre_checkpoint <CampaignId>`, generates the
`CheckpointId`, derives `stre-<CheckpointId>`, and asks the runtime to begin.
The campaign must exist, be sealed, have its exact full roster admitted, and
have no active checkpoint. SQLite captures the immutable Candidate snapshot and
source revision before the runtime publishes transient `CHECKPOINTING`.

While that activity exists, `CampaignRuntimeService` fences unrelated durable
mutations for that campaign. The map is keyed by `CampaignId`, contains only the
active checkpoint identity/source revision/native identity, and is not durable
authority. Candidate roster ownership remains exclusively in SQLite. Another
campaign remains independently usable.

Each exact Candidate slot records one canonical acknowledgement. The server
derives `PlayerId`, `CampaignSlotId`, and `CharacterBindingId` from the admitted
connection; none are accepted from the packet. When every Candidate slot has a
complete, valid artifact, `CommitCheckpoint` atomically selects it as
`LastCommittedCheckpoint`, clears the transient activity, and returns the
runtime to `ACTIVE` when the full-roster predicate still holds.

Checkpoint-only persistence operations advance the campaign revision without
changing the core-state payload. Runtime loading therefore identifies the most
recent core-state-producing journal mutation, strictly decodes the payload at
that revision, verifies that only checkpoint journal operations follow it, and
then projects the current durable revision. This preserves codec integrity
without coupling SQLite checkpoint transactions to the runtime core codec.

## Protocol

Only two new message directions exist.

Server to client, `CampaignCheckpointSaveRequest`:

- `CampaignId`;
- `CheckpointId`;
- `SourceRevision`;
- expected `NativeSaveIdentity`.

Client to server, `CampaignCheckpointSaveResult`:

- `CampaignId`;
- `CheckpointId`;
- expected `NativeSaveIdentity`;
- success or failure;
- on success only: `SHA-256`, fingerprint version 1, exactly 32 fingerprint
  bytes, metadata codec version 1, and at most 256 metadata bytes.

The result has no player, slot, binding, path, expected revision, or
client-generated mutation identity. IDs reuse the canonical bounded wire
codec. Result enums, success/failure shape, algorithms, codecs, fingerprint
length, metadata length, truncation, and the canonical
`Fingerprint = SHA256(SaveMetadata)` relationship fail closed.

## Client save and replay

`CampaignCheckpointService` is the small network/native boundary. It validates
the request against the current client admission and delegates fresh creation
to the existing `CampaignNativeSave` primitive. A successful artifact is
atomically persisted before the ACK in a bounded local file keyed by a SHA-256
of `CampaignId` plus `CheckpointId`. The file contains only versioned logical
identities, deterministic metadata, and the fingerprint; it contains no path
or server campaign authority.

If the exact request is retransmitted, `stre_checkpoint_resend <CampaignId>`
sends the same server-owned fields. The client loads its cached artifact,
resolves the existing `.ess` and `.skse`, opens both read-only without
write/delete sharing, re-hashes every byte, rebuilds the canonical metadata,
and requires exact equality with the cache before resending the same result.
It never invokes Skyrim Save, overwrites the files, invents a new identity, or
silently accepts a conflict. Malformed/unsupported cache content is preserved
and rejected.

Server checkpoint mutations use stable logical IDs derived from checkpoint and
slot identity. For an ACK replay, the runtime obtains the original expected
revision from the durable journal before invoking
`RecordCheckpointSlotSave`; this preserves ADR-0016 replay semantics without a
second in-memory revision ledger.

## Failure, disconnect, and restart

- A client failure clears the transient activity but leaves the Candidate and
  all local/server evidence intact.
- A disconnect abandons the transient activity before presence is removed. The
  runtime projects `WAITING_FOR_ROSTER`; it does not enter `RECOVERY_LOCK`.
- An incomplete Candidate never replaces the previous committed checkpoint.
- A crash before commit leaves an unfinished Candidate that is not resumed
  automatically. A crash after commit resolves the new
  `LastCommittedCheckpoint`.
- There is no retry policy, scheduler, autosave cadence, upload, retention,
  pruning, deletion, or cleanup. Old and failed saves remain indefinitely.

These bounds are intentional for #55. Collective recovery, native load, and
gate release are #56; late join/catch-up remains forbidden by ADR-0018.

## Nominal two-PC runtime evidence — 2026-08-24

One real campaign with `sealed=true` and `present=2/2` completed the production
path:

- campaign: `campaign-367760f49cba23fd72a5ad5013a75e1b`;
- checkpoint: `checkpoint-4a33f050b434778db8b09094658831d5`;
- native identity on both clients:
  `stre-checkpoint-4a33f050b434778db8b09094658831d5`;
- source revision: 3;
- Candidate revision: 4.

The server accepted the exact roster slots in this order:

1. `slot-01`, revision 5, `committed=false`, fingerprint
   `df3375f2a880046c219edd91b3249c176a788c37756d9da5e9f23573c5f1ea45`;
2. `slot-02`, revision 7, `committed=true`, fingerprint
   `19f95c49fa39ba16084e37a243199456c583fcbf04950a3086e78a3d07d61c51`.

The server then emitted `CHECKPOINT_COMMITTED` at revision 7. This proves in
live multiplayer that the first successful slot did not commit the Candidate
and that commit occurred only after the second required slot succeeded.

Both real clients received the same server-owned request, automatically invoked
Skyrim's native save path, produced their own `.ess`/`.skse` bundle, completed
the fail-closed observer, sent a successful result, and matched independent
PowerShell per-file hashes. The accepted global fingerprints are intentionally
different because each player owns a distinct native payload. Shared authority
is the checkpoint ID, source revision, operation, identity convention, and
server commit boundary—not byte-identical saves. Detailed member sizes and
hashes are retained in
[`CAMPAIGN_NATIVE_SAVE_SPIKE.md`](CAMPAIGN_NATIVE_SAVE_SPIKE.md).

No host save authority was involved; each exact canonical slot was required.

## Deferred resilience validation

Automated coverage exercises begin eligibility, runtime state/fence behavior,
canonical connection authority, malformed/conflicting/duplicate/out-of-order
ACKs, exact 2/4/10-slot commits, failure/disconnect preservation, independent
campaigns, wire robustness, local cache restart, and replay conflict. The
nominal two-PC checkpoint path above is also runtime-validated.

Live client failure/disconnect during `CHECKPOINTING`, lost-ACK exact replay and
no-overwrite proof, and server interruption before/after commit remain tracked
by [#72](https://github.com/matthieuAlbertelli/Skyrim-Together-Reborn-Enhanced/issues/72).
They are not claimed as completed runtime validation. Collective recovery
remains separate issue #56.
