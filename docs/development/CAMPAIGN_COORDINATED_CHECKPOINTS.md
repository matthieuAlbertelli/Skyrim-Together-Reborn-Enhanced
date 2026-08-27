# Coordinated campaign checkpoints

> **Status:** issue #55 implementation, automated validation, and Windows
> client/launcher/server builds are complete. The nominal coordinated checkpoint
> path was validated end-to-end with two real Skyrim clients on 24 August 2026.
> Issue #72 resilience validation is complete through deterministic
> failure/replay/ordering evidence and a live abrupt post-commit server restart;
> the narrow mid-ACK packet/timing races remain explicitly non-live.
> recovery and restore are implemented separately by issue #56. The production
> multiplayer save-policy surface is implemented and automated-tested; Manual
> NewSlot and Quick are live validated, while Manual ExistingSlot overwrite and
> Auto provenance remain unproved and fail closed.
> The native Skyrim Gameplay rows cannot currently be disabled through an
> audited engine-safe seam; the STR Settings projection remains informational.

## Authority and lifecycle

The production path reuses the existing layers:

```text
CampaignProtocolService
  -> CampaignAdmissionService
  -> CampaignRuntimeService
  -> ICampaignStore
  -> SqliteCampaignStore
```

In production, a Skyrim Manual Save or Quick Save sends a bounded checkpoint
intent from the admitted client. The server derives the campaign exclusively
from that connection, generates the `CheckpointId`, derives
`stre-<CheckpointId>`, and asks the runtime to begin. The explicit server
command `stre_checkpoint <CampaignId>` remains a development/validation seam.
The campaign must exist, be sealed, have its exact full roster admitted, and
have no active checkpoint. SQLite captures the immutable Candidate snapshot and
source revision before the runtime publishes transient `CHECKPOINTING`. The
runtime re-encodes its authoritative core at that exact source revision for the
snapshot, so a checkpoint created immediately after recovery does not retain an
older embedded revision.

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
without coupling SQLite checkpoint transactions to the runtime core codec. The
immutable checkpoint snapshot separately owns the source-revision-normalized
core payload supplied by that runtime boundary.

## Protocol

The original checkpoint barrier retains its two message directions.

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

The player-facing policy adds one minimal request and one presentation outcome:

- client to server, `CampaignCheckpointRequest`, contains only the classified
  `Manual` or `Quick` reason. It contains no campaign, checkpoint, revision,
  player, slot, binding, mutation ID, or host claim;
- server to client, `NotifyCampaignCheckpointState`, reports bounded
  `Started`, `Committed`, or `Failed` presentation state correlated with the
  server-owned campaign/checkpoint when one exists.

The server accepts an intent only from an exactly admitted member while its
authoritative campaign is `ACTIVE` with the full sealed roster. A concurrent
intent during `CHECKPOINTING` returns the same open activity; it creates no
second Candidate, checkpoint identity, or revision.

## Client save and replay

`CampaignCheckpointService` is the small network/native boundary. It validates
the request against the current client admission and delegates fresh creation
to the existing `CampaignNativeSave` primitive. A successful artifact is
atomically persisted before the ACK in a bounded local file keyed by a SHA-256
of `CampaignId` plus `CheckpointId`. The file contains only versioned logical
identities, deterministic metadata, and the fingerprint; it contains no path
or server campaign authority.

The same success boundary also writes a versioned non-canonical campaign-save
sidecar keyed by SHA-256 of the exact `stre-<CheckpointId>` logical identity.
It contains campaign, slot hint, character binding, checkpoint, and native
identity only. If this marker cannot be persisted, the client reports
checkpoint failure instead of allowing an unidentifiable save to commit.
Manual and Quick Save inside an admitted campaign are intercepted before
Skyrim writes a local vanilla save and become the collective request above.
Autosaves and unknown/modded filename families are blocked fail-closed. The
resulting server-dispatched managed save is the only operation that receives a
marker or checkpoint authority. The marker is consumed only by the fail-closed cold-session flow in
[`CAMPAIGN_LOAD_CAMPAIGN.md`](CAMPAIGN_LOAD_CAMPAIGN.md).

