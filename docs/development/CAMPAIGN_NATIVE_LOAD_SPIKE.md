# Campaign native-load Slice 0 spike

> **Status:** human runtime-validated on 25 August 2026; the temporary
> CEF/chat validation harness was removed before integration. Production #56
> recovery orchestration remains outside this spike.
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
`STRECampaignGateMenu`. The menu's `PostDisplay` callback must then observe
`UI::GameIsPaused() == true`. A real connected `TransportService` update while
locked is the last required proof. Only an explicit service release can remove
the gate and reset the terminal correlation.

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
created admission. That behavior remains historical validation evidence, not a
new production resume surface.

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

### Validated conclusion and remaining #56 scope

STRE now has a production-capable primitive for deterministic programmatic
loading of one exact native Skyrim save, correlated through the existing
`BGSSaveLoadManager::Load_Impl` hook and completed through `TESLoadGameEvent`,
with an engine-pausing managed gate that preserves CEF and network liveness.

This resolves the technical blocker identified during the #56 audit. It does
not implement issue #56. The following remain unimplemented:

- `RecoveryService` and the client recovery state machine;
- `RestoreAttemptId` and restore-protocol orchestration;
- full-roster collective rollback;
- the `LoadedAndLocked` acknowledgement barrier;
- canonical server snapshot restore;
- the `SnapshotApplied` acknowledgement barrier;
- durable completion and restart reconstruction;
- no-checkpoint recovery diagnostics;
- live multi-client resilience validation.

## Historical validation procedure

The detailed run above was completed before the temporary harness was removed.
It used a production Angular build plus the three now-removed commands to obtain
authoritative admission, invoke the exact load, reject a duplicate, and release
the terminal gate. Before/after hashes and timestamps proved that the native
bundle remained immutable, and a subsequent ordinary Skyrim load proved the
unarmed path remained unmanaged.

That procedure is intentionally no longer executable from player-facing UI.
Future #56 work must call the retained service from the production recovery
orchestrator and must not restore a chat, debug-key, or console trigger.

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
runtime-validated** for the exact evidence above. This verdict applies only to
the focused local primitive and managed gate; it does not claim #56 recovery
orchestration, collective rollback, server snapshot restoration, or durable
recovery completion.
