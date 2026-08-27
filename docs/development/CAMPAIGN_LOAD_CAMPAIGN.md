# Load Campaign production flow

> **Implementation status (2026-08-27):** the cold marked-save backend path was
> exercised in Skyrim through exact checkpoint restore and authoritative #56
> completion. That run exposed a presentation defect: ResumeRequired still
> enumerated unrelated cached campaigns. The exact-target correction was then
> exercised live through another nominal authoritative completion; that run
> exposed one final lifecycle defect where the completed panel remained open as
> OrdinaryResume. ResumeRequired now terminates atomically to idle and closes
> the surface only after authoritative recovery release. This correction is
> automated-, Playwright-, Windows build-, and live-tested. A later live
> same-process Quit-to-Main-Menu rerun confirmed the
> volatile-admission clear, durable-binding retention, and real transport
> close. It exposed a separate client-only projection defect where that
> intentional disconnect still opened the gameplay recovery gate over the Main
> Menu. The semantic one-shot local-gate correction is live validated: Main Menu
> remains responsive without a zombie gameplay gate; server recovery behavior is
> unchanged.
> Main Menu `Continue` target resolution is now live-proven: the exact callback
> root creates one direct typed `LoadRequest` child carrying the canonical
> `.ess` target. Functional interception at its native consumer is implemented
> and live validated through Continue/Resume re-entry into the existing #56
> recovery. Issue #56 is live validated for N=1, N=2, successive recovery, and
> durable incomplete-attempt rehydration without a second restore revision.

This feature turns the cold-session `Resume campaign` surface into the safe
entry point for loading a sealed STRE campaign. It does not make a native save,
the UI, the local cache, or the former Session Manager authoritative. The
server's campaign, immutable roster/bindings, `LastCommittedCheckpoint`,
per-slot artifact records, and canonical snapshot remain the only authority.

## Local save marker

The client has no registered SKSE serialization/co-save API. The production
metadata seam is therefore a small versioned sidecar in the existing
`CampaignIdentityStore` directory. Its filename is keyed by SHA-256 of the
exact logical native-save identity, and its bounded payload contains:

- format `stre-campaign-save-v1`;
- `CampaignId`;
- `CampaignSlotId` as a non-authoritative consistency hint;
- `CharacterBindingId`;
- `CheckpointId`;
- the exact `stre-<CheckpointId>` native-save identity.

It contains no secret, filesystem path, snapshot, authorization, or server
state. The sidecar is written only after the managed #55 `.ess`/`.skse` bundle
has completed and its canonical artifact has been durably cached, and before a
successful checkpoint ACK is sent. A marker failure turns that ACK into a
failure so an unidentifiable save cannot become a committed checkpoint.

Only #55-managed checkpoint saves have an unambiguous completion and identity
boundary. Inside a campaign, Manual and Quick Save are routed to that collective
checkpoint flow rather than producing ordinary local saves; autosaves and
unknown save families are blocked. Outside a campaign, all remain vanilla.
Only the eventual server-dispatched managed save is marked and promoted to
checkpoint authority. Old markers are retained;
after `LeaveCampaign` removes the binding candidate, loading such a save still
fences gameplay and reports that no exact cached binding is available.

## Engine-safe load boundary

The existing `BGSSaveLoadManager::Load_Impl` hook recognizes only the bounded
`stre-` identity namespace. Before invoking Skyrim it reads the marker and arms
`CampaignRuntimeGate`. Missing, malformed, unsupported, or unavailable marker
data still arms the gate; if the fail-closed gate itself cannot be armed, the
native load is rejected. A failed native load returns the gate to its prior
state.

Vanilla Main Menu `Continue` bypasses `Load_Impl`. Its separate production seam
is the first exact typed `LoadRequest` proven to descend directly from the
`ContinueLastSavedGame` callback request. Immediately before Skyrim consumes
that request, STRE converts the canonical `name.ess` to the extensionless
`NativeSaveIdentity` and evaluates the same player `CampaignLoadPolicy`.
Ordinary saves retain the original native consumer unchanged. A valid marked
save establishes only logical ResumeRequired ownership and its exact identity
before that original consumer runs; STRE does not cancel and relaunch the save.
The runtime gate, input lock, and guard menu remain inactive while Main Menu
still owns the UI. If the native consumer accepts the exact correlated request,
the semantic `MainMenuClosed` event commits that pending transition directly to
the existing post-load ResumeRequired gate exactly once. A rejected native
request, a replacement attempt, or a failed commit clears the pending ownership.
A blocked/unproved target is consumed before native transition and uses the
existing public Main Menu rebuild. The decision is one-shot per exact root
lineage, and all request correlation is cleared before forwarding, so requeues
or later requests cannot duplicate ResumeRequired. Operation codes, device IDs,
timestamps, and save-list order are diagnostic only and never grant authority.