The production hook is `BGSSaveLoadManager::Save_Impl` (AE Address Library ID
`35727`) with the exact CommonLibSSE-NG ABI `(self, int32 deviceId, uint32
outputStats, const char* fileName)`. The original pointer, detour thunk,
trampoline call, and #55 managed call all retain that order, and an automated
sentinel test prevents a regression to the different `Load_Impl` order. STRE
classifies safely readable case-insensitive `Save*`, `QuickSave*`, and
`AutoSave*` names; a null, unreadable, empty, unterminated, or different family
is `Unknown`. It never converts `Unknown` to Manual.

The first live player-facing policy run on AE 1.6.1170 found a blocker before
origin classification: both Manual Save and QuickSave reached the correctly
typed hook with `fileName == nullptr`. Therefore the earlier
`nameAvailable=false` diagnostic was the actual value of the filename register,
not evidence of a `Load_Impl`-style argument-order bug. The hook now records
`deviceId`, `outputStats`, pointer present/null, the bounded filename only when
it can be read safely, and the resulting classification. Manual/Quick native UI
classification could not come from that null name; the apparently blocked
autosave is also not accepted as Auto evidence because a null name is `Unknown`.
Subsequent causal tracing
proved the exact Quick request-pointer lineage into `Save_Impl` and the
synchronous Manual NewSlot callback scope. Those explicit provenance transports
are now live validated end-to-end. Manual ExistingSlot overwrite and Auto remain
unproved and fail closed; neither is inferred from device ID or exclusion.

The #55 managed call crosses that same hook with an explicit thread-local,
scoped internal provenance. That provenance—not the `stre-` prefix—is the only
bypass, so the managed save cannot recursively request another checkpoint and
a user/mod cannot obtain the bypass by choosing a filename.

Outside a campaign all origins pass through unchanged. Inside a campaign:

- `ACTIVE`: Manual/Quick request a checkpoint;
- `CHECKPOINTING`: Manual/Quick coalesce with the open checkpoint;
- `WAITING_FOR_ROSTER`, recovery lock, restore, resume-required, or any runtime
  gate fence: Manual/Quick are blocked temporarily;
- Auto and Unknown are blocked in every campaign runtime state.

The runtime hook is the authority even if UI projection is absent or stale.
The STR Settings surface projects Skyrim's four supported autosave preference
families—rest, wait, travel, and character menu—as localized, unchecked,
disabled controls while admitted/fenced in a campaign. This projection never
writes Skyrim's preferences; leaving the campaign restores the ordinary view
with the user's settings untouched. Requested, committed, failed, automatic-
save-blocked, unknown-blocked, and temporarily-unavailable outcomes use the
existing localized system-message surface.

### Vanilla Gameplay menu projection audit — 2026-08-25

The supported and locally installed target is Skyrim AE `1.6.1170.0` with SKSE
`2.2.6`. The real Gameplay settings are part of `Journal Menu`, backed by
`quest_journal.swf`. Native `JournalMenu` owns a `Journal_SystemTab`; its
`JournalTab` base exposes only the movie view and `Journal_SystemTab::Accept()`
registers the native Scaleform callbacks. Neither CommonLibSSE-NG nor STRE's
smaller local UI wrappers expose a typed Gameplay-row model.

