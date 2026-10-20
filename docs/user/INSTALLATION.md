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

For the first main-menu-only test, disable incompatible optional native plugins
through the mod manager and deploy. The inspected Better Grabbing 1.17,
CCExtender and CKPE `winhttp.dll` are excluded from this baseline. Better
Grabbing 2.0 is a candidate for later World Sync qualification, not a validated
STRE dependency here. The server's required-plugin policy remains unchanged.
Do not author or convert plugins/Papyrus with CK/AutoMod until that toolchain
has been requalified for 1.7.

## Local DEV deployment and launch

Follow [the runtime procedure](../development/EXPERIMENTAL_RUNTIME_1_7.md) and
[build guide](../development/BUILDING.md). Confirm the selected worktree,
compiled commit and every deployed file hash in the actual game directory.
Vortex staging alone is not deployment proof. Preserve existing content that
differs from the branch; do not automatically import Data into Git.

The maintainer's current Windows account, Steam installation, Vortex staging
and normal Documents/AppData/HKCU writes are authorized for this bounded test.
Launch the official project launcher:

```text
<Skyrim>/Data/SkyrimTogetherReborn/SkyrimTogether.exe
```

Select the installed `SkyrimSE.exe` when needed. The launcher starts the matching
SKSE DLL; do not use `skse64_loader.exe` as the STRE launch path.
Observe native/SKSE/CEF initialization, menu entries, keyboard/mouse navigation
and normal Quit. Never select New, Load or Continue in this first test.
No server, existing campaign or second-PC deployment is included.
Custom menu presentation may remain disabled by its historical 1.6 guard;
record the vanilla fallback separately from engine startup acceptance.

## Evidence and later validation

Retain `tp_client.log`, `cef_debug.log`, the profile's SKSE log, process exit and
any crash dump, along with source/binary hashes and visual observations.
An active process or compiled candidate is not a usable-menu PASS. Diagnose a
failure before retrying. Solo play, World Sync, Valen and coordinated recovery
require later explicit test scope; no 1.6 save/campaign migration is established.