After `TESLoadGameEvent`, the gate remains locked, the native guard menu pauses
Skyrim, CEF/network processing continues, and the STR surface opens as soon as
`OverlayService` observes an in-game `PlayerCharacter` with a NiNode. The
resume popup explains that the loaded save cannot continue offline.
ResumeRequired resolves the marker directly to zero or one opaque local target;
it never enumerates, labels, disables, or offers other cached campaigns.

The Main Menu is not a viable production surface in the current client:

- `OverlayService::Render()` reports in-game only when a `PlayerCharacter` and
  its NiNode exist;
- `UiSurfaceService::SetSurface()` rejects interactive surfaces while that
  predicate is false;
- Angular mounts the application controls under its `inGame` projection.

Consequently there is no fabricated Main Menu player, alternate New Game path,
or new Skyrim hook. The earliest reliable boundary is a real marked save chosen
through Skyrim's native load UI. The hook fences that load before gameplay and
the CEF choice appears at the first engine-safe in-game boundary.

## Gameplay-to-Main-Menu lifecycle

Skyrim's public `MainMenuOpened` event is also the semantic end of an admitted
loaded-game runtime. A fresh boot emits the same event, but has no campaign
admission and is therefore a no-op; no elapsed-time or frame heuristic is used.
When admission exists, the client clears the volatile admission, runtime
projection, and automatic same-process reconnect candidate while retaining the
durable `CampaignIdentityStore` binding. It then closes the real transport.
The existing server disconnection path removes the connection presence and
enters or retains collective recovery according to the canonical runtime. It
does not issue `LeaveCampaign`, alter the sealed roster, delete a binding, or
add a new protocol message. A later marked Main Menu load is consequently cold
with respect to admission and enters `BeginResumeRequired`.

That transport close is also correlated to the semantic Main Menu runtime
departure exactly once. `CampaignRecoveryService` consumes this context before
projecting the disconnect locally: it keeps the authoritative server recovery
semantics but logs `LOCAL_GATE_SKIPPED` and does not create a provisional
recovery lock or open `STRECampaignGateMenu` while the client has no gameplay
world to fence. There is no frame, timing, thread, or menu-label heuristic.
Uncorrelated disconnects in an admitted gameplay world retain the existing
fail-closed gate. The skip does not persist into the next world: a subsequent
marked native load still arms ResumeRequired normally and hands that gate to
#56 until authoritative recovery completion.

`LoadGame` is a shared `void` Scaleform callback and has no supported cancel
result. As a fail-safe edge defense, blocked confirmations inspect the public
owning menu. In-world Journal loads retain the live-validated normal Journal
`kHide` path. If a stale/edge block occurs under Main Menu, the client consumes
the callback and queues public `Main Menu` `kHide` then `kShow` messages. This
destroys the disabled/busy SWF instance and reconstructs the normal menu; it
does not write private SWF flags, use offsets, timers, synthetic clicks, fades,
or partially invoke the native callback. `Load_Impl` remains the final safety
boundary in both contexts.

## Unified campaign surface

The marked-save prompt and the ordinary cold-session Resume entry reuse the
same `CampaignShell` and `CampaignRoster` presentation primitives as the
Create/Join campaign bootstrap. They do not create a second campaign overlay.
The existing connection form is embedded when the marked save is not yet
connected, with its cancel and public-server exits removed because neither can
release the mandatory gameplay fence.

For a valid marked save, the native service reads only the binding named by the
marker `CampaignId`, then requires exact campaign, slot-hint, and character-
binding equality before minting one opaque target. Missing, corrupt, or
mismatched data produces no target and fails closed. This path never calls the
binding enumeration used by ordinary F2 Resume, whose explicit multi-campaign
selection remains unchanged.

Angular therefore renders no candidate list in ResumeRequired. It offers at
most one `Resume this campaign` action and projects a bounded progress sequence:
local save verification, server connection, authoritative campaign
verification, sealed-roster waiting, collective checkpoint restore, and
canonical synchronization. Other cached campaigns are not offered, disabled,
or exposed as identifiers. The roster projection contains only ordinal,
presence, and a local-slot flag; durable player, slot, campaign, and binding
identifiers never cross into Angular. Sealed campaigns no longer retain the
pre-seal lobby pseudos, so the waiting roster uses `You`/ordinal player labels
rather than inventing names.

