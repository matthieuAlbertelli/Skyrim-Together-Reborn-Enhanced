# Campaign native-load Slice 0 spike

> **Status:** human runtime-validated on 25 August 2026; the temporary
> CEF/chat validation harness was removed before integration. Production #56
> recovery orchestration now consumes this primitive but remains documented
> separately.
> **Issue:** #56, Slice 0 only.

## Question and scope

This spike asks whether one already validated logical native-save identity,
`stre-<CheckpointId>`, can be loaded programmatically on the Skyrim game thread
and correlated through the existing managed-load gate. It does not implement
recovery authority, protocol messages, roster handling, checkpoint selection,
server state, SQLite state, or a recovery state machine.

The successful proof is deliberately stronger than the native return value:

```text
artifact revalidation -> exact Load_Impl entry -> native return true
-> TESLoadGameEvent -> LockedAfterLoad -> guard menu active and paused
-> connected transport update -> completed
```

## Audited native boundary

The existing `CampaignSaveLoadGate` hook remains the only owner of
`BGSSaveLoadManager::Load_Impl`. CommonLibSSE-NG and the current hook establish
the ABI as:

```cpp
bool __fastcall BGSSaveLoadManager::Load_Impl(
    const char* saveName,
    std::int32_t deviceId,
    std::uint32_t outputStats,
    bool checkForMods);
```

For Skyrim AE, the Address Library ID is `35728`. The manager singleton pointer
is resolved with ID `403340`. CommonLib's public wrapper supplies `-1`, `0`, and
`true` for the final three arguments; the spike does the same.

`saveName` is Skyrim's logical save basename, not a path and not a filename with
`.ess`. The exact input is therefore the existing identity
`stre-<CheckpointId>`. Skyrim resolves the associated `.ess`, while the
established save-completion path already treats `.ess` and `.skse` as the
required native bundle. An owned `std::string` keeps the C string alive through
the synchronous call boundary.

During validation, the removed CEF harness queued the call through
`RunnerService`; it ran while `World::Update` drained the runner on the
established game-update thread. The retained primitive preserves that
game-thread requirement for its future #56 caller. The native Boolean proves
only that `Load_Impl` accepted/succeeded at its boundary; it is never used as
final recovery proof.

## Player-load policy and cold-session provenance

The production `Load_Impl` hook now also owns one small pure player-load
policy. Its inputs are deliberately narrower than the native call arguments:

- target evidence is `Ordinary`, `Campaign`, or `Unknown`;
- authority is either a player action or the exact active #56 native-load
  correlation;
- sensitive runtime evidence is an authoritative campaign admission or an
  already locked campaign gate;
- a valid local `stre-campaign-save-v1` marker is required before a cold target
  is classified as `Campaign`.

The matrix allows the exact internal #56 correlation first, blocks every
player load while campaign runtime evidence exists, sends a cold marked target
through the existing resume-required gate, and leaves an ordinary outside-
campaign target vanilla. A caller-controlled `stre-*` name is never authority:
without its exact valid marker it is an unproven reserved target and is blocked
before Skyrim loads it. Null or otherwise unavailable target evidence remains
`Unknown`; it fails closed in a campaign-sensitive context and cannot bypass an
internal recovery.

The hook records bounded `[STRE][CampaignLoadTrace]` lines for `Load_Impl`,
`TESLoadGameEvent`, QuickLoad's exact `QuickSaveLoadHandler::ProcessButton`, and
Main/Journal menu context. Each line
contains a process-wide sequence, STRE frame, Windows thread, and the fields
available at that seam. `Load_Impl` additionally records the original target
pointer/presence/readability, a bounded name only after guarded inspection,
target source, exact native scalar arguments, admission/gate state, internal
correlation, target classification, and decision. The event order makes the
native return and `TESLoadGameEvent` routing visible without treating elapsed
time as evidence.

CommonLibSSE-NG exposes `LoadMostRecentSaveGame()` at AE ID `35766`, the public
`saveGameList` member at offset `0x100`, and each entry's public `fileName`. A
candidate adapter observes the front entry at that semantic boundary and owns a
copy only for the synchronous call stack; it neither copies a `BSFixedString`
nor retains an entry or character pointer. Its ID, ABI, MinHook trampoline, and
non-overlap with the save-list population functions were audited against the AE
1.6.1170 Address Library after a reproducible crash while opening Show All
Saves. The dump contains no `LoadMostRecentSaveGame`, `Load_Impl`, or STRE
save-list frame: it faults in `GFxValue::ObjectInterface::ObjectAddRef` (ID
`82269`) while the native `CharacterSelected` callback (best-resolved AE ID
`52919`) copies a fourth Scaleform argument even though its `FxDelegateArgs`
contains exactly one. The active 2017 `SkyUI_SE.bsa` overrides
`Interface/quest_journal.swf`; that obsolete movie sends only the historical
Show All selection value, whereas the 1.6.1170 native callback expects the
expanded Journal contract. The vanilla 1.6.1170 movie is materially different
and includes the post-1.6.1130 save/Creations surface. This is an installed UI
compatibility failure, not checkpoint metadata corruption or an STRE hook
write. The ID `35766` adapter therefore remains enabled; a compatible
`quest_journal.swf` is still required to rerun Show All Saves. Main Menu
`Continue` uses its separately proven exact `LoadRequest` lineage and is live
validated through ResumeRequired and #56 re-entry. `deviceId`, time windows, UI
text, and filename prefix are not accepted as authority.

