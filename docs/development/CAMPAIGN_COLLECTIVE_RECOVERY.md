# Collective campaign recovery

> **Status:** issue #56 production implementation, including the four blocking
> crash/reconnect review cases, is automated-tested and the Windows
> client/server targets build. The post-load guard-menu proof blocker is fixed,
> and a cold one-member run completed the authoritative recovery. The first
> two-client rerun after canonical recipient preparation then completed live
> end-to-end. A second consecutive recovery failed closed after durable restore
> because the preceding restore revision labeled a core payload still encoded
> at its older source revision. Checkpoint creation and restore now canonicalize
> that payload at their exact revisions, with sequential N=1/N=2 and restart
> regressions. The next live restart proved canonical snapshot dispatch for the
> previously incomplete r14 attempt, then exposed that a fresh client had no
> in-memory attempt/checkpoint correlation and correctly rejected the direct
> snapshot replay. Recovery rehydration now replays the exact native-load
> barrier first and durably journals per-slot barrier receipts as accepted
> no-ops. A fresh N=2 rerun live validated nominal recovery, the successive
> checkpoint B/recovery B cycle, and restart rehydration of the same durable
> attempt/revision without a second restore. The #56 recovery contract is live
> validated. Both disconnect incident branches are now live validated:
> `StayAndRecover` completed the N=2 authoritative recovery, while the corrected
> `ReturnToMainMenu` dispatch reached a responsive Main Menu without a CTD or
> zombie gameplay gate and then re-entered the existing #56 rehydration through
> Continue/Resume.

## Authority and invariant

The persistent STRE server remains authority for shared campaign state. A
sealed v1 roster never changes: a required-member disconnect from `ACTIVE` or
`CHECKPOINTING` locks the entire campaign, all exact roster members must return,
and every member rolls back to the same committed `CampaignCheckpoint`. Each
member loads only the `.ess` plus
`.skse` bundle recorded for its own canonical
`PlayerId`/`CampaignSlotId`/`CharacterBindingId`. Those native files restore
local Skyrim/Papyrus runtime; they do not author shared STRE state.

No host or Session Manager chooses a checkpoint, no partial roster can continue,
and no late join, player replacement, stage-only quest reconstruction, cleanup,
retention, upload, or catch-up path is introduced.

## Mandatory disconnect incident UX

The gameplay fence remains immediate and unconditional when an admitted ACTIVE
client observes authoritative recovery or loses its own transport. A small
presentation-only state then opens an interactive STRE incident over the guard
menu. It derives the current missing count and roster ordinals from the latest
sealed snapshot; the public snapshot intentionally exposes no durable display
name, so the UI uses one-player/multiple-player fallbacks and never treats a
transient player ID as display identity. Local transport loss has separate
connection-lost wording.

`StayAndRecover` is not a rollback request. It emits no campaign packet, chooses
no checkpoint or attempt, and never calls Skyrim's native load boundary. It
only replaces the incident presentation with the existing campaign resume/
recovery component and its `WaitingForRoster`, `Recovery`, and `Synchronizing`
phase mapping. The real `CampaignRecoveryLoadRequest` remains the sole trigger
for the native checkpoint load, and only correlated authoritative completion
closes the surface and releases gameplay.

`ReturnToMainMenu` records one bounded local request and returns from the
initiating CEF/service action before its next service update touches Skyrim. The
native projection sets only the top-level `Main::resetGame` request and leaves
the distinct `fullReset` content-reset flag untouched. STRE then waits for the
semantic `CampaignMainMenuEnteredEvent` emitted when the Main Menu opens before
removing the local guard projection;
the existing runtime-departure lifecycle then clears volatile admission and
projections, closes transport, and retains the durable campaign/character
binding. It emits no `LeaveCampaign` and does not change the server's durable
recovery attempt. A later marked load or explicit Resume re-enters the existing
ResumeRequired/#56 rehydration path.

The first live action click crashed before the former native diagnostics. The
CEF callback was already marshalled through the existing Skyrim update runner,
so the corrective pass added one-shot boundary traces from Angular through
native receipt/dispatch and removed the unneeded stronger reset request. The
corrected live rerun reached a responsive Main Menu without CTD, released the
gate only at `MainMenuEntered`, retained the durable campaign binding, and
successfully resumed the existing #56 recovery through Continue/Resume. This
branch is runtime validated.

This mandatory safety recovery is separate from any future voluntary collective
rollback proposal/vote feature. No rollback vote, timeout, refusal path, host
privilege, protocol field, or persistence state is introduced here.

