# Two-PC DEV workflow: native build and installed main-PC contents

This procedure owns deployment mechanics for issue #99.
[STATUS](../project/STATUS.md) owns executed validation; the
[migration guide](EXPERIMENTAL_RUNTIME_1_7.md) owns native compatibility.
The fixture validation, native-build evidence and later maintainer-operated
installation receipts are distinguished in STATUS. Pc-vero remains
**NON AUDITÉ PAR CODEX** directly; reading its retained local verification receipt
is not a new remote audit. The maintainer owns its readiness, real execution and
both manual Vortex operations. No incompatibility is inferred from that boundary.

## Sources and scope

**Contenu STRE installé sur le PC principal → remplacement sur Pc-vero**, avec
contrôle des empreintes après synchronisation. The same frozen candidate feeds
A's staging. An initial destination difference is normal, not a hash failure.

| Category | Deployment source | Treatment |
|---|---|---|
| Native client, launcher, server, CEF libraries, generated UI | Fresh build of clean `feat/skyrim-1.7-runtime` | Validate compiled identity and snapshot distribution |
| Managed STRE contents, including Valen WAVs | A's configured `gameRoot/Data` at capture time | Independent snapshot, replace/create only listed paths on A/B |
| Server/personal configuration, saves/co-saves, campaigns, identities, databases, logs/caches, credentials | Each machine | Preserve; never distribute or include in package backups |
| Skyrim, SKSE, Address Library, Better Grabbing | External dependency installation | Verify pinned hashes; never distribute |

The pinned reference's `contentFiles.path` list defines the exact content scope.
It is not a mirror of Data or of every mod. Old content hashes and
`repositoryBytes/repositorySha256` are **historical evidence**, not expected
capture bytes. The tracked `GameFiles/Skyrim` path inventory must still agree
with the scope. Scope/dependency changes require review and a newly pinned
reference. Copying installed content does not import or promote it into Git,
authorize CK authoring, or invoke the historical CK collector/importer.
The historical 293 native and 914 content files are not fixed candidate counts.

Content categories are independently fenced. The two shipped
`STRE/MainMenu/{presentation,localization}.ini` files are presentation assets,
distinct from server/personal configuration. Native output owns
`SkyrimTogetherReborn/` in the combined Data-relative manifest; contents cannot
enter that namespace or dependency paths. Duplicate paths, traversal, alternate
streams, reparses and category collisions block. Files outside the new manifest,
**including old UI bundles**, remain untouched: no purge or global equality.

## Branch boundary before main integration

The current helper deliberately requires source branch `feat/skyrim-1.7-runtime`
and validates that branch in retained candidate provenance. A source checkout on
`main` is rejected with `Wrong source branch.` even after the migration merges.
The ordinary xmake/CI build path has no such restriction. Keep this experimental
worktree and private routing intact until a separate, reviewed adaptation is
authorized; merging alone does not make the two-PC command a main-source workflow.
That adaptation must retain repository/commit/submodule checks and the ability
to verify or restore historical receipts. Do not just delete the branch guard,
relabel old receipts or weaken content/import policy. No workflow behavior is
changed by this integration-review note.

## Entry and private configuration

The ignored personal entry routes `-Runtime17TwoPc` to the experimental worktree's
`Tools/Scripts/Invoke-StreRuntime17Dev.ps1` and `Stre.Runtime17Dev.psm1`.
The historical body remains behind `-Legacy`, which is not a 1.7 procedure.
Ambiguous no-mode invocation fails. Run from the personal entry's directory:

```powershell
pwsh -NoProfile -File .\build-and-deploy-dev.ps1 -Runtime17TwoPc
```

Use PowerShell 7.2+, Git and the existing xmake/MSVC/Node/pnpm build environment.
The private config and entry remain ignored. `runtime17-dev.example.json` and
`Start-StreRuntime17Dev.example.ps1` are generic templates for another machine.

| Field | Meaning |
|---|---|
| `repository` | Absolute local root of `feat/skyrim-1.7-runtime`, not the parent checkout |
| `receipts` | Existing local directory outside the source worktree and destinations |
| `buildRoot` | Existing short local build root, e.g. `C:\STRE-R17`, separate from source, receipts and destinations |
| `referencePath`, `referenceSha256` | Qualification/scope manifest and pinned SHA-256 |
| `targets` | Ordered A,B; A is the local installed-content source |
| `stagingRoot` | Existing STRE Vortex mod root, containing content and `SkyrimTogetherReborn` |
| `gameRoot` | Actual Skyrim root with `SkyrimSE.exe` and `Data`, never a staging alias |

The maintainer supplied B's staging and installed-game shares separately. Values
can be read without resolving them; preparation does not probe or change shares.
At real execution, directories must be accessible, non-overlapping ordinary
directories with distinct physical IDs. A is local; B readiness is human-attested.
The process guard enumerates local processes only. No remote process execution.