F9 has one earlier enforcement point solely to preserve correct Skyrim UX. An
actionable QuickLoad press evaluates the same pure policy; when admission or
the gate already makes the runtime sensitive, STRE consumes that press before
Skyrim can reinterpret `Load_Impl == false` as a corrupt save. It does not
classify the target, map a device ID, or create a second policy. Cold QuickLoad
still enters the common `LoadMostRecentSaveGame`/`Load_Impl` path.

Manual load now also has an early semantic enforcement boundary. Runtime
disassembly of AE 1.6.1170 `UISaveLoadManager::Accept` proves that the literal
Scaleform callback `LoadGame` is registered on Address Library adapter `52914`
(the adjacent proven `SaveGame` sibling is `52915`). The callback receives the
selected save-list index; STRE resolves that index against the same public
CommonLib `saveGameList`, immediately owns a bounded copy of the target, and
evaluates the common `CampaignLoadPolicy`. With authoritative admission or an
already locked gate, every player target is `BlockPlayerLoad` and STRE consumes
the void callback without forwarding it. The native load operation and its
fade/transition are therefore never created. No provenance, selection, or
decision survives the callback.

The first live rerun of that seam confirmed `Consumed` with no `Load_Impl`, no
`TESLoadGameEvent`, no fade, and no campaign-gate lock. It also exposed the
callback's actual UI contract: before invoking native `LoadGame`, SkyUI's
`SystemPage` has already set the save-list `disableSelection` flag and its own
`bMenuClosing` flag. The callback ABI returns `void` and exposes no supported
failure/cancel response or completion callback. Forwarding the original creates
the native operation that normally completes the transition; simply returning
leaves those private SWF states armed and the visible Journal non-interactive.

STRE therefore projects the same policy decision onto a stateless UX action.
For either blocked player decision it still consumes the callback, then queues
the normal `Journal Menu` `UIMessage::kHide` message and publishes a localized
system notification. It does not call any part of the original, mutate SWF
state, write private offsets, repair a fade, or use a timer. Allowed vanilla,
cold `BeginResumeRequired`, and exact internal #56 decisions forward unchanged.

Outside campaign, ordinary targets still forward unchanged. A cold valid
campaign marker forwards as `BeginResumeRequired`; the final `Load_Impl`
boundary remains responsible for arming the existing resume-required gate.
Unavailable early target evidence also continues to that final safety boundary
outside a sensitive runtime rather than inventing a classification. Journal
open/close events remain observational; the blocked callback requests closure
through Skyrim's normal UI message queue. `Load_Impl` still evaluates the same
policy and blocks any forbidden operation that bypasses the UI seam. The second
live rerun confirmed `Consumed -> JournalCloseRequested -> JournalClosed`, the
localized player notification, and the absence of fade, `Load_Impl`,
`TESLoadGameEvent`, and campaign-gate acquisition. The
post-load owner remains transient:
`InternalRecovery`, `ResumeRequired`, or `Vanilla`; only the first two can arm
the gate, and `TESLoadGameEvent` dispatches to the already active owner. There
is no local rollback, new protocol, persistence, recovery state, vote, or
release path.

## Validation and ownership

`CampaignNativeLoad` owns only the exact native call. The validation-only
`CampaignNativeLoadService` coordinates this spike and selects nothing: it
requires the current campaign admission and uses its exact campaign ID plus the
checkpoint ID embedded in the requested identity to read the cached
`NativeSaveBundleArtifact`.

Before arming or invoking the load, the service:

1. validates the exact `stre-<CheckpointId>` syntax;
2. reads and parses the cached artifact without modifying it;
3. rejects an active `CampaignNativeSave` operation;
4. calls `CampaignNativeSave::ValidateExistingOnGameThread`;
5. waits for the existing completion path to reopen and hash both members;
6. requires the resulting metadata and fingerprint to equal the cached
   artifact exactly.

