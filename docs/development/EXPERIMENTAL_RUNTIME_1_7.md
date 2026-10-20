# Steam 1.7.104 migration and local DEV procedure

This is the current native build/deployment procedure for
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
The maintainer selected 1.7.104.0 as the sole future development target;
1.6 support is no longer maintained. [AGENTS.md](../../AGENTS.md) and the
[CK runbook](MODDING_TOOLCHAIN.md) require 1.7 tool requalification before authoring.
Existing ABI guards and historical release evidence are not widened/relabelled.

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
The xmake `after_install` callback copies runtime dependencies/UI into the
explicit distribution directory. The ordinary `build-and-deploy-dev.ps1` entry
is retained with an explicit local-only worktree route, described below.

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

## Authorized environment and local DEV routing

The resumed lot 4 authorizes the normal Windows account, current Steam 1.7.104
installation, Vortex staging/configuration and normal Documents/AppData/HKCU
writes. No second account, parallel Steam installation or full game copy is
required. The earlier lot-4 isolation stop is historical and superseded by this
authorization. Only the main-PC main menu is in scope; no server, gameplay,
campaign load, Pc-vero, push, PR, manual CI, merge or release.

Inspect the actual local DEV script and every called script before running it.
The historical script has a hardcoded primary checkout, CK auto-import and
remote deployment. A native-only local wrapper must select the experimental
worktree, pass `-P .` to every xmake command and never enter that remote/import
path. Local ignored scripts are not versioned project changes.

The resumed local entry uses explicit `Runtime17RepositoryRoot`,
`Runtime17ReceiptRoot`, `Runtime17ExpectedCommit` and `Runtime17Phase`
(`Build` then `Deploy`) parameters. It compiles/installs to a fresh receipt
directory, verifies the branch/commit and embedded client/server identity, and
copies only its new native/UI/dependency payload to the usual STRE Vortex
staging. It backs up old runtime files and unlinks destination hardlinks before
copying new bytes. It preserves content, saves, logs/cache and server state/config.
The exact local wrapper and script diff are retained with the dated receipt;
they must be reviewed again if the local workflow changes.

Before any import/deployment, compare installed CK-owned and tracked content.
Never auto-import differences. An unsatisfied byte-level CK import/compile gate
must remain reported as such. A separate native-only build may proceed when
the existing ESP/PEX/runtime content is verified and all content is preserved;
this is not CK authoring validation. Keep backups of divergent authoring files.

Use SKSE 2.3.1 and the exact qualified 1.7.104 Address Library. Inventory native
plugins and root proxies. Disable the inspected incompatible Better Grabbing
1.17, CCExtender and CKPE winhttp proxy through Vortex for the first menu test.
Better Grabbing 2.0 remains a later World Sync candidate; do not weaken the
server's required-plugin policy. Do not edit hardlinked plugin bytes in place.

After Vortex deploys the staged candidate, verify every payload hash at the
actual `Data/SkyrimTogetherReborn` destination, plus runtime/SKSE/AL and absence
of incompatible DLLs. Launch the official `SkyrimTogether.exe`, observe logs,
menu entries and keyboard/mouse navigation, and quit normally. Do not select
New/Load/Continue. Diagnose a startup failure before a focused correction and
retest; stop on memory-corruption symptoms, wrong destination or uncontrolled
writes. Ask only for required human observations if GUI access is unavailable.

Use the feature-owned protocols for the later human lot:
[startup/menu](../features/main-menu/TEST_PLAN.md#steam-171040-experimental-lot),
[creation/Valen/checkpoints](../features/alternate-start/TEST_PLAN.md#steam-171040-experimental-lot),
and [World Sync](../features/world-sync/TEST_PLAN.md#steam-171040-experimental-lot).
Do not run those gameplay scenarios while the native-capability gate rejects
startup. Record blocked, failed, not run and human PASS separately.

For the first authorized lot, use the
[main-menu-only gate](../features/main-menu/TEST_PLAN.md#first-main-menu-only-runtime-gate)
and stop after normal exit and evidence collection. The following wider sequence
requires later maintainer authorization after startup acceptance:

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
