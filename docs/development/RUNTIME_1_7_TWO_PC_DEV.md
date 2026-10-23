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
| Native client, launcher, server, CEF libraries, generated UI | Fresh build of the explicitly selected clean development or verified `main` checkout | Validate compiled identity and snapshot distribution |
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

## Source branches and retained receipts

Attached **`main`** and work branches using the repository's `feat/`, `fix/`,
`refactor/`, `docs/`, `test/`, `chore/`, `art/`, `audio/` conventions or the
`codex/` task prefix are accepted. This includes `audio/valen-voice-production-v1`
only when its selected checkout satisfies every preflight gate.
All require the official repository origin, clean tracked/untracked files,
submodules at the recorded gitlink commits with clean files and matching origins,
and the exact Skyrim **1.7.104.0** / SKSE **2.3.1** contract. BuildInfo and binaries
must identify `runtime17-<full source SHA>`. `ExpectedCommit`, when supplied, must
match that exact SHA. Unsupported branch names and detached HEADs are refused.
Development branches need not already be integrated into main. The helper never
switches branches or falls back to another checkout when the selected source is
dirty or incompatible. It displays that source's path, attached branch and full
SHA and mode before compilation. These DEV rules do not broaden the separate Modding
authoring/ancestry policy.

For `main`, `refs/remotes/origin/main` must resolve to a commit and **HEAD must
equal it exactly**. Local ahead, behind or divergent main is refused. Before an
operation using main, explicitly fetch and check the resulting HEAD/reference
state:

```powershell
git -C '<selected-main-checkout>' fetch origin
```

The helper never fetches;
equality with a local remote-tracking ref does **not** prove that ref is fresh.

`Verify` accepts supported historical candidate branches, preserving their original
commit, identity and branch even when the current verification HEAD is later.
The frozen candidate, configuration SHA-256 pin, scope/dependency hashes,
destination identities, protected configurations and installed bytes remain
checked. Original schema-2 experimental receipts remain verifiable/restorable
with their **unchanged original configuration**. Do not relabel receipts as main
or edit the configuration pin to reuse a receipt with a new source path.
RestoreStaging/VerifyRestored retain their existing backup and destination gates;
rollback does not require a clean or current source checkout.

New entry-selected builds use **receipt schema 3**: `configSha256` still pins
the original private JSON bytes, `sourceSelection` is `script-checkout`, and
`source.repository` records the only in-memory override to that configuration.
The candidate and build-layout provenance must agree with this source. Resume
reconstructs that context from the receipt, never from today's entry location or
the historical `repository` field. Schema 2 retains its original configuration-
selected source contract. Neither schema permits changing the configuration pin,
candidate identity or frozen inputs. Do not downgrade a receipt, relabel it,
or recreate deleted receipts/backups as if they could still support recovery.

This support is qualified on local fixtures only. **No real deployment from
main or human A/B validation of that route has been executed.** Historical
experimental A/B acceptances keep their own source identities.

## Entry and private configuration

The ignored personal entry defaults to the Skyrim 1.7 two-PC workflow. For a
new BuildDeploy, it resolves the Git root containing **the script itself**, using
`$PSScriptRoot`, and loads that root's `Tools/Scripts/Invoke-StreRuntime17Dev.ps1`.
It passes the same root through the internal `SourceRepository` argument into
the module, build and receipt. Neither the caller's PowerShell directory nor
`config.repository` can redirect a new normal build. An absolute-path invocation
from another directory still selects the script's checkout.

Run from the personal entry's directory, in PowerShell 7.2+:

```powershell
.\build-and-deploy-dev.ps1
```

Use PowerShell 7.2+, Git and the existing xmake/MSVC/Node/pnpm build environment.
The private config and entry remain ignored. `runtime17-dev.example.json` and
`Start-StreRuntime17Dev.example.ps1` are generic templates for another machine.
`-Runtime17TwoPc` remains an optional alias with identical source selection.
The entry displays path, branch, full SHA and mode. Missing helpers, dirty source,
detached/unsupported branches, wrong provenance or runtime fail without selecting
another checkout. A helper or native child failure propagates as a failed entry.

`-Runtime17ConfigPath <absolute-local-json>` selects destinations/pins/output
settings, not the source of a new build. `-Runtime17ExpectedCommit <full-SHA>`
pins BuildDeploy/Verify; restoration instead uses the saved candidate/backups.
`-Runtime17Mode Verify|RestoreStaging|VerifyRestored` requires an absolute local
`-Runtime17ReceiptRoot`. Resume loads the updated helper beside the entry but
uses the receipt's original source context; it cannot rebuild. Contradictory
receipt/build, Legacy/normal or restoration/ExpectedCommit options fail.

The historical personal body requires explicit `-Legacy`, alone and without
WhatIf; the generic example has no Legacy implementation. The optional historical
lot-4 `-Runtime17RepositoryRoot`/`-Runtime17Phase` route remains explicit and
separate. Neither is a fallback. Normal execution does not invoke the CK importer.