After a full Skyrim restart, the production connected-menu resume surface can
enumerate the existing local binding cache without exposing canonical IDs to
Angular. Explicit token selection sends the existing `CampaignResumeRequest`;
only its authoritative server response establishes admission. A sealed
incomplete roster remains waiting, while a member resuming into an already-open
attempt advances this recovery through the existing barriers and never creates
a replacement `RestoreAttemptId`. This surface is automated/build-tested and
human runtime-validated through cold marked-save Resume and Continue/Resume
re-entry into an existing #56 attempt.

## Durable attempt and runtime states

Disconnect removes transient presence. It appends a durable `BeginRecovery`
journal mutation only when a new recovery starts from `ACTIVE` or
`CHECKPOINTING`; a disconnect from `WAITING_FOR_ROSTER` never opens one, while a
disconnect during an existing recovery reopens its native-load barrier. The
resulting begin revision deterministically derives the
`RestoreAttemptId`; the pair `(CampaignId, RestoreAttemptId)` correlates all
subsequent messages. The runtime then projects:

- `RECOVERY_LOCK` while the roster is incomplete or no committed checkpoint is
  recoverable;
- `RESTORING_CHECKPOINT` during native loading and snapshot application;
- `ACTIVE` only after both collective barriers complete.

The same central runtime fence rejects checkpoint creation and unrelated durable
campaign mutations while recovery is open. A Candidate interrupted by
disconnect remains uncommitted; `LastCommittedCheckpoint` remains the selected
rollback point.

## Protocol and barriers

After exact admission restores the complete sealed roster, the server resolves
the one durable `LastCommittedCheckpoint` and privately sends each connection a
`CampaignRecoveryLoadRequest` containing its exact slot, binding, native
identity, fingerprint, and codec-v1 metadata.

The client locks gameplay before invoking the validated native-load primitive.
It reopens and hashes the locally cached artifact and acknowledges
`CampaignRecoveryLoadedResult` only with the exact committed proof. Missing,
wrong, malformed, stale, or failed evidence keeps the gate and campaign locked.
After `TESLoadGameEvent`, the first world update proves that
`STRECampaignGateMenu` is actually open and `UI::GameIsPaused()` is true. This
also covers a menu that was already open in `RECOVERY_LOCK` and survived the
load without emitting another `PostDisplay`; absent or unpaused state fails
closed.

Only after every exact slot acknowledges the native load does the server call
`RestoreCheckpointSnapshot`. SQLite materializes the immutable shared snapshot
at one new monotonic revision, preserves source checkpoint/revision provenance,
and supersedes obsolete outbox work. The checkpoint boundary re-encodes the
runtime core at the exact immutable `SourceRevision`; the restore boundary
re-encodes the same state at the new `RestoreRevision` before updating current
state and producing the canonical outbox snapshot. Thus a restore-generated
revision is a first-class source for a later checkpoint/recovery cycle. Existing
checkpoints written before this rule are decoded only through their exact,
bounded restore lineage in the durable journal; no current or unrelated
snapshot is substituted. Before sending the correlated
`CampaignRecoverySnapshot`, the server resolves each checkpoint
`CampaignSlotId`/`PlayerId`/`CharacterBindingId` to exactly one current admitted
connection and resolves every corresponding live `Player`. This preparation is
all-or-nothing and ordered by the durable checkpoint roster; old transient IDs
from before reconnect grant no authority. Missing, duplicate, unexpected, or
unresolvable recipients produce no partial dispatch and leave the durable
attempt replayable in its existing recovery state. Clients apply the snapshot and acknowledge
`CampaignRecoverySnapshotApplied`. After every exact slot reaches that second
barrier, a durable accepted no-op `CompleteRecovery` marker closes the attempt,
the server sends `CampaignRecoveryComplete`, and clients release the local gate.

Duplicate requests and acknowledgements are idempotent. Correlation mismatches,
wrong revisions, wrong identities, incomplete presence, and invalid artifacts
fail closed.

## Restart and ordering

Recovery reconstruction scans the append-only journal:

- `BeginRecovery` without a matching restore restarts the native-load barrier;
- every accepted `Loaded` and `SnapshotApplied` slot receipt is a stable
  accepted no-op in that same journal. These records do not advance or mutate
  canonical campaign state, but make partial barrier evidence and exact replay
  idempotency reconstructible without a schema or protocol change;
- a matching `RestoreCheckpoint` proves the restore is already durable, but a
  restarted server does not send its snapshot directly. It replays the exact
  native-load request for the same attempt/checkpoint so a fresh client process
  acquires correlation and re-proves its local bundle. Survivors answer
  `ResendLoaded`; the current admitted roster must still produce a fresh
  volatile full-roster barrier before snapshot dispatch;
