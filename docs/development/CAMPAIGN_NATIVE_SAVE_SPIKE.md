# Campaign native-save request spike

> **Status:** bounded #55 spike v2; automated validation covers identity and
> single-request state, while the Windows build covers hook integration. The v2
> path has not yet been validated in Skyrim. Native save completion, file
> resolution, fingerprinting, and checkpoint coordination remain unproved and
> unimplemented.

## Question

Can STRE accept one dedicated Skyrim save intent on its normal game-update path,
return to the caller immediately, and consume that intent only from Skyrim's
save/load processing boundary?

This spike deliberately stops before checkpoint protocol, acknowledgement,
fingerprinting, persistence coordination, recovery, retention, or save cleanup.

## Spike v1 runtime result

The first probe called `BGSSaveLoadManager::Save_Impl(2, 0, name)` directly from
the `RunnerService` task drained during the game update. Human validation on AE
1.6.1170 observed all of the following:

- Skyrim froze at the call;
- `stre-native-save-spike.ess.tmp` appeared with a size of zero bytes;
- no final `.ess` appeared;
- the post-call log was never reached.

This rejects the v1 architecture. The evidence establishes that the direct
game-update call did not return in that run; it does not by itself prove the
engine's internal deadlock mechanism.

## Audited request model

CommonLibSSE-NG exposes `BGSSaveLoadManager::Save(fileName)` as a synchronous
C++ wrapper around:

```cpp
bool BGSSaveLoadManager::Save_Impl(
    std::int32_t deviceId,
    std::uint32_t outputStats,
    const char* fileName);
```

That wrapper does not provide a deferred request boundary or a native completion
callback.

SKSE's `RequestSave` is an SKSE-owned abstraction, not a Bethesda-native engine
method. The audited SKSE implementation stores a boolean request flag and an
owned `std::string`. Its hook invokes the original Skyrim save/load processing
function first, then calls `Save` when the request flag is set. The v2 spike
copies that request/process separation while retaining its own single owned
request slot; it does not call or claim to implement SKSE's internal API.

For AE 1.6.1170, the checked Address Library database resolves:

- ID `35772` to RVA `0x6130B0`, the save/load process function used as the v2
  full-function hook boundary;
- ID `35727` to RVA `0x60FD40`, `BGSSaveLoadManager::Save_Impl`;
- ID `36564` to RVA `0x645EA0`, the main-loop function containing SKSE's call
  site.

The installed SKSE 2.2.6 DLL for AE 1.6.1170 was also inspected. Its save/load
hook calls module RVA `0x6130B0`, then, when its save-request state is set,
calls module RVA `0x60FD40` with arguments `(2, 0, name)`. That binary evidence
ties IDs `35772` and `35727` to the intended current runtime model. STRE hooks
the resolved full function at ID `35772`; it does not copy SKSE's historical
main-loop call-site offset.

The process function is modelled as:

```cpp
void BGSSaveLoadManager::ProcessEvents_Internal();
```

The exact Bethesda symbol name is not exported by the runtime and remains an
audited semantic label. The `Save_Impl` boolean's exact meaning is likewise not
documented by the cited sources and is not completion evidence.

## Spike v2 architecture

```text
F3 diagnostic
  -> RunnerService game-update task
     -> validate identity and store one owned Requested intent
        -> return immediately (no native save call)

later Skyrim save/load process boundary (Address Library ID 35772)
  -> enter STRE full-function hook
  -> call the original process function
  -> move Requested -> Processing
  -> call Save_Impl(2, 0, owned identity) (ID 35727)
  -> record only the literal native return
  -> move Processing -> Idle
```

The state machine is intentionally limited to `Idle`, `Requested`, and
`Processing`. Empty identities, invalid identities, unavailable relocations,
and a second request while one is active are rejected. It adds no checkpoint
protocol, server state, completion detector, retry loop, or fingerprinting.

The default diagnostic checkpoint ID is `native-save-spike-2`, which derives
the single native identity `stre-native-save-spike-2`.

## Threading and name lifetime

The request is accepted by the existing `RunnerService` game-update task. The
native call occurs only when Skyrim later enters the save/load process function.
The request slot owns a `std::string` from acceptance until the native call
returns, so no UI or task-local buffer crosses that boundary.

No sleep, timeout, polling, detached thread, background worker, arbitrary retry,
or frame-count delay is used. The acceptance and process-boundary thread IDs are
logged so runtime validation can confirm the actual relationship rather than
assuming it.

## Instrumentation contract

The successful path emits these ordered event names:

1. `REQUEST_ACCEPTED` with native identity and thread ID;
2. `PROCESS_BOUNDARY_ENTER` with native identity and thread ID;
3. `SAVE_CALL_ENTER` with native identity and thread ID;
4. `SAVE_CALL_RETURN` with native identity, literal `native_return`,
   `completion=unproven`, and thread ID;
5. `PROCESS_BOUNDARY_EXIT` with native identity and thread ID.

`SAVE_CALL_RETURN` means only that the native function returned. It does not
prove that a final `.ess` exists, that writes are closed or durable, or that an
SKSE cosave is complete.

## One-PC runtime validation still required

Build and deploy a non-master Windows client, then on one PC:

1. start Skyrim AE 1.6.1170 with SKSE 2.2.6 and load a playable native save;
2. press `F3`, open `Debuggers -> Campaign native save probe`, leave the default
   ID `native-save-spike-2`, and select `Queue dedicated native save` once;
3. verify the UI call returns immediately and Skyrim remains responsive;
4. capture the five ordered log events above, including every thread ID and the
   literal native boolean;
5. observe whether `stre-native-save-spike-2.ess.tmp` and the final
   `stre-native-save-spike-2.ess` appear, recording exact paths, sizes, and
   ordering relative to the logs;
6. record whether an SKSE cosave appears and its ordering, without treating it
   as success evidence.

Do not add a fixed wait or infer completion from elapsed time. Until a reliable
completion boundary is separately demonstrated, #55 must not send a success
acknowledgement or fingerprint the requested save.

## Remaining unknowns

- whether the v2 call returns and preserves game responsiveness on the target
  runtime;
- the meaning of the `Save_Impl` boolean;
- the final `.ess` path and exact `.tmp` to final-file lifecycle under the real
  Vortex/MO2 setup;
- whether an SKSE cosave is produced, required, and complete at any observable
  engine boundary;
- a reliable completion/failure boundary suitable for fingerprinting and a
  checkpoint acknowledgement;
- arbitration if another SKSE or engine save request is active in the same
  process pass.

## Sources

- [CommonLibSSE-NG `BGSSaveLoadManager.cpp`](https://github.com/CharmedBaryon/CommonLibSSE-NG/blob/main/src/RE/B/BGSSaveLoadManager.cpp)
  and
  [`BGSSaveLoadManager.h`](https://github.com/CharmedBaryon/CommonLibSSE-NG/blob/main/include/RE/B/BGSSaveLoadManager.h);
- [CommonLibSSE-NG `Offsets.h`](https://github.com/CharmedBaryon/CommonLibSSE-NG/blob/main/include/RE/Offsets.h);
- [SKSE `Hooks_SaveLoad.cpp`](https://github.com/ianpatt/skse64/blob/4c1b425415c15f4655c73abb4682023baeb99d48/skse64/Hooks_SaveLoad.cpp);
- [Address Library database symbol map](https://github.com/Exit-9B/AddressLibraryDatabase/blob/main/skyrimae.rename).
