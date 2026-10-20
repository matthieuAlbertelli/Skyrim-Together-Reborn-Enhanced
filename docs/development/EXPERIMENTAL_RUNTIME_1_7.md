# Native Steam 1.7.104 experiment

This is the build and future environment procedure for
[issue #99](https://github.com/matthieuAlbertelli/Skyrim-Together-Reborn-Enhanced/issues/99).
[STATUS](../project/STATUS.md) owns executed results and the
[compatibility matrix](../testing/COMPATIBILITY_MATRIX.md) owns support claims.
The dated [lot-1 audit](../audit/2026-10-20-runtime-1.7.104.md) and
[lot-2 audit](../audit/2026-10-20-runtime-1.7.104-lot2.md) record provenance;
the [lot-3 audit](../audit/2026-10-20-runtime-1.7.104-lot3.md) adds exact binary qualification.
The [native contract register](RUNTIME_1_7_NATIVE_CONTRACTS.md) owns the finite
admission conditions and supersedes the original broad blocker classification.

## Scope and fail-closed boundary

`feat/skyrim-1.7-runtime` targets **Steam 1.7.104.0 only**. A format-2 Address
Library reader is retained for parser coverage, not dual-runtime admission.
The maintainer's native experiment does not change
[AGENTS.md](../../AGENTS.md), the [CK runbook](MODDING_TOOLCHAIN.md), or their
1.6.1170 content-authoring restrictions.

The bootstrap contract is: exact executable version -> bounded PE/store
inspection -> Address Library -> exact SKSE dependency -> required STRE native
capabilities -> path routing/Steam loading -> map image -> startup hook ->
STRE initializers/hook commit -> SKSE startup hooks -> game entry.
The loader's own form-allocation sentinel runs during image mapping, so checking
the version only inside `RunTiltedInit` is too late.

An incompatible version/store, missing or invalid Address Library, missing or
incompatible SKSE, and unavailable STRE native capability have distinct
diagnostics. PE/store inspection requires a Steam API import and a `.bind` entry
section; GOG/EOS evidence or an unknown executable fails closed. This conservative
identification is not a cryptographic executable attestation.

`CampaignAbiVerified` is derived from the complete native contract table without
an INI/debug override. It stays false while any mandatory condition lacks its
required source/binary evidence. Partial source evidence or an AL entry cannot
replace a required target-binary proof. The contract register distinguishes
inherited hooks, STRE additions, optional features and diagnostics. No fake success, partial campaign
activation, server authority change or partial-roster continuation is permitted.
Optional presentation may retain vanilla fallback once gameplay is admissible.

Lot 3 closes all 18 static prerequisites on the exact inspected bytes, so this
derived gate is now open. Runtime acceptance is still absent. The gate is not
a per-launch SHA-256 allowlist: compare the EXE/SKSE/AL to the lot-3 receipt
before preparing a smoke, and requalify any difference. Passing version checks
alone does not extend this binary evidence to patched or different images.

Address Library parsing bounds the file at 32 MiB and the table at 1,048,576
slots/records, checks module/version/pointer width, all reads, compression
arithmetic, duplicate IDs, nonzero/in-image offsets, trailing data and required
bootstrap IDs. Sparse format-5 zeros mean absent IDs. Failure clears exposed
maps/version/base; cached `VersionDbPtr` addresses are invalidated by generation.
The four explicit bootstrap IDs are a minimum, not a complete initializer
coverage claim. Loading/reloading is a startup operation, not thread-safe live
database replacement.

`IsScriptExtenderLoaded()` retains its existing wire meaning of DLL presence.
It does not attest plugin initialization. `StartSKSE` installation logs separately;
the existing native plugin enumeration and Better Grabbing admission policy
remain authoritative for those checks.

## Building without installation

Use the isolated checkout and explicitly pass `-P .`, especially in a nested
worktree. Use a shell without a profile that changes directories. Inspect
`xmake.lua`, target callbacks and `package.json` before changing this procedure.

```powershell
xmake config -P . -m debug -y
xmake build -P . -y TPTests
./build/windows/x64/debug/TPTests.exe "[runtime-port]" --reporter compact
./build/windows/x64/debug/TPTests.exe --reporter compact
python -m unittest discover -s Tools/Scripts -p 'test_*.py'
xmake build -P . -y -j 2 SkyrimImmersiveLauncher
xmake build -P . -y -j 2 SkyrimServerRunner
xmake build -P . -y -j 2 TPProcess
git diff --check
```

When retaining an earlier candidate, choose a new ordinary output directory
with `xmake config -P . -m debug -y -o <new-absolute-build-directory>`.
Generated BuildInfo/BranchInfo include paths honor that selection. Inspect it
before building, retain new logs/manifest, and never overwrite earlier receipts
or reuse their binary hashes. Lot 2 uses `_audit/rt17-lot2/build`;
lot 3 uses `_audit/rt17-lot3/build`.

`SkyrimTogetherClient` alone is a static library. The actual client executable is
`SkyrimTogether.exe` from `SkyrimImmersiveLauncher`, with `EarlyLoad.dll` and
`TPProcess.exe`; server outputs are `STServer.dll` and `SkyrimTogetherServer.exe`.
The client build's `deploy:production` npm script only runs Angular `ng build`
into the checkout's `Code/skyrim_ui/dist`. It does not deploy to a game.
The xmake `after_install` callback copies runtime dependencies/UI; do not invoke
it in this mission. Never invoke `build-and-deploy-dev.ps1`.

The tracked UI lockfile has version 6. With an incompatible pnpm default, use
`corepack pnpm@8.15.9 install --frozen-lockfile` inside `Code/skyrim_ui`, with
`COREPACK_ENABLE_AUTO_PIN=0`, rather than rewriting the lockfile. Node 20 is
the current build reference. This is dependency installation inside the worktree.

Commit the reviewed source before final configuration/build. `BUILD_COMMIT`
embeds `runtime17-` plus the full Git commit; client and server still require
exact equality. CommonLib enables `build.always_update_configfiles`: Git HEAD
is not a normal template dependency, and ordinary reconfiguration alone can
otherwise retain an old SHA. Verify generated `build/BuildInfo.h` against
`git rev-parse HEAD` before retaining binaries (use the selected builddir's
`BuildInfo.h` with `-o`). A later documentation-only commit does not retroactively
rename the compiled source commit. Record both commits, mode/toolchain,
commands/exits, UI output and binary SHA-256 in ignored audit receipts.
Unchanged historical `GIT_TAG` alone is not candidate identity.

## Future isolated environment, not executed by this mission

The following preparation and game runs require a later authorization. A Git
branch or Vortex profile is not evidence of filesystem isolation.
The maintainer has already deliberately updated the installed Steam game and
activated SKSE. Preserve that state; it does not itself isolate saves, plugins,
server state or test outputs.

1. Provision a complete, legitimately obtained Steam **1.7.104.0** installation
   with matching data/masters in a separate environment. Prefer a dedicated
   Windows user or test machine with its own Documents and AppData. Do not copy
   only a newer EXE into 1.6 data, update the stable Steam installation to obtain
   files, or hardlink the experiment to stable files.
2. Inventory EXE/data identity, Steam app/depot identity, root proxy DLLs, SKSE
   and every native plugin. Resolve symlinks/junctions and inspect hardlink
   counts for the install, staging and profile paths. Reject any link back to
   stable game, Vortex, distribution or Pc-vero destinations.
3. Install the official [SKSE64 2.3.1](https://skse.silverlock.org/) dependency
   matching 1.7.104 (DLL `skse64_1_7_104.dll`, file resource `0.2.3.1`) and
   [Address Library v13](https://www.nexusmods.com/skyrimspecialedition/mods/32444?tab=files)
   with `versionlib-1-7-104-0.bin`. Verify files, not just archive names.
   [Better Grabbing 2.0](https://www.nexusmods.com/skyrimspecialedition/mods/134769?tab=description)
   advertises Skyrim 1.7 support; this is a candidate for World Sync validation,
   not proof of STRE interoperability. Preserve its required-plugin policy.
4. Use the normal required STRE content and separately owned resources, copied
   without authoring/conversion. Do not migrate ESP/PSC/PEX/voices/catalogues or
   run CK/AutoMod. Optional SkyUI, CCExtender, proxy loaders, and other native
   mods need their own exact-target evidence; do not infer it from SKSE loading.
   Begin with the smallest coherent dependency set and the vanilla Journal.
5. Resolve `Documents/My Games/Skyrim Special Edition`, INI `sLocalSavePath`,
   SKSE logs/co-saves, `%LOCALAPPDATA%/Skyrim Special Edition` plugin/load-order
   files, launcher-selected EXE paths, CEF data/cache, and each process's working
   directory. Separate all of them from the stable user profile. Preserve the
   full isolated save profile, including per-player STRE bindings/sidecars.
6. Give the test server its own working directory, configuration, port and
   `state/stre-server.sqlite3`; verify any `StatePath` setting. Never point it at
   an existing database. Keep client and server source build identities equal.
7. Stage only in a new explicitly named experiment directory after reviewing
   every copy's resolved source/destination and lack of links. The ordinary local
   build/deploy script references the primary checkout, installed Data, local
   Vortex staging and `\\Pc-vero\STRE-Dev-Initial`; it is unsuitable here.
8. Start a **new** solo save and a **new** fixed-roster test campaign. Resuming
   1.6 saves/campaigns is a separate migration subject with no guarantee.
   Preserve ADR-0018 throughout: full sealed roster, persistent server authority,
   recovery lock, one committed checkpoint, each player's own `.ess + .skse`.

Use the feature-owned protocols for the later human lot:
[startup/menu](../features/main-menu/TEST_PLAN.md#steam-171040-experimental-lot),
[creation/Valen/checkpoints](../features/alternate-start/TEST_PLAN.md#steam-171040-experimental-lot),
and [World Sync](../features/world-sync/TEST_PLAN.md#steam-171040-experimental-lot).
Do not run those gameplay scenarios while the native-capability gate rejects
startup. Record blocked, failed, not run and human PASS separately.

For the first authorized lot, advance one stage at a time:

1. Record exact candidate/source and EXE/SKSE/AL/plugin hashes, then test
   startup ordering and plugin initialization without loading an existing save.
2. On fresh isolated saves, compare ordinary manual load, quick load and
   Continue. Deliberately make the remembered target differ from the first
   native-list entry; record the copied canonical identity and actual loaded save.
3. Run new solo and two-player creation in both completion orders, capturing
   final actor/base identities, retirement/destructor observations, private-base
   lifetime and local/remote seating. A delete request is not retirement PASS.
4. Exercise Valen, required-plugin/Better Grabbing behavior and pause/input/
   CEF/network coexistence; preserve vanilla fallback for disabled presentation.
5. Commit a new full-roster checkpoint, disconnect/restart, then collectively
   recover each player's own native save plus shared server state.

Stop on a wrong save target, native exception, stale/reused binding, duplicate
retirement, lost guard menu or incomplete roster. None of these stages was run
by lot 3; no deployment or launch follows automatically from static admission.