| Field | Meaning |
|---|---|
| `repository` | Historical schema-2/direct-helper source; ignored for a new normal entry-selected build. Keep unchanged for retained receipts. |
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

### Selecting main after integration

The same script-location mechanism selects main or a development checkout.
Place the ignored entry in the checkout intended for work; it must already
contain the compatible versioned helper/module and runtime changes. Main also
requires the explicit fetch and exact HEAD alignment described above. Prepare
the existing submodules and preserve clean tracked/untracked files. No branch
switch, integration, stash or tracked-source copy is performed by the entry.

Keep the reviewed private destination/reference/output settings unchanged.
Select their existing file with `-Runtime17ConfigPath` when it is not beside the
entry. Never repoint `repository` merely to make a new normal build select main.
Historical direct calls to `Invoke-StreRuntime17Dev.ps1` without the internal
`SourceRepository` argument still use configuration-selected schema-2 builds.
Use the original config/receipt pairing for every resume operation. Changing
any byte of that config intentionally invalidates its pin.

The currently retained primary audio checkout may be dirty and may lack the
1.7 helper/runtime changes; an allowed branch name is not sufficient. Preserve
that work and resolve its prerequisites under separate authorization. Keep the
experimental/corrective worktrees until a compatible clean checkout is usable;
fixture PASS alone does not make the primary checkout ready for a real build.
In particular, selecting a parent checkout may make an existing receipts root
fall inside that source: the isolation gate still refuses it. Prepare separate
output/configuration settings for a future run under its own authorization;
keep the original config bytes for historical receipts. The entry never moves
outputs or rewrites the private JSON to bypass this check.

Each new build allocates `buildRoot/<GUID>/build`; the complete build-directory
path is limited to 100 characters as a conservative budget for this project's
object/PCH/dependency suffixes. The root must be ordinary, writable and local;
UNC/mapped-network roots, overlap and physical aliases remain rejected. It is
not a renamed/moved source checkout. If `buildRoot` is omitted, it defaults to
`STRE-R17` at the root of the **configured historical repository's drive**;
prepare that directory first. Selecting a different source does not move outputs.

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

1. Resolve the entry checkout and read local configuration/source metadata.
   `-WhatIf` validates a new entry-selected source and returns a local plan;
   resume plans read their pinned receipt context. No destination resolution/IO,
   process enumeration, compilation or writes occur. Git optional index writes
   are disabled. The historical direct-helper WhatIf remains configuration-only.
2. Check source root/origin/branch, clean source and submodules, gitlink commits
   and origins, runtime/SKSE/build-identity contract, and local output isolation.
   No automatic fetch; main requires the explicit fetch described above and exact
   equality with origin/main. Optional `-Runtime17ExpectedCommit <full-SHA>` pins
   HEAD on the selected branch. At `READY`,
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
private config unchanged: receipts pin its hash. Schema-2/3 receipts are supported;
older native-only schema-1 receipts are explicitly rejected, not reinterpreted.
A failure before complete backups/staging has no target writes to undo; inspect
its phase before choosing recovery. Unrelated external changes are not undone.

## Local validation

```powershell
pwsh -NoProfile -File Tools/Scripts/Test-StreRuntime17Dev.ps1 -OutputRoot '<fresh-local-fixture-directory>'
pwsh -NoProfile -File Tools/Scripts/Test-StreRuntime17Entry.ps1 -OutputRoot '<another-fresh-local-fixture-directory>' -EntryPath '<absolute-test-copy-of-personal-entry.ps1>'
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
Additional cases cover exact main provenance (including missing/non-commit
origin/main, ahead/behind, detached HEAD, dirty source/submodules and wrong
origin/runtime/SKSE/BuildInfo), a complete main candidate cycle, retained
experimental verification/restoration and unchanged configuration pins. Remote
refs are created locally in disposable repositories; no fetch or network access
occurs in these tests. Fixture success does not validate a real main deployment.

The entry suite copies the supplied **actual entry bytes**, including its inert
Legacy body, into local fixture checkouts. It requires the routing prefix to
match the versioned example. Inert helpers test routing/error propagation; the
real wrapper/module test WhatIf and source refusals before any process guard or
build. It never executes the supplied entry in place. Without EntryPath it tests
the generic example. Schema-3 fixtures exercise the effective source through a
complete simulated A/B cycle and resume, including hash/context corruption.
Development-source cases cover every admitted prefix, a complete audio-branch
cycle, source display, dirty-source refusal without fallback, and unchanged
historical candidate identity after verification from a different work branch.
See [dated evidence](../audit/2026-10-22-runtime-1.7-two-pc-workflow.md).
The subsequent [C1083 diagnosis and real native-build validation](../audit/2026-10-22-runtime17-c1083-build-path.md)
records the short-build-root correction, 45 passing fixture cases and the exact
compiled commit. Native build acceptance is separate from deployment/runtime acceptance.
