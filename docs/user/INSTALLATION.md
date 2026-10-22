# Installing STRE for Skyrim 1.7 development

Skyrim Special Edition **Steam 1.7.104.0** is the sole future development target.
The migration remains on `feat/skyrim-1.7-runtime` until validated. This is a
development setup, not an assertion that an existing public release supports 1.7.
[STATUS](../project/STATUS.md) owns executed results; the
[compatibility matrix](../testing/COMPATIBILITY_MATRIX.md) distinguishes target
versions from tested scenarios. The [historical alpha guide](../audit/2026-10-21-installation-0.4.0-alpha.1.md)
preserves 0.4.0-alpha.1 / 1.6 instructions and must not be applied as a 1.7 guide.

## Dependencies

- Windows x64 and the complete Steam 1.7.104.0 game/data installation.
- SKSE64 **2.3.1**, including `skse64_1_7_104.dll` beside `SkyrimSE.exe`.
- Matching Address Library `Data/SKSE/Plugins/versionlib-1-7-104-0.bin`.
- STRE native binaries, UI/CEF dependencies and content from the identified
  development build. Do not overlay a separate upstream STR client.
- Existing external content prerequisites, including EEKs Fireplace Resource,
  remain separately installed; STRE does not redistribute those assets.

The inspected Better Grabbing 1.17, CCExtender and CKPE `winhttp.dll` remain
excluded from the 1.7 baseline. **Better Grabbing 2.0** has separate provenance,
SKSE-loading and Solo-manipulation PASS evidence in STATUS. Multiplayer World
Sync manipulation/interruption remains unqualified; keep the existing server
policy requiring `BetterGrabbing.dll`. Dependency installation and mod-manager
deployment are maintainer operations, not implicit actions of this guide.
AutoMod/Papyrus CLI fixtures, an xEdit fixture cycle and read-only CK inspection
have bounded qualification. Production authoring/conversion is not authorized
by those results; follow the [modding runbook](../development/MODDING_TOOLCHAIN.md).

## Local DEV deployment and launch

Follow [the runtime procedure](../development/EXPERIMENTAL_RUNTIME_1_7.md) and
[build guide](../development/BUILDING.md). Confirm the selected worktree,
compiled commit and every deployed file hash in the actual game directory.
Vortex staging alone is not deployment proof. Preserve existing content that
differs from the branch; do not automatically import Data into Git.

For a separately authorized human run, use the official project launcher:

```text
<Skyrim>/Data/SkyrimTogetherReborn/SkyrimTogether.exe
```

Select the installed `SkyrimSE.exe` when needed. The launcher starts the matching
SKSE DLL; do not use `skse64_loader.exe` as the STRE launch path.
The initial menu-only smoke is historical. Scoped Solo, two-player and STRE
presentation A-D acceptance is recorded in STATUS; this guide does not request
their repetition. Presentation now has an exact 1.7.104 adapter. The common
package contains the background/branding; the private intro remains excluded
until music redistribution rights are established. Missing intro proceeds to
the usable menu/background, and presentation failure retains vanilla fallback.
The [two-PC DEV procedure](../development/RUNTIME_1_7_TWO_PC_DEV.md) documents
the separate, guarded maintainer-operated workflow and its current branch limit.

## Evidence and later validation

Retain `tp_client.log`, `cef_debug.log`, the profile's SKSE log, process exit and
any crash dump, along with source/binary hashes and visual observations.
An active process or compiled candidate is not a usable-menu PASS. Diagnose a
failure before retrying. Broader Solo/Valen matrices, multiplayer World Sync and
coordinated recovery require their own explicit test scope; accepted saving and
loading is not crash/disconnect recovery evidence. No 1.6 save/campaign migration
is established, and this development guide is not public-release qualification.
