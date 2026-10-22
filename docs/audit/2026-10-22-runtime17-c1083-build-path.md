# Runtime17TwoPc — MSVC C1083 output-path correction

Date: 2026-10-22. Refs #99. Experimental branch `feat/skyrim-1.7-runtime`.
This records compiler/build evidence, not deployment or runtime acceptance.

## Original failure and execution boundary

The maintainer's receipt `20261022T014200Z-a9950dfd2300456b91d5f845fd3e3d73`
under the private `runtime17-dev-runs` directory records:

- source `b617b9dca2c20e2146d7fc340fdad29eeb2d8d71`;
- `status=STOPPED`, `failedPhase=BUILDING`, `candidate=null`, `preflight=null`;
- `xmake build failed: -1`, at `SEMemoryBlock.cpp` in SkyrimImmersiveLauncher;
- no distribution, snapshot, backup, staging journal or per-PC success receipt.

The configured working directory was the experimental `_audit/rt17/wt`
worktree. Its retained `.xmake` configuration selected
`../../runtime17-dev-runs/<receipt>/build`. The helper invokes, from that worktree,
`xmake config -P . -m debug -y -o <receipt>/build`, then
`xmake build -P . -y -j 6`. Build failure throws before install, content snapshot,
destination preflight or staging. The build log and receipt establish that this
workflow execution did not reach staging; they do not establish the global state
of either installation or exclude unrelated human activity.

All **73 original failure files** were inventoried before diagnosis and verified
unchanged by size/SHA-256 afterward. Reproduction used a new, equally long output
directory. No original build artifact was reused or overwritten. No B access.

## Controlled reproduction and cause

Installed xmake: `3.0.9+HEAD.2b184e178`. Its local help supports `-vD` and `--files`.
The targeted reproduction was:

```powershell
xmake build -P . -vD -y -j 1 --files=Code/immersive_launcher/script_extender/SEMemoryBlock.cpp SkyrimImmersiveLauncher
```

Compiler: `C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Tools\MSVC\14.44.35207\bin\HostX64\x64\cl.exe`.
Executable file version: **19.44.35228.0** (product 14.44.35228.0).
The new long output reproduced the identical C1083, with this `/Fo` equivalent:

```text
-Fo..\..\runtime17-dev-runs\20261022T020029Z-fdb14874f9e54a3a99ba12ace8e167fa\build\.objs\SkyrimImmersiveLauncher\windows\x64\debug\Code\immersive_launcher\script_extender\SEMemoryBlock.cpp.obj
```

The raw relative value is 190 characters. Its resolved absolute path is 253;
concatenating the 76-character working directory and the relative spelling before
collapsing `..` gives **267**. The `/Fd` resolves to 188 characters and is nonempty.
This unit uses no `/Fp`. Full builds additionally exercise PCH `/Fp` outputs.
The compiler command has no empty output argument; the empty filename shown by
C1083 is not evidence of an empty `/Fo`.

Direct compiler comparisons used the captured argv/toolchain environment, same
source/options/PDB/dependency path, and changed **only `/Fo`**:

| `/Fo` variant | Result |
|---|---|
| Original relative spelling, resolved length 253, concatenated length 267 | FAIL, exit 1, C1083 |
| The very same output file passed as an absolute path, length 253 | PASS, exit 0 |
| New short output with spaces, length 78 | PASS, exit 0 |
| Additional absolute output lengths 201, 240, 248, 249, 250, 253, 259 | All PASS |

This confirms a compiler output-path problem involving the long **relative
spelling**, not a blanket 253-character absolute-path limit. Xmake's local config
implementation rewrites absolute builddir input through `path.relative`; the long
receipt-based build consequently produces the problematic `..` spelling. The
tested short root produces short absolute compiler outputs. No assertion about
an undocumented internal MSVC temporary filename is needed.

Both output directories existed and passed an exclusive write-probe check.
Paths with spaces succeed when passed as one argument. The diagnostic environment
uses `TEMP=TMP=C:\Users\matth\AppData\Local\Temp`; xmake temporaries and
`/sourceDependencies` JSON are under `.xmake/261022` there. The targeted captured
command has 53 arguments, 2509 rendered characters and **no response file**.
Six available response files from the verbose full native build were archived
separately. The older successful lot-4 object path is 201 characters, with its
original artifact retained; its build used the same configure/build pattern.

## Minimal workflow change