The private reference retains schema 1, runtime `1.7.104.0`, `nativeFiles`
(runtime-relative), `contentFiles` (Data-relative), `dependencies`
(game-root-relative), and provenance. Rows have `path`, `bytes`, `sha256`.
Its hash and external dependency hashes remain blocking checks. Back up this
reference, the private configuration and entry together; do not regenerate the
reference from a divergent destination to bypass a failure.

Each new build allocates `buildRoot/<GUID>/build`; the complete build-directory
path is limited to 100 characters as a conservative budget for this project's
object/PCH/dependency suffixes. The root must be ordinary, writable and local;
UNC/mapped-network roots, overlap and physical aliases remain rejected. It is
not a renamed/moved source checkout. If `buildRoot` is omitted, it defaults to
`STRE-R17` at the root of the repository's drive; prepare that directory first.

`build-layout.json` in the receipt and `receipt-link.json` in the allocated build
root record both directions, the repository, source commit and directory ID.
`receipt.json.build` retains that layout, and `build-commands.json` records argv
and working directory. Do not discard the short build directory while retaining
the candidate: verification needs its generated BuildInfo. Existing pre-fix
receipts without a layout still read their original `<receipt>/build` header;
new compilation requires an explicit layout and cannot silently use that path.

## Server configuration preservation

The canonical path is **`SkyrimTogetherReborn/config/STServer.ini`**, as used by
`DediRunner::LoadSettings`; a root-level INI is not a substitute.

- Installed `Data/SkyrimTogetherReborn/config/STServer.ini` is required to verify
  `sRequiredNativePlugins=BetterGrabbing.dll`. Absence blocks.
- The staging copy is optional. Initial absence is recorded with a null hash.
  If present, its policy and hash are checked. Installed configuration remains required.
- Verification checks unchanged presence/hashes. INI contents and credentials
  are never printed, copied, created or backed up. Do not launch a server or
  create/copy a default INI to bypass the check.
- Only explicit file/directory-not-found means absence. Access denial, IO errors,
  non-directory ancestors and reparses remain blocking, including for an optional
  INI. Managed-file inspection also distinguishes absence from unreadability.

## Run sequence

1. Read configuration. `-WhatIf` stops here: no destination resolution/IO,
   process enumeration, compilation or writes.
2. Check source root/origin/branch, clean source and submodules, gitlink commits
   and origins, runtime/SKSE/build-identity contract, and local output isolation.
   No fetch. Optional `-Runtime17ExpectedCommit <full-SHA>` pins HEAD. At `READY`,
   confirm both PCs' game, launcher, server, Vortex, Steam, CK and xEdit are closed
   and remain closed through staging.
3. Create a unique receipt directory and linked short build directory. In the selected worktree run
   `xmake config -P . -m debug -y -o <fresh-build>`,
   `xmake build -P . -y -j <jobs>`, then
   `xmake install -P . -o <fresh-distrib>`. Capture errors/logs and recheck source.
   BuildInfo and embedded client/server IDs must identify that commit. Validate
   native/CEF/UI requirements; exclude tests/debug artifacts. Objects, PCHs, PDBs,
   generated headers and linked outputs use the short build tree. Installation
   packaging still writes only `<receipt>/distrib`, never either actual game.
4. **After xmake completes**, freeze the combined candidate under `package/`.
   Native/UI output goes under `package/SkyrimTogetherReborn/`. Capture only the
   listed contents from A's installed Data: relative path, source path, bytes,
   SHA-256, `installed main PC` provenance. New files are independent, never
   hardlinks; source handles deny write/delete sharing while copying. Check
   source hashes/timestamps across capture and snapshot hashes. Missing,
   unreadable or changed sources block without Git fallback. Source Data is not
   written. `manifest.json` records the schema-2 candidate and capture time.
5. Preflight both destinations: physical isolation, external dependencies,
   qualified plugin set, no old proxy, protected INIs. Record staging and Data's
   state for each manifest path: `identical`, `replace` or `create`. Initial
   staging/Data differences are allowed. Bad paths/access/IO still block.
6. Back up both PCs' scoped staging and installed files independently under
   `backup/A|B/staging|installed`, including recorded absence. Recheck sources
   and backup hashes before changing either target. Both old sets matter when
   staging differs from Data.
7. Recheck frozen-package integrity before each target. Copy only manifest paths
   to staging. Leave identical files alone; unlink replacements before copying
   to new files, safely breaking Vortex hardlinks. Check preflight state before
   replacement and frozen hashes after copy. Journal per-file progress and any
   partial-copy bytes. No out-of-scope purge.