That reused path requires the exact `.ess` and `.skse`, rejects an `.ess.tmp`,
and detects missing, inaccessible, unstable, or hash-mismatched members. The
load path never saves, repairs, renames, deletes, or overwrites a member or the
artifact cache.

## Correlation, gate, and failures

One pure request object holds one expected identity and one bounded lifecycle.
Only the exact expected name observed at the existing `Load_Impl` hook consumes
the armed gate. A normal manual load while idle is not managed. An unrelated
name during an outstanding invocation fails that request and continues as an
ordinary native load without acquiring the STRE gate.

The coordinator arms `CampaignRuntimeGate` immediately before invoking the
native call. The existing `TESLoadGameEvent` callback performs the transition
to `LockedAfterLoad`, applies the input lock, and requests
`STRECampaignGateMenu`. Completion requires post-load observable state: the
menu is actually open and `UI::GameIsPaused() == true` at the first world update
after `TESLoadGameEvent`. The menu's `PostDisplay` callback remains another
valid proof path, but is not required to fire again when a recovery-lock menu
survives the native load. A real connected `TransportService` update while
locked is the last required proof. An absent menu or unpaused game fails closed;
only an explicit service release can remove the gate and reset the terminal
correlation.

A second request is rejected while validation, invocation, or proof is active,
and remains rejected after completion while the gate is locked. Terminal state
is cleared only by the explicit service release. There is no retry and no
fallback save. Failure reasons are bounded, including invalid identity,
unavailable/invalid artifact, native-save busy, validation rejection/failure,
gate-arm failure, unavailable native boundary, unrelated native load, native
rejection, missing post-load, missing gate/menu/pause/transport proof, timeout,
and internal failure.

Validation has a failure deadline only: 35 seconds for artifact validation and
60 seconds for post-load/safety proof. Elapsed time can never establish
success.

## Removed validation harness

Human validation used three temporary chat commands:

```text
/stre-campaign-resume <CampaignId>
/stre-native-load stre-<CheckpointId>
/stre-native-load-release
```

Their Angular commands, TypeScript declarations/mock methods, TPProcess V8
registrations, OverlayClient handlers, shared bridge manifest, and bridge-only
tests were removed after the successful run. They are not present as
production-facing UX. No replacement debug key, console command, or local
admission shortcut was added.

The cold-session harness called the existing
`CampaignService::ResumeCampaign` on `RunnerService` and never modified
`CampaignClientAdmissionState`. The cached binding was used only to build the
existing `CampaignResumeRequest`; the server still had to accept the exact
PlayerId, membership, and CharacterBinding before the normal response handler
created admission. That harness remains historical validation evidence. The
production replacement now enumerates the same non-canonical cache through a
read-only `CampaignIdentityStore` API, projects only ephemeral local tokens to
the connected Angular menu, and resolves an explicit selection through the same
`CampaignService::ResumeCampaign` call. It adds no chat/console command, server
protocol, persistence, local admission shortcut, or Character Creation
authorization path.

## Runtime validation evidence — 2026-08-25

The focused spike was validated in a cold Skyrim client session against:

```text
Campaign:       campaign-367760f49cba23fd72a5ad5013a75e1b
Checkpoint:     checkpoint-4a33f050b434778db8b09094658831d5
Native identity: stre-checkpoint-4a33f050b434778db8b09094658831d5
```

### Cold-session authoritative resume

The development harness emitted `RESUME_QUEUED` and
`RESUME_SENT sent=true`. The existing admission handler then recorded:

```text
[STRE][CampaignAdmission] server-validated admission accepted
operation=2
campaign=campaign-367760f49cba23fd72a5ad5013a75e1b
revision=7
```

`operation=2` is `CampaignProtocolOperation::Resume`. No locally synthesized
admission was used; the cached binding only supplied the resume request and the
server response remained the admission authority.

### Exact native-load completion

The client produced the complete correlated sequence:

```text
REQUEST_QUEUED
REQUESTED
ARTIFACT_VALIDATION_SCHEDULED
ARTIFACT_VALIDATED
LoadArmed
SCHEDULED
INVOKE
NATIVE_ENTER
BGSSaveLoadManager::Load_Impl result=true
NATIVE_RETURN success=true
TESLoadGameEvent
POST_LOAD
GATE_LOCKED
TRANSPORT_ALIVE connected=true
GUARD_MENU_ACTIVE
GAME_PAUSED value=true
COMPLETED
```

The exact checkpoint visibly loaded. Skyrim gameplay was frozen after load,
F2/CEF remained responsive, and network transport remained alive while the
managed gate was locked.

### Single-flight and explicit release

While the completed managed load remained locked, a duplicate trigger emitted:

```text
REQUEST_QUEUED
REQUEST_REJECTED reason=request-not-idle
```

There was no second `INVOKE` and no second `NATIVE_ENTER`. Explicit release then
produced:

