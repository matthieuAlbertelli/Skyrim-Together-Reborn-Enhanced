# Building STRE

> **Status:** audited build entry points, not yet a fully reproducible clean-machine guide.

## Prerequisites

The repository currently expects a Windows C++ development environment compatible with the upstream Skyrim Together Reborn build, plus:

- xmake `3.0.0` or compatible;
- a Visual Studio toolchain capable of C++20/x64 builds;
- Git with initialized submodules;
- Node.js and `pnpm` for `Code/skyrim_ui`;
- the game/runtime prerequisites documented by Skyrim Together Reborn.

The upstream technical build guide remains the authoritative prerequisite reference until STRE validates its own clean-machine matrix:

`https://wiki.tiltedphoques.com/tilted-online/technical-documentation/build-guide`

## Checkout

```powershell
git clone --recurse-submodules <STRE repository URL>
cd "Skyrim Together Reborn Enhanced"
git submodule update --init --recursive
```

## Generate Visual Studio projects

```powershell
xmake project -k vsxmake
```

The repository also contains `MakeVSLatestProjects.cmd` for the upstream Visual Studio generation flow.

## Build and install native targets

```powershell
xmake config -m releasedbg
xmake -y
xmake install -o distrib
```

`Build.bat` currently automates project generation, native compilation, installation into `distrib`, and a production UI build. Treat it as a convenience script rather than the only documented build contract.

## Build the Skyrim UI

```powershell
cd Code\skyrim_ui
pnpm install
pnpm deploy:production
```

The generated UI is written under `Code/skyrim_ui/dist`. Local deployment scripts and generated outputs must not be committed.

## Tests

Steam 1.7.104.0 is the sole development target. Follow the
[local DEV build/deployment procedure](EXPERIMENTAL_RUNTIME_1_7.md): select the
experimental worktree explicitly, pass `-P .` to xmake, preserve earlier outputs,
and verify compiled/deployed identities. Initial menu-only authorization is
historical; STATUS records later scoped acceptance. The standard build commands
above do not depend on the branch name. The separate Runtime17TwoPc helper accepts
only `feat/skyrim-1.7-runtime` or clean `main` exactly equal to an explicitly
fetched `origin/main`, with the existing provenance checks. Merging this port
does not retarget the private entry/configuration. See its
[source and receipt rules](RUNTIME_1_7_TWO_PC_DEV.md#source-branches-and-retained-receipts)
and [main selection procedure](RUNTIME_1_7_TWO_PC_DEV.md#selecting-main-after-integration).
Main support is fixture-tested; no real main deployment is implied.
Production CK/AutoMod authoring remains separately gated; a native build must
not silently import changed Data.

For CK/AutoMod authoring, official xEdit validation and the existing local import/
deployment boundary, follow [the modding toolchain runbook](MODDING_TOOLCHAIN.md).
Its fixture checks run with `pwsh -NoProfile -File Tools/Scripts/Test-StreModding.ps1`.

The C++ tests are defined under `Code/tests` and included by the root xmake project. Run the relevant xmake test target(s) for your configured environment and include the exact command/output in the PR until a single canonical test command is established in CI.

## Known documentation gaps

Before a broad contributor launch, the project still needs:

- a validated clean-machine prerequisite matrix;
- exact compiler/SDK versions;
- a canonical one-command test flow;
- documented game deployment and launch steps;
- CI parity with the local release build.

Track this work under roadmap milestone R0.
