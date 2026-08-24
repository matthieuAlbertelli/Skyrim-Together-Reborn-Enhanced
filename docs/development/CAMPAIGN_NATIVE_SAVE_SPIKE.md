# Campaign native-save completion spike

> **Status:** the deferred v2 save-request path is human-validated on Skyrim AE
> 1.6.1170 with SKSE 2.2.6. The bounded v3 completion observer, two-member
> bundle model, SHA-256 fingerprint, and metadata codec are implemented and
> automated/build-tested, but have not yet been validated in Skyrim. Checkpoint
> protocol, acknowledgement, coordination, recovery, retention, and cleanup
> remain unimplemented.

## Question

When is one requested STRE native save fully complete and safe to fingerprint?

The answer must cover the recoverable local bundle, not merely a native boolean
or the existence of one `.ess`. This spike deliberately stops before server
checkpoint orchestration.

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
schema change. V3 does not send or persist the artifact yet.

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

## One-PC runtime validation required

Use the fresh default checkpoint ID `native-save-spike-3`, producing logical
identity `stre-native-save-spike-3`:

1. identify the real save directory used by the active vanilla/MO2/Vortex
   profile and confirm no v3 targets already exist;
2. deploy a non-master client, start AE 1.6.1170 with SKSE 2.2.6, and load a
   playable save;
3. press `F3`, open `Debuggers -> Campaign native save probe`, and select
   `Queue dedicated native save` once;
4. verify responsiveness and the ordered logs above;
5. verify that `REQUEST_COMPLETED` appears only after both member hashes;
6. compare logged sizes and per-member hashes with PowerShell;
7. load `stre-native-save-spike-3.ess` manually in Skyrim.

PowerShell inspection, after replacing the path with the exact active profile
save directory:

```powershell
$saveDir = 'C:\exact\active\profile\save-directory'
$names = @(
    'stre-native-save-spike-3.ess',
    'stre-native-save-spike-3.skse'
)
$members = Get-ChildItem -LiteralPath $saveDir -File |
    Where-Object { $_.Name -in $names } |
    Sort-Object Name
$members | Select-Object Name, Length, LastWriteTimeUtc
$members | Get-FileHash -Algorithm SHA256 |
    Select-Object Path, Hash
Get-ChildItem -LiteralPath $saveDir -Filter 'stre-native-save-spike-3*' |
    Select-Object Name, Length, LastWriteTimeUtc
```

## Remaining unproved work

- in-game validation of the native path resolver call and v3 sharing proof;
- observed v3 log ordering and size/hash agreement on the target profile;
- persistence/restart retention of the local artifact;
- idempotent retry behavior after an acknowledgement loss;
- server request/acknowledgement and Candidate-to-Committed coordination;
- collective recovery and restore, owned by #56;
- retention, pruning, cleanup, and save upload, all outside this spike.

## Sources

- [SKSE v2.2.6 `Hooks_SaveLoad.cpp`](https://github.com/ianpatt/skse64/blob/v2.2.6/skse64/Hooks_SaveLoad.cpp)
- [SKSE v2.2.6 `Hooks_Papyrus.cpp`](https://github.com/ianpatt/skse64/blob/v2.2.6/skse64/Hooks_Papyrus.cpp)
- [SKSE v2.2.6 `Serialization.cpp`](https://github.com/ianpatt/skse64/blob/v2.2.6/skse64/Serialization.cpp)
- [SKSE v2.2.6 `PluginAPI.h`](https://github.com/ianpatt/skse64/blob/v2.2.6/skse64/PluginAPI.h)
- [CommonLibSSE-NG `BGSSaveLoadManager`](https://github.com/CharmedBaryon/CommonLibSSE-NG/blob/main/include/RE/B/BGSSaveLoadManager.h)
- [CommonLibSSE-NG `BSSaveDataSystemUtility`](https://github.com/CharmedBaryon/CommonLibSSE-NG/blob/main/include/RE/B/BSSaveDataSystemUtility.h)
- [CommonLibSSE-NG Address Library IDs](https://github.com/CharmedBaryon/CommonLibSSE-NG/blob/main/include/RE/Offsets.h)
