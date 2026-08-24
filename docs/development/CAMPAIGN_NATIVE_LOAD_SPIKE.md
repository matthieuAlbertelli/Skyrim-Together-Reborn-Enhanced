# Campaign native-load Slice 0 spike

> **Status:** human runtime-validated on 25 August 2026; production #56
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

The call is queued from CEF through `RunnerService` and runs while
`World::Update` drains the runner on the established game-update thread. The
native Boolean proves only that `Load_Impl` accepted/succeeded at its boundary;
it is never used as final recovery proof.

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
locked is the last required proof. Only the explicit validation release can
remove the gate and reset the terminal correlation.

A second request is rejected while validation, invocation, or proof is active,
and remains rejected after completion while the gate is locked. Terminal state
is cleared only by the explicit release command. There is no retry and no
fallback save. Failure reasons are bounded, including invalid identity,
unavailable/invalid artifact, native-save busy, validation rejection/failure,
gate-arm failure, unavailable native boundary, unrelated native load, native
rejection, missing post-load, missing gate/menu/pause/transport proof, timeout,
and internal failure.

Validation has a failure deadline only: 35 seconds for artifact validation and
60 seconds for post-load/safety proof. Elapsed time can never establish
success.

## Temporary development controls

Open the STRE overlay with F2 and enter the exact chat command:

After a cold client restart, a simple transport connection is not sufficient:
the prior admission and reconnect candidate are intentionally volatile. Request
the persisted campaign binding through the existing authoritative resume path:

```text
/stre-campaign-resume campaign-367760f49cba23fd72a5ad5013a75e1b
```

The harness validates the argument, queues `CampaignService::ResumeCampaign`
on `RunnerService`, and does not modify `CampaignClientAdmissionState`. The
cached binding is used only to build `CampaignResumeRequest`; the server must
accept the exact PlayerId, membership, and CharacterBinding before the existing
command-response handler creates admission. Missing binding, offline transport,
or server rejection leaves the client unadmitted. Wait for the existing
`server-validated admission accepted` record before requesting a native load.

Then enter the exact native-load command:

```text
/stre-native-load stre-checkpoint-4a33f050b434778db8b09094658831d5
```

Replace the example with the known checkpoint ID from the local #55 bundle.
The CEF bridge queues the request onto `RunnerService`; it never invokes Skyrim
from the CEF thread.

After a terminal success or failure, explicitly release/reset with:

```text
/stre-native-load-release
```

These commands are Slice 0 validation controls, not production recovery UX.
They remain intentionally available on this spike branch for subsequent #56
development and validation, but must not be treated as player-facing recovery
controls or proof that #56 is implemented.

All three runtime functions are declared once in
`CampaignNativeLoadBridge.h`. `TPProcess` iterates that shared manifest from
`ProcessHandler::OnContextCreated` and creates each function on the live
`skyrimtogether` CEF V8 object with the existing overlay handler. The native
`OverlayClient` compares incoming `ui-event` names against the same constants.
TypeScript declarations describe this contract but do not create the runtime
functions.

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

## Reproduction procedure

Do not perform this procedure until `TPTests`, `SkyrimTogetherClient`,
`TPProcess`, and the `SkyrimImmersiveLauncher` relink build successfully.

1. Build and stage the current branch:

   ```powershell
   xmake config -m releasedbg
   xmake -b -j 4 SkyrimTogetherClient
   xmake -b -j 4 TPProcess
   xmake -b -j 4 SkyrimImmersiveLauncher
   xmake install -o distrib
   ```

2. Deploy the staged `distrib` contents using the repository's normal mod
   deployment so the built executable and UI replace their installed
   counterparts under:

   ```text
   <Skyrim>\Data\SkyrimTogetherReborn\SkyrimTogether.exe
   <Skyrim>\Data\SkyrimTogetherReborn\TPProcess.exe
   <Skyrim>\Data\SkyrimTogetherReborn\UI\
   ```

   Launch that `SkyrimTogether.exe`, never `skse64_loader.exe`.