Audit evidence is the repository's
[`1.6.1170`/`2.2.6` compatibility matrix](../testing/COMPATIBILITY_MATRIX.md),
the installed `SkyrimSE.exe` file version, STRE's `IMenu`/`UI` and custom-movie
wrappers, CommonLibSSE-NG's published
[`JournalMenu`](https://github.com/CharmedBaryon/CommonLibSSE-NG/blob/main/include/RE/J/JournalMenu.h),
[`Journal_SystemTab`](https://github.com/CharmedBaryon/CommonLibSSE-NG/blob/main/include/RE/J/Journal_SystemTab.h),
and [`JournalTab`](https://github.com/CharmedBaryon/CommonLibSSE-NG/blob/main/include/RE/J/JournalTab.h)
layouts, plus SkyUI's published
[`SystemPage`](https://github.com/schlangster/skyui/blob/master/src/PauseMenu/SystemPage.as),
[`OptionsList`](https://github.com/schlangster/skyui/blob/master/src/PauseMenu/OptionsList.as),
and [`SettingsOptionItem`](https://github.com/schlangster/skyui/blob/master/src/PauseMenu/SettingsOptionItem.as)
ActionScript. Bethesda does not publish the vanilla ActionScript source; the
installed vanilla movie is archived in `Skyrim - Interface.bsa`, not exposed as
a stable native API. The exact post-`1.6.1130` Gameplay list used by `1.6.1170`
is independently recorded in SkyUI
[#97](https://github.com/schlangster/skyui/issues/97). This provenance is itself
why a private object path cannot be promoted to a supported engine contract.

The audited ActionScript path is:

```text
Quest_Journal.SystemFader.Page_mc (SystemPage)
  -> onSettingsCategoryPress()
  -> OptionsListsPanel.OptionsLists.List_mc.entryList
  -> GameDelegate.RequestGameplayOptions(entryList)
  -> OptionsList.SetEntry()
  -> SettingsOptionItem
  -> GameDelegate.OptionChange(ID, value)
```

For AE `1.6.1170`, the actual labels and control types are `$Save on Rest`,
`$Save on Wait`, and `$Save on Travel` as checkbox rows (`movieType == 2`),
plus `$Save on Pause` as an option stepper (`movieType == 1`) whose final choice
is `$Disabled`. `$Save on Pause` is localized in English as "Save on Character
Menu"; it is the fourth family previously described as character/menu context.
The extra `$SaveGameMissingCreationsCheck` row added in `1.6.1130` places these
four entries at zero-based positions 8 through 11. Bethesda's native hydration
maps this list by position, so those positions are not a safe authorization or
long-term identity seam.

There is no per-entry disabled contract in this movie. `OptionsList.SetEntry()`
copies only `movieType`, stepper options, `ID`, `value`, and `text`.
`SettingsOptionItem` then handles keyboard/controller input directly in
`handleInput()` and mouse input in `onMousePress()`/`onRelease()`; all three
control types eventually call `OptionChange`. Its `selected` alpha is focus
presentation, not a disabled state. The movie's `disableInput` switch is at the
whole-list level and therefore cannot disable only the four autosave rows.

The same movie has no per-setting description/help field. `HelpTextPanel` is a
separate Help-topic state populated through `RequestHelpText`, not a help target
for Gameplay entries. Entering Gameplay rebuilds `entryList`, requests the
native values, invalidates the list, and focuses it. Returning to the category
and entering Gameplay repeats that path; closing `Journal Menu` destroys the
menu instance. A projection would therefore have to be reapplied at every row
construction and refresh, as well as when campaign state changes while the
options list is already open.

The repository can find an open `IMenu` and queue UI messages, but its
`IMenu::uiMovie` is an untyped `void*`. The only existing Scaleform manipulation
uses raw vtable calls to host a separate custom movie; it has no `GFxValue`,
`Invoke`, `SetVariable`, row provider, or callback-registration surface for the
vanilla Journal movie. STRE starts SKSE through `StartSKSE` rather than loading
as an SKSE plugin, so SKSE's movie-creation registration interface is not
available to this DLL. Even that interface only supplies a movie/root object;
it does not add the missing row-level disabled/help behavior to
`SettingsOptionItem`.

Consequently, the requested native projection is a known UX limitation. Making
it real would require at least one excluded heavyweight path:

- ship and own a forked `quest_journal.swf` that adds a row-level disabled/help
  contract and campaign callback, conflicting with other Journal/SkyUI movies;
- inject or replace private ActionScript methods/pooled row instances using
  hard-coded movie paths, then separately guard keyboard, controller, and mouse
  handlers across every rebuild;
- hook undocumented native `Journal_SystemTab` callbacks/row indices, which
  still cannot make the existing checkbox MovieClip non-interactive or add help
  text without altering the movie;
- change the underlying INI/preferences so native hydration displays disabled
  values, which changes user state and still does not disable interaction.

No such path was added. During a campaign, the four real vanilla controls
therefore remain visually and interactively vanilla and may still change the
user's stored preferences. They cannot permit an autosave: the independent
`CampaignSavePolicy` hook remains fail-closed. Outside campaigns the menu is
strictly untouched. STR Settings remains a localized secondary explanation and
does not claim to modify the native menu or preferences.

Reconsidering this limitation requires either an ADR that accepts ownership and
compatibility policy for a replacement Journal SWF, or a separately validated
typed GFx/ActionScript contract that provides row-level disabled state, help
text, all-device input suppression, and refresh notifications without relying
on row indices.

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
- A disconnect abandons the transient checkpoint activity before presence is
  removed. With #56 integrated, a sealed campaign then enters collective
  `RECOVERY_LOCK`; the Candidate remains uncommitted and the previous committed
  checkpoint remains selected.
- An incomplete Candidate never replaces the previous committed checkpoint.
- A crash before commit leaves an unfinished Candidate that is not resumed
  automatically. A crash after commit resolves the new
  `LastCommittedCheckpoint`.
- There is no retry policy, scheduler, checkpoint cadence, upload, retention,
  pruning, deletion, or cleanup. Old and failed saves remain indefinitely.

These bounds are intentional for #55. Collective recovery, native load, and
gate release are implemented by #56 and documented in
[`CAMPAIGN_COLLECTIVE_RECOVERY.md`](CAMPAIGN_COLLECTIVE_RECOVERY.md); late
join/catch-up remains forbidden by ADR-0018.

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

## Resilience validation

Automated coverage exercises begin eligibility, runtime state/fence behavior,
canonical connection authority, malformed/conflicting/duplicate/out-of-order
ACKs, exact 2/4/10-slot commits, failure/disconnect preservation, independent
campaigns, wire robustness, local cache restart, and replay conflict. The
nominal two-PC checkpoint path above is also runtime-validated.

Additional coverage proves the expected native filename families, the exact
`Save_Impl` ABI/order, vanilla behavior outside campaigns, Manual/Quick routing
and state gating, Auto/Unknown
fail-closed behavior, scoped internal recursion prevention, one-Candidate
coalescence under duplicate intent, strict intent/outcome wire validation, and
the conditional informational autosave projection in STR Settings. The vanilla
Gameplay rows are not projected as disabled; the audit above records the known
UX limitation. Live Skyrim validation found that the correctly typed hook
receives a null filename for Manual and Quick UI saves, then proved exact
request-pointer Quick provenance and scoped Manual NewSlot provenance. Both
player-facing paths are now live validated; Manual ExistingSlot and Auto remain
unproved and fail closed.

Issue #72 completed the checkpoint resilience gate using proportionate evidence.
Deterministic tests cover partial-Candidate disconnect, both
checkpoint/disconnect orderings, exact ACK replay/conflict behavior,
no-overwrite artifact revalidation, restart, and the atomic Candidate ->
Committed boundary. A live two-player force-kill after a fresh commit preserved
the exact `LastCommittedCheckpoint`, revision, snapshot, and both slot artifacts
after restart and readmission. The millisecond mid-ACK disconnect,
first-ACK packet loss, and pre-commit force-kill races were not manually
reproduced and are not claimed as live validation. Collective recovery is a
separate #56 implementation and is live validated for nominal N=1/N=2,
successive recovery, durable restart rehydration, and both disconnect incident
UX branches.