Code commit: **`d2fee200399af846a41c06657b0901920b125a13`**.

- Optional private `buildRoot` selects a short ordinary local directory. The
  maintained configuration now explicitly uses `C:\STRE-R17`; the previous JSON
  is retained privately and destination strings are unchanged.
- `New-R17BuildLayout` creates a unique GUID directory; the compiler builddir
  must be at most 100 characters, allowing room for current object/PCH suffixes.
- `build-layout.json`, `receipt.json.build` and the reverse `receipt-link.json`
  record receipt, source root, commit, builddir and physical directory identity.
  `build-commands.json` records argv and working directory.
- `Invoke-R17Build` uses the linked short builddir. `New-R17Candidate` verifies
  BuildInfo there; retained older candidates still use their original header
  location. Root isolation, local-path, reparse and identity checks remain.
- Recovery messages distinguish pre-staging build/preparation failure from a
  partial or completed staging. A failed build requests correction and a **new
  execution**, not Verify/RestoreStaging on a nonexistent candidate.

The receipt/distribution stays in its documentary location; no source checkout
was moved. No Windows registry, ACL, antivirus, Visual Studio installation,
native gameplay, ABI, voice or CK content change was made. Existing installed-A
content provenance, scoped replacement, INI/Better Grabbing policy and personal
data protection are unchanged. The personal entry command is unchanged.

## Executed validation

**45 workflow fixture cases PASS.** The 42 retained cases still cover provenance,
snapshot stability, divergent contents, access errors, hardlinks, preservation and
recovery. New cases cover short space-containing roots, unique build allocation,
bidirectional links and source-commit mismatch, overlong/nonlocal root rejection,
and an actual workflow BUILDING exception with no copies or restoration advice.
The existing xmake-routing case checks the new builddir and retained packaging
directory. These fixture builds are simulated; the following native builds are not.

1. Long-path xmake unit reproduction: FAIL as expected.
2. `/Fo`-only comparisons and short-root xmake unit build: PASS.
3. Fresh verbose full native build at source `b617b9dc`: PASS, including linking
   the launcher/client, STServer, server runner, EarlyLoad and TPProcess.
4. **The committed corrected helper itself**, on clean `d2fee200`, was then
   invoked only for build/packaging into a new local receipt. Its source and
   submodules passed checks before/after. Configure/build/install all succeeded;
   install wrote exclusively to that receipt's `distrib`, not either game.
   `New-R17Candidate` and exact distribution verification passed for **293
   native/UI files**. BuildInfo and client/server embedded identities both equal
   `runtime17-d2fee200399af846a41c06657b0901920b125a13`.

The validated compiler directory is
`C:\STRE-R17\9475c547a15d4d59a45c824924e6a6a1\build`.
The first complete short-tree inventory found 678 objects (longest path 177),
5 PCHs (143), 38 PDBs (101), and 7 generated headers (125); dependency paths top
out at 179. Required final executable/DLL paths are below 100 characters. Counts
are observations, not future constraints. Existing compiler warnings remain;
there are no C1083 or link errors in the successful builds. No executable was
launched to infer runtime or multiplayer acceptance.

Private evidence: `_audit/runtime17-c1083/` in the experimental worktree.
It retains failed-file inventories, original config, verbose/unit comparisons,
exact compiler argv/environment, response files, path inventories and
`tests-01/results.json`. `validated-build/validation.json` records
`NATIVE_BUILD_PACKAGE_PASS`; its native manifest, build layout, commands and log
identify the **compiled** commit, separately from later documentation commits.
No content snapshot, staging or actual-Data verification was invoked.

## Correct resumption and stop boundary

Keep the original failed receipt unchanged. It has no candidate to Verify and
no staging writes to restore. The maintainer can start a fresh execution with:

```powershell
pwsh -NoProfile -File .\build-and-deploy-dev.ps1 -Runtime17TwoPc
```

That future command allocates a new build/receipt, builds the current clean HEAD,
captures installed A content and reaches the existing human Vortex handoff only
after its checks. The compiler-only validation artifacts are not reused as an
already-deployed candidate. See the [workflow procedure](../development/RUNTIME_1_7_TWO_PC_DEV.md).

**STOP before deployment.** No Pc-vero access, real staging/deployment, game/server,
Steam/Vortex/CK/xEdit launch, CK import, push, PR, manual CI or merge occurred.