F2 may hide the overlay while the native guard menu keeps gameplay fenced. The
mandatory view remains selected and reappears unchanged when F2 reopens the
overlay; the native client auto-opens it only once after the marked load rather
than defeating the player's F2 toggle. The surface disappears automatically
only after the correlated recovery completion has actually released the gate.
No UI action can synthesize `ACTIVE`.

That completion is a terminal ResumeRequired transition, not a conversion to
ordinary Resume. Once the correlated recovery has produced authoritative
`ACTIVE` and released the managed gate, the native state clears its exact token,
candidates, roster, marker, and error, projects `Unavailable`, and closes the STR
surface. Angular also closes the mandatory view on that terminal projection.
The ordinary multi-campaign model is populated again only if the player later
opens Resume explicitly. Failures and retries retain the mandatory surface and
cannot take this completion path.

Admission errors and locally observable native recovery failures remain in the
same shell with a concise reason and retry. A recovery retry replays the exact
already-selected campaign through the existing idempotent Resume request, so it
can only cause the server to resend the current authoritative barrier. The
existing protocol does not carry the server-only `NO_COMMITTED_CHECKPOINT`
diagnostic to CEF; that case therefore remains locked in the recovery state and
diagnosed in server logs rather than fabricating a client-side reason.

The disconnect incident offers an engine-safe `ReturnToMainMenu` action. It
records one bounded request, returns from the initiating handler, and dispatches
only Skyrim's top-level `Main::resetGame` request on the next service update;
the distinct `fullReset` flag remains untouched. The local gameplay gate is
released only after the semantic `CampaignMainMenuEnteredEvent`, while volatile
admission is cleared, transport is closed, and the durable campaign binding and
server recovery attempt are retained. The live rerun reached a responsive Main
Menu without CTD or zombie gate, then completed Continue/Resume re-entry into
the existing #56 recovery. No console command, Papyrus workaround, fake New
Game, arbitrary load, `LeaveCampaign`, protocol, or persistence change was
introduced.

## Authoritative resume and recovery

Native selection calls the existing `CampaignService::ResumeCampaign()` with a
single `RestoreCommittedCheckpoint` intent bit on the existing
`CampaignResumeRequest`. The bit conveys no checkpoint choice or authority.
The server still validates durable `PlayerId`, exact immutable membership,
server slot, and `CharacterBindingId` before admission.

For an accepted cold load, the server holds only a transient per-campaign load
intent while the roster is incomplete. When the last exact member resumes, the
canonical runtime first becomes `ACTIVE`; only then does the server append the
existing durable `BeginRecovery` and run the unchanged #56 flow. This preserves
the reviewed invariant that a new recovery never begins from
`WAITING_FOR_ROSTER`. An already-open recovery is reused, including its
`RestoreAttemptId` and durable restore revision.

The client retains this restore intent across a same-process transport/server
reconnect only until it observes the authoritative recovery. If the server
crashes before durable `BeginRecovery`, the next exact Resume replays the
intent; if the attempt was already durable, reconstruction wins and the
transient intent is discarded. Once a correlated recovery has been observed,
the intent is cleared so the #56 completion-crash ACK replay cannot accidentally
open a second attempt.

The client gate hands ownership from `resume-required` to the correlated #56
recovery without opening. The local checkpoint is loaded again through the
existing native-load service, its exact artifact is verified, the restored
canonical snapshot crosses both collective barriers, and only the matching
`CampaignRecoveryComplete` releases gameplay. A generic `ACTIVE` snapshot
cannot release this lock because it is neither the transport-only provisional
lock nor a completed correlated recovery.

## Failure behavior

All failure paths retain the gameplay fence and provide a bounded UI error or
existing #56 server diagnostic:

- missing/corrupt/unsupported marker or unavailable local store;
- no matching cached binding, loaded campaign X versus selected campaign Y;
- server unreachable, send failure, deleted campaign;
- `IdentityMismatch` or `BindingMismatch`;
- missing `LastCommittedCheckpoint` (`NO_COMMITTED_CHECKPOINT` server
  diagnostic and recovery lock);
- missing/changed `.ess` or `.skse`, artifact/fingerprint mismatch, rejected
  native load, snapshot mismatch, incomplete roster, or barrier timeout.

There is no solo fallback, host privilege, local admission, checkpoint choice,
new persistence authority, recovery state machine, save upload, cleanup, or
retention policy.