```text
RELEASE_QUEUED
CampaignGate Released
RELEASED
PauseMenuDestroyed gateRemainsLocked=false
```

Gameplay control returned immediately.

### Native bundle immutability

The exact bundle was measured before and after the managed load. Comparison
produced no differences:

```text
ESS
length=2600863
LastWriteTime=2026-08-24 18:01:39
SHA256=8AC74662C3AC18F599C36690253907465326AD721B5BCE5D357176F0F83E6123

SKSE
length=2789
LastWriteTime=2026-08-24 18:01:39
SHA256=3FC8EA1291BE750871F23094E93723BC964EDF3E7C7CFE20B45D4D51033403CF
```

No matching `.tmp` existed before or after. The load therefore did not modify,
replace, repair, resave, or otherwise mutate the checkpoint bundle.

### Ordinary-load regression

After explicit release, a vanilla/manual Skyrim save load remained unmanaged.
It produced no `REQUESTED`, `ARTIFACT_VALIDATED`, `LoadArmed
owner=CampaignNativeLoadService`, managed `INVOKE`/`NATIVE_ENTER`,
`GATE_LOCKED`, or `COMPLETED` records.

### Validated conclusion and production #56 consumption

STRE now has a production-capable primitive for deterministic programmatic
loading of one exact native Skyrim save, correlated through the existing
`BGSSaveLoadManager::Load_Impl` hook and completed through `TESLoadGameEvent`,
with an engine-pausing managed gate that preserves CEF and network liveness.

This resolved the technical blocker identified during the #56 audit. The
production #56 implementation now consumes this primitive through
`CampaignRecoveryService`, correlated `RestoreAttemptId` messages, two
full-roster barriers, canonical server snapshot restore, durable restart
reconstruction, and explicit no-checkpoint diagnostics. Those additions are
automated/build-tested and live validated for nominal N=1/N=2, successive
recovery, and durable restart rehydration without a second restore revision.
See [`CAMPAIGN_COLLECTIVE_RECOVERY.md`](CAMPAIGN_COLLECTIVE_RECOVERY.md).

## Historical validation procedure

The detailed run above was completed before the temporary harness was removed.
It used a production Angular build plus the three now-removed commands to obtain
authoritative admission, invoke the exact load, reject a duplicate, and release
the terminal gate. Before/after hashes and timestamps proved that the native
bundle remained immutable, and a subsequent ordinary Skyrim load proved the
unarmed path remained unmanaged.

That procedure is intentionally no longer executable from player-facing UI.
The production #56 orchestrator now calls the retained service and does not
restore a chat, debug-key, or console trigger.

## Historical ordered evidence

The accepted client log contained this ordered native-load trace (gate records
could interleave):

```text
[STRE][CampaignNativeLoad] REQUEST_QUEUED identity=...
[STRE][CampaignNativeLoad] REQUESTED identity=...
[STRE][CampaignNativeLoad] ARTIFACT_VALIDATION_SCHEDULED identity=...
[STRE][CampaignNativeLoad] ARTIFACT_VALIDATED identity=... fingerprint=...
[STRE][CampaignNativeLoad] SCHEDULED identity=... boundary=RunnerService-game-update
[STRE][CampaignNativeLoad] INVOKE identity=... nativeName=...
[STRE][CampaignNativeLoad] NATIVE_ENTER identity=... nativeName=... boundary=Load_Impl
[STRE][CampaignNativeLoad] POST_LOAD boundary=TESLoadGameEvent
[STRE][CampaignNativeLoad] GATE_LOCKED state=LockedAfterLoad
[STRE][CampaignNativeLoad] NATIVE_RETURN success=true completion=unproven
[STRE][CampaignNativeLoad] GUARD_MENU_ACTIVE menu=STRECampaignGateMenu
[STRE][CampaignNativeLoad] GAME_PAUSED value=true
[STRE][CampaignNativeLoad] TRANSPORT_ALIVE connected=true update_observed=true
[STRE][CampaignNativeLoad] COMPLETED identity=... proof=...
```

The engine could order `NATIVE_RETURN` before `POST_LOAD`; both orders were
accepted, but all distinct milestones were mandatory. The duplicate while
locked showed `REQUEST_REJECTED reason=request-not-idle` with no second
invocation.

## Current verdict

The deterministic native checkpoint-load primitive is **human
runtime-validated** for the exact evidence above. The broader #56 recovery
orchestration is also automated/build-tested and live validated for nominal
N=1/N=2, successive recovery, and durable restart rehydration without a second
restore revision; those broader claims are owned by
[`CAMPAIGN_COLLECTIVE_RECOVERY.md`](CAMPAIGN_COLLECTIVE_RECOVERY.md).