8. At `AWAITING_HUMAN_VORTEX`, deploy STRE DEV manually through Vortex on **both**
   PCs, close Vortex, then enter `DEPLOYED`. Do not launch games/server.
9. Verify the retained package, both stagings, both actual Data trees (manifest
   paths only), dependencies and unchanged INIs. Write `verified-A.json` and
   `verified-B.json`. Only both passing yields `INSTALLED_VERIFIED`. Distinguish
   compiled commit from later verification HEAD. No runtime/admission/World Sync
   or multiplayer validation follows from these hashes.

The inspected xmake client install hook copies CEF/Discord and generated Angular
output to the **fresh distribution**, not installed Data. It does not copy
`GameFiles/Skyrim`. Snapshot assembly follows xmake; no later Git-content pass can
overwrite it. Namespace fences make source-category collisions explicit.

## Failure, verification and restoration

Keep the receipt directory: `receipt.json`, `manifest.json`, build log/distribution,
independent `package/`, backups, `backup-A|B.json`, `copy-A|B.json`,
`staged-A|B.json` and verification results as phases finish. `failedPhase` names
the stop. Journals distinguish pending/unlinked/identical/verified/failed files,
including observed partial hashes. A success does not imply B success. Verification
attempts have an `attemptId`; an old per-PC receipt is not a result for a newer
failed attempt. There is no cross-PC transaction or automatic deployment retry.

An error in BUILDING, snapshot preparation, destination preflight or backup occurs
before staging writes: preserve its receipt, correct that failure and start a
new execution. Do not use Verify/RestoreStaging for a failed build with no
candidate. An error during staging instead requires inspection of per-PC copy
journals and guarded restoration; completed staging can resume with Verify.

If staging succeeded but Vortex was deferred/failed, finish the manual deployment,
then verify **the same frozen candidate**, without rebuilding:

```powershell
pwsh -NoProfile -File .\build-and-deploy-dev.ps1 -Runtime17TwoPc -Runtime17Mode Verify -Runtime17ReceiptRoot '<receipt-directory>'
```

If staging failed partway, Verify cannot finish that copy. Restore, redeploy and
verify the old state, then start a new build/capture when ready:

```powershell
pwsh -NoProfile -File .\build-and-deploy-dev.ps1 -Runtime17TwoPc -Runtime17Mode RestoreStaging -Runtime17ReceiptRoot '<receipt-directory>'
```

RestoreStaging validates both backup sets and destination identities before either
write, rejecting unknown intervening changes. It accepts recorded old/new bytes,
absence and precisely journaled partial-copy bytes. It prepares each PC's
**pre-run installed backup** in that PC's staging with unlink/copy, including
removing in-scope paths originally absent from Data. Restoring an initially
divergent staging would not restore the prior installation after Vortex. The
original staging backup remains available as evidence. Out-of-scope files and
configuration stay untouched. Dirty current source does not prevent rollback.

Deploy the restored stagings manually through Vortex on both PCs, close it, then:

```powershell
pwsh -NoProfile -File .\build-and-deploy-dev.ps1 -Runtime17TwoPc -Runtime17Mode VerifyRestored -Runtime17ReceiptRoot '<receipt-directory>'
```

Only `RESTORED_VERIFIED` establishes that scoped installed files match their
pre-run installed backups. The helper never writes Data. Unavailable B, corrupted
backups, unknown changes or changed directory identities block recovery. Keep the
private config unchanged: receipts pin its hash. Schema-2 receipts are required;
older native-only schema-1 receipts are explicitly rejected, not reinterpreted.
A failure before complete backups/staging has no target writes to undo; inspect
its phase before choosing recovery. Unrelated external changes are not undone.

## Local validation

```powershell
pwsh -NoProfile -File Tools/Scripts/Test-StreRuntime17Dev.ps1 -OutputRoot '<fresh-local-fixture-directory>'
```

Use a fresh **absolute local path with a short prefix**, for example a unique
directory beneath the local temporary folder. Deep worktree paths can exceed
Windows hardlink path limits in the synthetic voice-file case. This test-output
choice does not change the real deployment configuration or relax its checks.

Only local fake repositories/installations are used. Compiler, process checks and
human Vortex steps are simulated; hashes, files, hardlinks, backups and restoration
are real local operations. Cases cover installed-vs-Git WAV provenance, divergent
or absent destinations, missing/unstable sources, corrupt snapshot, copy/final-hash
failures, partial recovery, personal-file preservation, INI behavior and routing.
See [dated evidence](../audit/2026-10-22-runtime-1.7-two-pc-workflow.md).
The subsequent [C1083 diagnosis and real native-build validation](../audit/2026-10-22-runtime17-c1083-build-path.md)
records the short-build-root correction, 45 passing fixture cases and the exact
compiled commit. Native build acceptance is separate from deployment/runtime acceptance.