3. Set an explicit PowerShell path to the active Skyrim/MO2 save directory and
   the exact logical identity, then capture the immutable baseline:

   ```powershell
   $streSaveDir = 'C:\replace\with\the\active\Skyrim\save\directory'
   $streIdentity = 'stre-checkpoint-4a33f050b434778db8b09094658831d5'
   $streMembers = @(
     Join-Path $streSaveDir ($streIdentity + '.ess')
     Join-Path $streSaveDir ($streIdentity + '.skse')
   )
   $streBefore = $streMembers | ForEach-Object {
     $item = Get-Item -LiteralPath $_
     [pscustomobject]@{
       Path = $item.FullName
       Length = $item.Length
       LastWriteTimeUtc = $item.LastWriteTimeUtc
       SHA256 = (Get-FileHash -LiteralPath $_ -Algorithm SHA256).Hash
     }
   }
   $streBefore | Format-Table -AutoSize
   Test-Path -LiteralPath (Join-Path $streSaveDir ($streIdentity + '.ess.tmp'))
   ```

   Both members must exist, and the final command must print `False`.

4. Start Skyrim in a visibly different, later state and connect STRE normally.
   After a cold client restart, open F2 and first restore authoritative
   admission:

   ```text
   /stre-campaign-resume campaign-367760f49cba23fd72a5ad5013a75e1b
   ```

   Require this ordered evidence before continuing:

   ```text
   [STRE][CampaignNativeLoad] RESUME_QUEUED campaign=...
   [STRE][CampaignNativeLoad] RESUME_SENT campaign=... sent=true
   [STRE][CampaignAdmission] server-validated admission accepted operation=...
   ```

   A `sent=false`, missing cached binding, or server rejection must leave the
   client unadmitted. Do not invoke native load until server acceptance appears.

5. Enter the exact load command:

   ```text
   /stre-native-load stre-checkpoint-4a33f050b434778db8b09094658831d5
   ```

6. Stop immediately if Skyrim freezes/crashes during loading, loads another
   save, returns `NATIVE_RETURN success=false`, or does not emit `POST_LOAD`.
   Preserve the exact ordered log instead of adding another hook.

7. On success, verify visually that the checkpoint state loaded and that,
   while locked, movement, combat, interaction, pause/save/load, and console
   access are unavailable. Confirm F2/CEF remains usable and transport-update
   records continue.

8. While still locked, enter the same load command again. It must log a rejected
   request and must not emit a second `INVOKE` or `NATIVE_ENTER`.

9. Enter `/stre-native-load-release`. Confirm the guard disappears and normal
   gameplay resumes.

10. Recompute file evidence and compare every field:

   ```powershell
   $streAfter = $streMembers | ForEach-Object {
     $item = Get-Item -LiteralPath $_
     [pscustomobject]@{
       Path = $item.FullName
       Length = $item.Length
       LastWriteTimeUtc = $item.LastWriteTimeUtc
       SHA256 = (Get-FileHash -LiteralPath $_ -Algorithm SHA256).Hash
     }
   }
   Compare-Object $streBefore $streAfter -Property Path,Length,LastWriteTimeUtc,SHA256
   Test-Path -LiteralPath (Join-Path $streSaveDir ($streIdentity + '.ess.tmp'))
   ```

   `Compare-Object` must produce no rows and the temporary-file check must still
   print `False`.

11. Through Skyrim's ordinary load UI, load a different save without first
    issuing the managed command. It must load normally, produce no
    `CampaignNativeLoad NATIVE_ENTER`, acquire no STRE gate, and leave gameplay
    unpaused by STRE.

## Expected ordered evidence

Filter the client log for `CampaignNativeLoad|CampaignGate`. A successful run
must contain one ordered native-load trace (gate records may interleave):

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

The engine may order `NATIVE_RETURN` before `POST_LOAD`; both orders are
accepted, but all distinct milestones are mandatory. A duplicate while locked
must show `REQUEST_REJECTED reason=request-not-idle` with no second invocation.

## Current verdict

The deterministic native checkpoint-load primitive is **human
runtime-validated** for the exact evidence above. This verdict applies only to
the focused local primitive and managed gate; it does not claim #56 recovery
orchestration, collective rollback, server snapshot restoration, or durable
recovery completion.
