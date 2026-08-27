# Campaign native-save completion spike

> **Status:** the deferred v2 save-request path and bounded v3 completion proof
> are human-validated in Skyrim. V3 was exercised through the production #55
> two-PC checkpoint flow on 24 August 2026, including native path resolution,
> two-member hashing, and successful load of the generated save. Remaining live
> resilience validation is tracked by
> [#72](https://github.com/matthieuAlbertelli/Skyrim-Together-Reborn-Enhanced/issues/72);
> recovery, retention, and cleanup remain out of scope.

## Question

When is one requested STRE native save fully complete and safe to fingerprint?

The answer must cover the recoverable local bundle, not merely a native boolean
or the existence of one `.ess`. Production checkpoint orchestration now consumes
this primitive through issue #55 and remains documented separately.

## Spike v1 — rejected direct call

The first probe called `BGSSaveLoadManager::Save_Impl(2, 0, name)` directly from
the `RunnerService` task drained during the game update. Human validation on AE
1.6.1170 observed:

- Skyrim froze at the call;
- `stre-native-save-spike.ess.tmp` appeared with a size of zero bytes;
- no final `.ess` appeared;
- the post-call log was never reached.

This rejects the v1 architecture. It proves that the direct call did not return
in that run; it does not prove the engine's internal deadlock mechanism.

## Spike v2 — validated deferred request

V2 stores one owned request from the STRE game-update task, returns immediately,
then consumes it after Skyrim's original save/load process function at Address
Library ID `35772`. It calls `Save_Impl` at ID `35727` only from that process
boundary.

The production multiplayer save policy also hooks `Save_Impl` at ID `35727`.
Its audited CommonLibSSE-NG signature and every local wrapper use `(self, int32
deviceId, uint32 outputStats, const char* fileName)`; `Load_Impl`'s different
filename-first order is not reused. The policy classifies safely readable
case-insensitive Skyrim name families `Save*`, `QuickSave*`, and `AutoSave*`,
and treats every null, unreadable, empty, unterminated, or different family as
unknown. The deferred managed call passes through the same hook under a scoped
thread-local provenance guard. The guard, never the `stre-` filename, prevents
recursive policy entry.

The first live CampaignSavePolicy run on AE 1.6.1170 observed `fileName ==
nullptr` for both Manual Save and QuickSave despite that correct ABI. This
means the native UI request supplied no filename at this boundary; it was not a
misread `deviceId` or `outputStats` caused by a copied `Load_Impl` signature.
Those attempts correctly remained Unknown and were blocked fail-closed. The
hook now logs both scalar parameters, pointer present/null, a bounded name only
when safely readable, and the resulting classification so a subsequent live
run can establish the remaining native request behavior without inventing a
fallback. The apparently blocked autosave is not yet classification evidence.

### Upstream save-origin observation — live evidence and native-chain audit

The initial observation surface did not change `CampaignSavePolicy` or transmit
an origin to it. Every trace line carries one process-wide monotonic `sequence`,
the current STRE `frame`, and the Windows `thread`. `Save_Impl` additionally
records the audited scalar arguments, safe filename observation, current
classification, internal #55 provenance, and diagnostic
`processBoundaryDepth`. The depth remains only a call-stack marker around the
already-hooked ID `35772` boundary; it is not a save-intent store and is never
consulted by policy.

The first ordered AE 1.6.1170 run proved that one F5 produced multiple
`MenuControls::ProcessEvent(Quicksave)` observations on several threads over
frames 6760 through 6766, followed by
`Save_Impl(4, 0, nullptr)` on thread 32044 at frame 6766 with
`processBoundaryDepth=1`. Static audit of the exact installed executable and
Address Library closes the intermediate chain:

1. `QuickSaveLoadHandler::ProcessButton`, Address Library ID `52251`, is the
   exact `MenuEventHandler::ProcessButton(ButtonEvent*)` override. It accepts
   only a non-zero button value with `heldDownSecs == 0`, checks that no UI is
   pausing the game, and applies the native Quick-save eligibility check at ID
   `35734`. Repeated/held/released input dispatches therefore explain the
   multiple broad `Quicksave` observations; only the first actionable press
   can continue.
2. The accepted Quick path calls ID `35769` with native operation code
   `0xF0000200`. That function allocates a 24-byte
   `bgs::saveload::Request`, whose only correlatable fields are its intrusive
   refcount, operation code at offset `0xC`, and request state at offset
   `0x10`. There is no native operation identifier. Because this code is signed
   negative, the request is pushed into the manager's
   `BSTCommonStaticMessageQueue<...Request..., 8>` consumed by ID `35772`;
   it is not sent to the manager's asynchronous save/load thread queue.
3. ID `35772` pops the same request, decrements its initial state, coalesces
   duplicate low-24-bit operation kinds, and performs the native screenshot/UI
   readiness steps. Its `0xF0000200` branch then calls exactly
   `Save_Impl(4, 0, nullptr)`. The observed `deviceId=4` is an output of this
   proven semantic branch, not a general-purpose origin mapping.

An ineligible or non-actionable F5 creates no request. A queue-full push can
fail. Once queued, the process loop can requeue while native readiness is not
satisfied, or consume/coalesce an operation without reaching `Save_Impl`.
Consequently request creation alone is not treated as execution. The request
pointer is useful only as a bounded live-trace correlation while that native
object exists; it is neither stable nor persisted.

The same live run proved that a real new-slot Manual Save emitted
`JournalOpened`, then `Save_Impl(2, 0, nullptr)` 293 frames later on thread
32044 with `processBoundaryDepth=0`, then `JournalClosed`. No
`MenuControls::NewSave` occurred, invalidating that candidate. The exact
manual chain is:

1. `UI::MenuOpenCloseEvent` remains context only. The save/load panel registers
   its Scaleform callbacks in ID `52902`; its `SAVE` callback selects the panel
   mode but does not write a save.
2. The public `FxDelegateHandler::CallbackFn(const FxDelegateArgs&)` callback
   named `SaveGame`, adapter ID `52915` and body ID `52923`, is the actual
   confirmed operation seam. Its numeric selection argument is `0` for a new
   slot and non-zero for an existing slot.
3. New-slot selection calls `Save_Impl(2, 0, nullptr)` directly and
   synchronously on the callback thread. Existing-slot selection resolves the
   selected `BGSSaveLoadFileEntry` and tail-calls ID `35533`, which then invokes
   `Save_Impl`. The new-slot path has no intermediate native request object or
   request queue. Cancellation before confirmation never invokes `SaveGame`;
   failure inside `Save_Impl` remains an executed but failed operation.

The follow-up client instrumentation is bounded to these proven seams:

- the exact Quick `ProcessButton` logs entry/exit and whether the event is the
  first actionable press;
- ID `35769` logs the native operation code at request creation;
- the exact public static-request-queue `PushInternal` and `PopInternal`
  virtual slots log only real operations, including queue, request pointer,
  operation code, state, frame, and thread. Failed pops are intentionally not
  logged. This supplies causal pointer correlation without a time window;
- the exact `SaveGame` callback logs entry/exit, argument count, and
  new/existing selection before the synchronous native path.

ID `35772` is called from the regular update path even when its queue is empty,
so unconditional entry/exit logging there would be unbounded. A successful
process-queue pop plus the existing `processBoundaryDepth=1` at `Save_Impl`
provides the narrower causal proof.

A second ordered live run closed the transport question for Quick and Manual
NewSlot. The one actionable Quick handler call created operation `0xF0000200`;
the exact same request address traversed its pushes/requeues and final pop
immediately before `Save_Impl`. Production now tags only that exact pointer
while the actionable handler is synchronously creating it. A successful pop of
the correlated object arms one thread-local, one-shot Quick proof. A requeue
returns the same object to `Queued`; a failed push, mismatched operation, later
pop, process-boundary exit without `Save_Impl`, or successful consumption drops
the proof. There is no device-ID mapping, elapsed-time window, persisted token,
or inference from the broad repeated input events.

The same run proved that `SaveGame(NewSlot)` enters, invokes `Save_Impl`, and
exits in the same frame on the same thread. Production therefore scopes one
thread-local Manual proof around that callback only. The proof is one-shot and
is cleared on callback exit. `ExistingSlot` remains observation-only because
its distinct resolved-entry path has not been validated live.

Auto remains unclassified. Static audit identifies several native producers of
operation `0xF0000040`, whose ID `35772` branch calls
`Save_Impl(3, 0, nullptr)`, but neither that operation code nor `deviceId=3` is
accepted as Auto provenance without a deterministic live trigger. The next run
must use a vanilla Save-on-Wait action and correlate its exact upstream source,
queue operation, and `Save_Impl`. Auto and all otherwise unproven operations
remain Unknown/fail-closed. `CampaignSavePolicy` consumes the proven Quick or
Manual transport when present and otherwise retains filename classification;
the separate scoped #55 internal provenance retains priority and an
`stre-*` filename alone grants nothing. The transports are automated-tested,
client-build-tested, and live validated end-to-end for Quick and Manual NewSlot.
Manual ExistingSlot and Auto remain explicitly unproved and fail-closed.

Human validation on AE 1.6.1170 with SKSE 2.2.6 proved:

- the request and save-processing callbacks ran on different threads;
- Skyrim remained responsive;
- `Save_Impl` returned `true`;
- `stre-native-save-spike-2.ess` and
  `stre-native-save-spike-2.skse` were produced;
- the `.ess` loaded successfully in Skyrim;
- the ordered v2 logs reached `PROCESS_BOUNDARY_EXIT`.

This validates the request/deferred-process architecture and relocation. It does
not assign completion semantics to the native boolean.

## Actual Bethesda/SKSE save order

The audited SKSE v2.2.6 order is:

```text
STRE calls BGSSaveLoadManager::Save_Impl
  -> SKSE SaveGame_Hook runs inside the Bethesda save path
     -> Serialization::SetSaveName(name)
     -> dispatch kMessage_SaveGame              (pre-save notification)
     -> call the original Bethesda save target
        -> SkyrimVM SaveGlobalData hook
           -> original Papyrus/global-data save
           -> SKSE HandleSaveGlobalData
              -> create/write .skse
              -> close .skse synchronously
     -> clear SKSE save name
  -> remaining Bethesda Save_Impl work
  -> Save_Impl returns bool
```

Consequences for `SAVE_CALL_RETURN`:

- it does not, from the available public contract, prove that the `.ess` write
  is complete;
- it does not prove that the final `.ess` handle is closed;
- it does not prove that `.ess.tmp` has been renamed;
- SKSE source does prove that its own cosave writer has called `Close()` before
  this return, when cosave creation reached that code;
- it does not prove that the whole recoverable bundle exists and is readable.

`kMessage_SaveGame` is dispatched before the original save target and is not a
post-save notification. SKSE exposes no later save-completion message. STRE is
also loaded through `StartSKSE`, not `SKSEPlugin_Load`, so it does not receive an
`SKSEMessagingInterface`. CommonLibSSE-NG exposes incomplete/undocumented save
event types but no event with a proven post-rename, post-close bundle contract.
STRE's existing TES dispatcher exposes `TESLoadGameEvent`, not a corresponding
validated save-completion event.

The `.skse` member is required for this bundle because it contains the
serialization state of installed SKSE plugins. Omitting it could restore the
`.ess` while silently losing mod-owned character/runtime state.

## Spike v3 — completion proof

Because no native or SKSE post-save signal has proven whole-bundle semantics,
v3 uses the minimal filesystem fallback after `Save_Impl` returns `true`:

1. before the native call, resolve the exact final `.ess` path and require the
   `.ess`, sibling `.skse`, and `.ess.tmp` targets not to exist;
2. after the native return, move to `AwaitingCompletion` and enqueue one job on
   a managed background worker;
3. observe until `.ess.tmp` is absent and both required final members can be
   opened simultaneously with `GENERIC_READ` and only `FILE_SHARE_READ`;
4. keep both handles open while reading and hashing every byte;
5. require non-zero sizes and unchanged handle sizes across each hash;
6. only then publish the bundle artifact and `Completed`.

Opening with only `FILE_SHARE_READ` fails while an existing handle has write or
delete access and prevents new write/delete handles while STRE fingerprints the
files. Combined with fresh targets and absent `.ess.tmp`, this proves that the
new requested bundle is immutable for the complete fingerprint operation. It
does not infer completion from elapsed time or existence alone.

The worker observes every 100 ms only to re-evaluate the proof. The 30-second
deadline is exclusively a fail-closed upper bound: reaching it produces
`Failed`, never probable success. No wait, hash, or filesystem loop runs on a
game or engine save-processing thread.

## Save-directory resolution

STRE does not hardcode `Documents\My Games\Skyrim Special Edition\Saves`.
It resolves `stre-<CheckpointId>.ess` with Skyrim's
`BSWin32SaveDataSystemUtility::PrepareFileSavePath` using Address Library AE ID
`109278` (RVA `0x152E130` in the installed 1.6.1170 database), with the final
path and INI/profile handling enabled. The `.skse` path is the canonical sibling
obtained by replacing only the final extension.

The resolved path must be absolute and its filename must match the requested
logical identity. Absolute paths remain private implementation details and are
not logged or encoded in metadata.

## Bundle, fingerprint, and metadata

The required v3 bundle has exactly two canonical roles:

```text
NativeSaveBundle
  LogicalIdentity = stre-<CheckpointId>
  Members[ess]  = { size, SHA-256(file bytes) }
  Members[skse] = { size, SHA-256(file bytes) }
```

Members are sorted by role (`ess=1`, `skse=2`). Missing, duplicate, unknown, or
zero-size members are rejected.

Metadata codec v1 is deterministic, little-endian, path-independent, and
bounded to 256 bytes:

```text
8 bytes   magic "STRENSB1"
u16       codec version = 1
u16       logical identity byte length
bytes     logical identity
u8        member count = 2
repeated in canonical role order:
  u8      role
  u64     byte size
  32 bytes SHA-256 of member bytes
```

The bundle fingerprint is SHA-256 over the exact codec-v1 metadata bytes. Its
contract is therefore:

- `FingerprintAlgorithm = "SHA-256"`;
- `FingerprintVersion = 1`;
- `Fingerprint = SHA256(SaveMetadata)`;
- `SaveMetadataCodecVersion = 1`;
- `SaveMetadata =` the canonical manifest above.

This maps directly to the existing `CheckpointSlotRecord` fields without a
schema change. The production #55 client now persists this artifact locally
before acknowledgement and the server records it against the canonical slot.

## Lifecycle and failure behavior

The thread-safe local lifecycle is:

```text
Idle/Completed/Failed -> Requested -> Processing
Processing -- Save_Impl true --> AwaitingCompletion -> Completed
Processing -- failure -------------------------------> Failed
AwaitingCompletion -- proof/timeout failure ---------> Failed
```

A new request may replace only a terminal `Completed` or `Failed` result. A
second active request is rejected. Failures include unavailable relocations,
path/identity mismatch, pre-existing targets, `Save_Impl=false`, missing or
unopenable required members, read/hash/codec errors, state mismatches, and the
bounded deadline. V3 never deletes incomplete or failed files.

## Instrumentation

The success trace extends v2 with:

```text
REQUEST_ACCEPTED
PROCESS_BOUNDARY_ENTER
SAVE_CALL_ENTER
SAVE_CALL_RETURN native_return=true completion=unproven
SAVE_COMPLETION_AWAITING
PROCESS_BOUNDARY_EXIT
SAVE_COMPLETION_OBSERVED
SAVE_MEMBER_RESOLVED role=ess size=...
SAVE_MEMBER_HASHED role=ess sha256=...
SAVE_MEMBER_RESOLVED role=skse size=...
SAVE_MEMBER_HASHED role=skse sha256=...
REQUEST_COMPLETED
```

Logs contain the logical identity, roles, sizes, hashes, evidence, and thread
IDs, but not full filesystem paths. Any terminal error emits `REQUEST_FAILED`
with a bounded reason.

## Runtime validation evidence — 2026-08-24

The production #55 network flow automatically invoked v3 on two real Skyrim
clients for checkpoint
`checkpoint-4a33f050b434778db8b09094658831d5`, using the shared logical identity
`stre-checkpoint-4a33f050b434778db8b09094658831d5`.
Both clients produced the exact files
`stre-checkpoint-4a33f050b434778db8b09094658831d5.ess` and
`stre-checkpoint-4a33f050b434778db8b09094658831d5.skse`.

Skyrim remained responsive. `SAVE_CALL_RETURN` continued to report
`completion=unproven`; acceptance occurred only after `.ess.tmp` was absent,
the required `.ess` and `.skse` were available together under the implemented
no-write/delete sharing proof, their sizes remained stable while every byte was
hashed, and the canonical two-member artifact was built. The generated save was
successfully loaded in Skyrim.

PC 1 evidence:

- `.ess`: 2,600,863 bytes, SHA-256
  `8ac74662c3ac18f599c36690253907465326ad721b5bce5d357176f0f83e6123`;
- `.skse`: 2,789 bytes, SHA-256
  `3fc8ea1291be750871f23094e93723bc964edf3e7c7cfe20b45d4d51033403cf`;
- global bundle fingerprint:
  `df3375f2a880046c219edd91b3249c176a788c37756d9da5e9f23573c5f1ea45`;
- `SAVE_RESULT_SENT success=true sent=true`.

PC 2 evidence:

- `.ess`: 2,953,963 bytes, SHA-256
  `16e64497f4ba16e8573a76bd8819d2e89ccad61720792d8402d61d4345a4c4c0`;
- `.skse`: 4,900 bytes, SHA-256
  `c0151b7dd840b6023a9c98c38bd8b0c06fb7383a1f638bf74d552715b0fdbc9a`;
- global bundle fingerprint:
  `19f95c49fa39ba16084e37a243199456c583fcbf04950a3086e78a3d07d61c51`;
- `SAVE_RESULT_SENT success=true sent=true`.

Independent PowerShell `Get-FileHash -Algorithm SHA256` results matched every
STRE per-member hash exactly. The distinct global fingerprints are expected:
each roster member owns a different native Skyrim bundle even though both share
the coordinated checkpoint identity and server commit boundary.

## Remaining unproved work

- live local-artifact persistence and exact no-overwrite replay after an
  acknowledgement loss;
- live client failure or disconnect while checkpointing;
- live server interruption before and after the commit boundary;
- collective recovery and restore, owned by #56;
- retention, pruning, cleanup, and save upload, all outside this spike.

The three #55 resilience scenarios are tracked by
[#72](https://github.com/matthieuAlbertelli/Skyrim-Together-Reborn-Enhanced/issues/72).

See
[`CAMPAIGN_COORDINATED_CHECKPOINTS.md`](CAMPAIGN_COORDINATED_CHECKPOINTS.md)
for the production integration contract and nominal two-PC evidence.

## Sources

- [SKSE v2.2.6 `Hooks_SaveLoad.cpp`](https://github.com/ianpatt/skse64/blob/v2.2.6/skse64/Hooks_SaveLoad.cpp)
- [SKSE v2.2.6 `Hooks_Papyrus.cpp`](https://github.com/ianpatt/skse64/blob/v2.2.6/skse64/Hooks_Papyrus.cpp)
- [SKSE v2.2.6 `Serialization.cpp`](https://github.com/ianpatt/skse64/blob/v2.2.6/skse64/Serialization.cpp)
- [SKSE v2.2.6 `PluginAPI.h`](https://github.com/ianpatt/skse64/blob/v2.2.6/skse64/PluginAPI.h)
- [CommonLibSSE-NG `BGSSaveLoadManager`](https://github.com/CharmedBaryon/CommonLibSSE-NG/blob/main/include/RE/B/BGSSaveLoadManager.h)
- [CommonLibSSE-NG `BSSaveDataSystemUtility`](https://github.com/CharmedBaryon/CommonLibSSE-NG/blob/main/include/RE/B/BSSaveDataSystemUtility.h)
- [CommonLibSSE-NG `BSInputDeviceManager`](https://github.com/CharmedBaryon/CommonLibSSE-NG/blob/main/include/RE/B/BSInputDeviceManager.h)
- [CommonLibSSE-NG `InputEvent`](https://github.com/CharmedBaryon/CommonLibSSE-NG/blob/main/include/RE/I/InputEvent.h)
- [CommonLibSSE-NG `UserEvents`](https://github.com/CharmedBaryon/CommonLibSSE-NG/blob/main/include/RE/U/UserEvents.h)
- [CommonLibSSE-NG `MenuControls`](https://github.com/CharmedBaryon/CommonLibSSE-NG/blob/main/include/RE/M/MenuControls.h)
- [CommonLibSSE-NG `MenuEventHandler`](https://github.com/CharmedBaryon/CommonLibSSE-NG/blob/main/include/RE/M/MenuEventHandler.h)
- [CommonLibSSE-NG `BSTMessageQueue`](https://github.com/CharmedBaryon/CommonLibSSE-NG/blob/main/include/RE/B/BSTMessageQueue.h)
- [CommonLibSSE-NG `bgs::saveload::Request`](https://github.com/CharmedBaryon/CommonLibSSE-NG/blob/main/include/RE/R/Request.h)
- [CommonLibSSE-NG `FxDelegateArgs`](https://github.com/CharmedBaryon/CommonLibSSE-NG/blob/main/include/RE/F/FxDelegateArgs.h)
- [CommonLibSSE-NG `Journal_SystemTab`](https://github.com/CharmedBaryon/CommonLibSSE-NG/blob/main/include/RE/J/Journal_SystemTab.h)
- [CommonLibSSE-NG `UISaveLoadManager`](https://github.com/CharmedBaryon/CommonLibSSE-NG/blob/main/include/RE/U/UISaveLoadManager.h)
- [CommonLibSSE-NG Address Library IDs](https://github.com/CharmedBaryon/CommonLibSSE-NG/blob/main/include/RE/Offsets.h)