- once that replay barrier completes, the existing stable restore mutation and
  revision are reused. The exact snapshot is replayed, durable Applied receipts
  remain recognizable/idempotent, and the current admitted roster must likewise
  re-prove the second barrier before completion;
- matching `CompleteRecovery` means no attempt remains open; a replayed exact
  `SnapshotApplied` ACK still receives the idempotent completion message.

Stable slot-receipt, restore, and completion mutation IDs make replay safe
across process restart. No schema migration is needed because schema v2 already
records accepted no-op receipts, the checkpoint, immutable snapshot, and restore
provenance.

Server command handling is serialized. If a final checkpoint ACK commits before
the disconnect event, that checkpoint is eligible as the last committed point.
If disconnect wins, the transient Candidate is abandoned and a late ACK cannot
commit it; recovery selects the prior committed checkpoint.

## Failure behavior and validation boundary

An incomplete roster, absent committed checkpoint (`NO_COMMITTED_CHECKPOINT`),
client load failure, unavailable local bundle, or stale packet never releases a
correlated recovery gate. Retrying the same attempt across server/client restart
or transport loss is safe. A transport disconnect while the server was `ACTIVE`
uses a provisional local lock; an authoritative `ACTIVE` snapshot after
readmission may release only that uncorrelated provisional lock.

Automated coverage exercises disconnect lock, mutation fencing, no-checkpoint
failure, exact two-barrier completion, monotonic restore, duplicate ACKs,
restart before and after restore, disconnect during snapshot application,
completion-message crash replay, provisional transport locking,
checkpoint/disconnect ordering, strict wire validation, client correlation, and
fail-closed gate behavior. It now also covers a two-member dispatch plan after
both reconnect under new transient IDs, missing-recipient all-or-nothing
failure, durable restore replay without a second revision, and completion only
after 2/2 Applied ACKs. Restart coverage now preserves and recognizes partial
durable Loaded/Applied receipts while rebuilding both volatile full-roster
barriers, and proves that a fresh client rejects the snapshot until the exact
load request establishes its correlation. A sealed roster of one uses the same
generic barrier
implementation: its sole exact `Loaded` ACK immediately completes the first
barrier, restore produces one canonical revision, its sole exact `Applied` ACK
immediately completes the second barrier, and only authoritative completion
returns it to `ACTIVE`. There is no one-member production branch.

The diagnostic build records the exact campaign, attempt, checkpoint, and
revision at `LOAD_RESULT_SENT`, `LOAD_RESULT_RECEIVED`,
`LOADED_BARRIER_COMPLETE`, `RESTORE_APPLIED`, `RESTORE_SNAPSHOT_SENT`,
`SNAPSHOT_APPLIED`, `APPLIED_RESULT_SENT`, `APPLIED_RESULT_RECEIVED`,
`APPLIED_BARRIER_COMPLETE`, and `RECOVERY_COMPLETION_RECEIVED`, including send
success and idempotent actions. These records distinguish a missing client
continuation from transport failure, stale correlation, server barrier/restore,
snapshot application, or completion replay without weakening the gate.
`RECOVERY_REHYDRATION_STATE` additionally reports the persisted phase, replay
action, exact correlation/revision, durable per-slot receipt sets, volatile
barrier counts, and whether the restore is already durable.

The recipient correction has now completed one fresh two-client recovery live:
Loaded 2/2, one durable restore at revision 9, snapshot dispatch 2/2, Applied
2/2, and durable completion. A subsequent checkpoint used
`sourceRevision=9`; its next recovery durably produced revision 15 and resolved
both recipients, then emitted `reason=snapshot-unavailable`. Audit proved that
the revision-9 campaign row and checkpoint snapshot still carried a core
payload encoded at the older pre-restore revision, so the runtime correctly
rejected it after the second restore. The corrected path normalizes both
checkpoint and restore payload revisions, while missing/corrupt material remains
fail-closed. Automated coverage now includes three consecutive cycles at N=1
and N=2, second-cycle recipient resolution, replay without a second durable
restore, persistence reload between cycles, and corrupt snapshot rejection.
`RESTORE_SNAPSHOT_DISPATCH_FAILED` now reports source/restore revisions plus
checkpoint/runtime snapshot presence without logging payloads. The next live
restart of that incomplete r14 attempt proved the payload correction at the
dispatch boundary: source revision 9/restore revision 15 resolved and sent the
snapshot to both recipients. The fresh client correctly rejected that direct
snapshot because its expected attempt/checkpoint were empty. The corrected
rehydration now replays the exact native-load barrier before snapshot replay and
reuses revision 15 rather than restoring again. The fresh two-client rerun
completed that exact persisted attempt through both barriers and authoritative
completion, without a second durable restore.
