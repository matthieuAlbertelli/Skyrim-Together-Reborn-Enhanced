# Codex / AutoMod / official xEdit authoring

This is the development procedure, not an implementation-status tracker.
[STATUS](../project/STATUS.md) owns executed evidence and limitations.
[CK_IMPLEMENTATION](../features/alternate-start/CK_IMPLEMENTATION.md) owns record
contracts; its manifest/audits and the feature test plan remain mandatory for
content changes. Campaign authority and ADR-0018 are unchanged.

## Prerequisites and configuration

The sole runtime target is Skyrim SE **Steam 1.7.104.0**. The wrapper checks that
exact file version beside the configured Data directory, reports the actual path
and observed version, and rejects other, missing or unreadable runtimes. This
does not certify Creation Kit, xEdit, plugin format or Papyrus bytecode versions.
Offline fixture qualification and human editor validation are separate verdicts;
[STATUS](../project/STATUS.md) records which have actually run. Production
authoring requires its own authorization and the editor gates below.
Read-only content comparison and native-only DEV deployment are separate:
preserve differing Data files and never automatically promote them into Git.

Common CLI prerequisites are Windows, PowerShell 7.2+, Git, .NET 8 runtime,
AutoMod's inspected prebuilt CLI and the exact Skyrim executable. No plugin-format
migration or ESL conversion is approved. No MO2, TES5Edit-MCP or Delphi is required.

| Action | Additional prerequisite / meaning |
|---|---|
| `Inspect` | Readable plugin; xEdit configuration is optional; inspection is hash checked |
| `PapyrusStatus` | Reports compiler/decompiler discovery without invoking either; xEdit is optional |
| Standalone Papyrus fixture compilation | Available, explicitly selected compiler, assembler, flags/imports and new private output; decompiler is not needed |
| `Preflight`, `DryRun`, `Prepare` | Official xEdit identity required, even for simple CLI preparations; does not execute/validate xEdit. Mutation also requires the clean feature branch, main ancestry, local importer identity, matching Git/Data source and typed adapter |
| Human editor validation | Exact saved candidate, normalization, errors, reopening and retained human evidence; never inferred from preflight |
| `ImportValidated`, `FinalizeInterrupted` | Current runtime matches preparation; all existing source/tool/Git/candidate/receipt gates still apply; historical receipts lacking runtime identity are rejected |

Install [Spooky's AutoMod Toolkit](https://github.com/SpookyPirate/spookys-automod-toolkit)
outside the STRE repository. Build its Debug CLI once from the chosen source:

```powershell
dotnet build <AutoModRoot>/src/SpookysAutomod.Cli -c Debug
```

Extract [official xEdit](https://github.com/TES5Edit/TES5Edit/releases) separately,
keeping its support files. Verify download provenance manually: the executable's
ProductName check is a mistake detector, not a cryptographic publisher guarantee.
The wrapper never downloads tools. Record hashes when upgrading and rerun the
fixture checks. AutoMod installations extracted without `.git` have no provable
source commit; retain their distribution provenance and binary hashes.

Configure local paths in your shell/profile, outside tracked files:

```powershell
$env:STRE_AUTOMOD_ROOT = '<absolute AutoMod checkout/extraction>'
$env:STRE_XEDIT_PATH = '<absolute official xTESEdit.exe>'
$env:STRE_SKYRIM_DATA = '<Steam Skyrim Special Edition>/Data'
$tool = './Tools/Scripts/Invoke-StreModding.ps1'
& $tool -Action Preflight
```

Explicit `-AutoModRoot`, `-XEditPath` and `-DataRoot` override these variables.
The repository root is discovered from the script location, independently of
the caller's working directory. No personal-path fallback is built in.
The runtime is checked beside Data. The CLI project and prebuilt
`bin/Debug/net8.0/spookys-automod.dll` must exist; the wrapper checks the CLI's
ProductName and invokes `dotnet <absolute CLI DLL> --json ...` from AutoMod's root.
This preserves settings/tool resolution without MSBuild, SDK, restore or project
hooks at invocation time. An SDK is needed only to build AutoMod separately, not
to run this prebuilt CLI. ProductName checks detect mistakes; provenance still
needs retained archive/source information and hashes.
UNC/device paths and mapped network drives are refused before file traversal;
reparse ancestors are inspected before descendants. Access denial is blocking,
not interpreted as an absent optional file. Runtime path/version/hash are saved
in new session configurations and checked again before public import/finalization.
Only that child process uses .NET invariant globalization (named cultures are
permitted with invariant data), making float CLI parsing/reflection consistent
on French/English Windows. The user's process/system locale is unchanged.

Close Skyrim, STR client/server, CK, Vortex and every xEdit session first.
The shared process guard blocks `xTESEdit`, `xTESEdit64`, `SSEEdit`, `SSEEdit64`,
`xSSEEdit`, `xSSEEdit64`, and the existing game/CK/Vortex names. Never launch
editors during snapshot/import/deployment. Process checks cannot prevent a
user launching an unrelated application later; file handles also protect each
snapshot and the Data promotion. One wrapper operation per repository is locked.

## Inspection

```powershell
& $tool -Action Inspect                         # info, masters, all QUST
& $tool -Action Inspect -RecordType global
& $tool -Action Inspect -Plugin SkyrimTogether.esp
& $tool -Action PapyrusStatus
```

Every AutoMod response must be one JSON object with Boolean `success=true`.
Nonzero exits, stderr, malformed JSON, missing/false/string success stop execution.
Inspection checks the plugin hash before/after. `PapyrusStatus` reports tool
availability; inspect `compilerAvailable`/`decompilerAvailable` before the first
operation needing that tool. The bounded Valen adapter below compiles its two
generated fragments; the simple adapters do not compile/decompile scripts.

## Prepare a future authorized content change

**Run `git fetch origin` successfully before every `DryRun`, `Prepare` and
`ImportValidated` invocation.** Stop if fetch fails. The wrapper checks the local
`refs/remotes/origin/main` commit and requires it to be an ancestor of `HEAD`;
missing/invalid refs, stale/diverged branches and Git errors fail closed. It does
not fetch or establish remote freshness itself. Integrate the fetched main and
resolve any conflicts before continuing. If this changes HEAD after preparation,
discard the session and prepare a new candidate on that HEAD.

Start from current origin/main on a dedicated clean `feat/`, `fix/`, `test/`,
`chore/` or `codex/` branch. Main, detached HEAD, other branch families, pending
Git operations and dirty/untracked work fail closed. The target ESP must be
tracked and clean; there is no dirty-target override. Data must match the Git
ESP byte-for-byte. Resolve existing CK changes through the established importer
and review them before preparation; do not silently overwrite either version.

Only `STRE_AlternateStart.esp` can be written. `SkyrimTogether.esp` is explicitly
denied, even though it can be inspected. Paths/options cannot be supplied as
plugin names. The initial adapters are deliberately limited to `AddGlobal`
(float GLOB) and `AddQuest` (QUST name/priority, not start-enabled). EditorIDs
must begin `STRE_` and must not already exist. There is no arbitrary AutoMod
argument, deletion, override, merge, ESL or Papyrus write passthrough. Add other
operations only with a typed adapter, postconditions and fixture tests.
`AddValenHelgen` is the dedicated exception described below, with a fixed contract.

The following example is **only for an isolated fixture or a separately
authorized content mission**, never an instruction to add a production test record:

```powershell
git fetch origin
if ($LASTEXITCODE -ne 0) { throw 'Fetch failed; stop before mutation.' }
& $tool -Action DryRun -Operation AddGlobal -EditorId STRE_Example -Value 7.25
git fetch origin
if ($LASTEXITCODE -ne 0) { throw 'Fetch failed; stop before mutation.' }
& $tool -Action Prepare -Operation AddGlobal -EditorId STRE_Example -Value 7.25
# Future quest mission: -Operation AddQuest -EditorId STRE_QUEST_... -Name '...' -Priority 50
```

Each invocation allocates `_audit/modding/<unique-id>/`, ignored by Git, containing
`before.esp`, `repository-before.esp`, a private `Data/STRE_AlternateStart.esp`,
JSON command results and `session.json`. Backups use create-new semantics and
SHA256 verification. Dry-run executes on the private copy, verifies unchanged
bytes, and ends as `DryRunOnly`, which cannot be imported.

Prepare repeats the dry-run, writes only the private copy, rereads the requested
EditorID/FormKey/properties and checks the TES4 flags/form version/HEDR version.
It copies the three required masters into the private Data as ordinary files,
never links. Allow disk space for those copies. It then deliberately returns
**nonzero** with `HUMAN_XEDIT_REQUIRED` for AddQuest/AddGlobal. This is a blocked pipeline, not validation
success. A failed operation is terminal and retains backups; do not rerun its
mutation or bypass its state. Begin a fresh session after resolving the cause.

## Typed Valen authoring (#94)

`AddValenHelgen -ShowXEdit` creates exactly two QUST, one DLBR, two DIAL and two
INFO. Its contract is [CK_IMPLEMENTATION](../features/alternate-start/CK_IMPLEMENTATION.md).
It has no free-form operation/script argument. AutoMod does not author DIAL/INFO;
the versioned `xedit/STRE_CreateValenHelgenDialogue.pas` uses official xEdit's
record allocator, typed links and serializer. `valen_helgen.py` independently
audits every new identity, field order, CTDA, parent, fragment and French string,
and rejects any change to an existing record payload. The master order and ESP
format stay fixed. Valen's voice type must remain MaleWarlock.

```powershell
git fetch origin
if ($LASTEXITCODE -ne 0) { throw 'Fetch failed; stop.' }
& $tool -Action Prepare -Operation AddValenHelgen -ShowXEdit
```

The checkout must already contain committed source changes. This operation opens
two **visible private authoring** windows, first DryRun then Prepare. In each,
accept only the four preselected private modules, wait for script completion,
then close xEdit. These clicks are not human validation. Script mode in tested
xEdit 4.1.5f requires FormShow/module selection and ignores autoload/autoexit;
the bounded 15-minute timeout rejects an unfinished session. It never points at
production Data. The dry-run must leave identical bytes; Prepare exports a new
private `authored.esp`, audits it, then replaces only the private candidate.

Papyrus status must confirm compiler/decompiler availability. The installed
compiler builds the actual allocated QF/TIF names with private, repository and
installed CK/SKSE imports. Errors, warnings, missing output or non-success results
reject the session. Eight companion files are hashed: two PSC, two PEX, one SEQ
for the dialogue owner and three silent PCM WAV timing assets. The WAVs contain
no recorded/generated speech. The independent reader also creates the exact
private `candidate-manifest.json`; the canonical record manifest still describes
the unchanged Git ESP until validated import.

Success returns `State=AwaitingHumanXEdit`, **not validation approval**. This
operation creates neither `validation.json` nor `xedit-validation.log`. The
human must perform the boundary below and explicitly attest the actual result
before either file is created. Preparation records the four original ESP hashes,
private master hashes, tool identities and clean branch/HEAD. Keep that HEAD
unchanged while reviewing. No import/build/deploy is chained to preparation.

After the explicit human PASS, `ImportValidated` rechecks all seven identities,
companion hashes/paths, manifest hash, unchanged originals and common receipt
guards. It freezes the companions/manifest with the validated ESP, imports the
eight explicit generated-artifact paths through the existing CK importer and
updates the canonical records manifest. It rejects existing companion targets;
there is no overwrite escape hatch. Before the post-import packaging audit,
register only these eight verified paths with the existing
`Add-NewManagedFilesToManifest` helper (and its `Read-ManagedFileManifest`
dependency) from the local build script. Load only those function definitions;
do not invoke or dot-source the whole build/deploy script at this gate. Inspect
the complete Git diff and all audits, then obtain separate build/deploy
authorization. The local build's established CK ownership collector includes
generated fragments, SEQ and plugin voice assets.

Tests: `python -m unittest discover -s Tools/Scripts -p test_valen_helgen.py`
uses tiny synthetic xEdit output, including corrupted contract cases.
`Test-StreValenAuthor.ps1 -XEditPath <official executable>` repeats actual
DryRun/Prepare on newly generated synthetic modules, with the same visible-window
steps and before/after input/master hashes. Neither test grants human xEdit PASS.

## Typed Valen matrix preparation (#96)

Preparation preserves each raw Git/Data hash independently. Binary companions
must match byte for byte. For `.psc` only, the comparison permits CRLF versus LF
introduced by Git's `text=auto` / `core.autocrlf`; all other bytes, BOM and final
newline count must match. No original is normalized or rewritten. The private
copy uses the repository bytes, and `InputComparisons` records both raw hashes
and any `PscCrLfOnly` comparison. This does not change the CK importer's rules.

`AddValenDialogueMatrixV1 -ShowXEdit` is a bounded preparation operation for
[the complete matrix contract](../features/alternate-start/VALEN_DIALOGUE_MATRIX_V1.md).
It requires a clean committed feature checkout and both Windows/Linux CI runs
successful on that exact HEAD. Fetch origin before invoking it:

```powershell
git fetch origin
if ($LASTEXITCODE -ne 0) { throw 'Fetch failed; stop.' }
& $tool -Action Prepare -Operation AddValenDialogueMatrixV1 -ShowXEdit
```

Preparation writes the exact 300-record plan before opening official xEdit.
The same visible private DryRun/Prepare workflow applies. This adapter permits
293 creations and updates only the seven existing Valen identities; the audit
reports actual payload differences separately. No Python helper writes an ESP
or assigns a FormID. The independent reader checks all serialized conditions,
graph links, fields, VMAD and approved French text. Unknown additions fail closed.

Only PapyrusCompiler.exe compiles the generated QF and observer. Its exact
command and result are recorded; any warning/error fails preparation. The
private companion inventory is checked against the original manifest plus the
bounded generated paths. Only the existing narrative QF PSC/PEX may change;
the TIF, SEQ, three existing WAVs and all unrelated companions remain identical.
Every new WAV is checked for filename, PCM format, timing, silence and SHA-256.
Git/Data originals, masters, tools, branch, HEAD and plan are checked again.

Success is `HUMAN_XEDIT_REQUIRED`, with the session state `AwaitingHumanXEdit`.
The draft validation JSON contains empty hashes/reviewer and false check flags;
it is not an attestation. No human validation log is fabricated.
Import/promotion, build/deploy and runtime require later maintainer authorization.
Synthetic tests and automated UI authoring never satisfy the human boundary.

### Validated matrix import

`Import-StreValenMatrixValidated` handles only `AddValenDialogueMatrixV1`.
Other operations retain their exact preparation-HEAD requirement. The matrix
may be imported on a later committed HEAD on the same branch only when its
preparation HEAD is an ancestor, current origin/main is an ancestor, the checkout
is clean, and both Windows/Linux CI runs succeed on that exact import HEAD.
The session must still be `AwaitingHumanXEdit`. A changed candidate, script,
companion, plan, writer, reviewed graph, audit or protected original stops import.
An unchanged candidate does not need another human review merely because the
import implementation advanced. A changed artifact requires a new review.

Retain the original review dossier, including `review-evidence-hashes.json`,
`review-private-hashes.json`, `review-graph.json`, and the proposed record
manifest. After the actual human receipt/log is complete, pin their existing
bytes once with the known reviewed candidate hash and preparation commit:

```powershell
python -B Tools/Scripts/valen_matrix_import.py --root $session --seal-review `
  --expected-candidate-sha256 $reviewedSha256 --expected-preparation-head $preparationHead
```

This creates only `import-evidence.json`, refuses replacement, verifies the
historical hashes, and never creates a human verdict. It preserves the receipt,
log, ESP and historical evidence unchanged. The checksum inventory is not a
cryptographic signature. Existing xEdit `.refcache` files are pinned separately
as non-import files; new/unexpected private files fail closed.

Before promotion, the current binary auditor must reproduce `candidate-audit.json`
semantically, including the complete graph, 22 class/fallback contexts and all
100 survivor cases (32/17/51/0). The current asset reader verifies the original
PCM bytes, timing, names, scripts and hashes without rewriting `assets-audit.json`.
All originals, private masters, tools, receipts and target collisions are checked.

The delta is computed from `PrivateOriginals`, `Companions` and `assets-audit`.
`validated/` contains only the reviewed ESP and added/changed companions, never
masters, audit files, caches, `.vortex_backup` or `SkyrimTogether.esp`. Existing
targets are backed up with their separate Git/Data bytes. The wrapper rereads
the candidate immediately before promotion, uses the existing explicit CK
importer in batches of at most 64 paths and 16000 native-command characters, and requires snapshot = Data = repository SHA256
for every promoted file. Ordinary scripts are explicitly imported once here;
their later authority remains the repository, not automatic Data collection.

The exact proposed record manifest must preserve every existing constraint and
add only audited identities. Packaging uses the existing local helper with the
explicit import inventory, and verifies the resulting exact union. It never
loads the build/deploy entry point. `preparation-session.json` preserves the
original session bytes; lifecycle state records `PreparationHead`, `ImportHead`,
CI and inventory hashes separately. Partial promotion is terminal `ImportFailed`;
retain backups and investigate instead of retrying. `Imported` cannot be replayed.

One explicit completion action exists for a failed matrix transaction whose entire
content delta is already present, byte-identical in snapshot, Data and repository:
`Invoke-StreModding.ps1 -Action FinalizeInterrupted -Session <session>`. It does not
copy content or rerun the importer. It requires the original frozen evidence,
both original manifests, no staged files and no dirty path outside the exact
approved content delta. Preparation and original import HEADs must be ancestors
of the same branch's current HEAD, with current main ancestry and Windows/Linux
SUCCESS on that exact completion HEAD. The current binary/asset auditors rerun
before manifest promotion. Missing or differing files are rejected, not repaired.
`CompletionHead`/`CompletionCI` are recorded separately; the failed result and
preparation evidence remain intact.

The local build workflow uses `Stre.CkImportBatches.ps1` around its existing CK
importer call as well, preserving its existing snapshot and dirty-target policy.
This avoids the same Windows argument limit when collecting the complete bundle.
The underlying workstation importer remains unchanged.

Run `Test-StreValenMatrixImport.ps1` and `test_valen_matrix_import.py` before real
import. Their receipts, tools and content are explicitly synthetic. The former
uses real disposable Git ancestry and the CK bridge; the latter independently
rereads invented official-xEdit binary fixtures, PCM assets and review evidence.
After import run every feature audit/test, commit and push the content, wait for
Windows/Linux CI on that new HEAD, then perform an authorized build/deploy.
Never launch Skyrim as part of this workflow.

### Bounded presentation iteration after the c48be63 runtime failure

`FixValenPresentation -Variant A|B -ShowXEdit` prepares a separate, private
presentation-only candidate from the exact tested c48be63 ESP. The
[presentation smoke contract](../features/alternate-start/VALEN_PRESENTATION_SMOKE.md)
owns the six text substitutions, frozen corpus, one-INFO experiment and ordered
engine-load/skip evidence. No records or companions are added; no WAV or Papyrus
artifact is regenerated. All existing import evidence is retained unchanged.

For this iteration the maintainer explicitly defers new full Windows/Linux CI
until the merge HEAD: local tests/audits → DEV build/deploy → STOP → human Skyrim
smoke. This exception does not alter the historical matrix import CI guards or
the official xEdit human-review boundary below. The new private adapter stops at
`HUMAN_XEDIT_REQUIRED`; its generated plan is not a human receipt.

### Bounded navigation iteration after the D menu failure

`FixValenNavigation -ShowXEdit` derives the corrected Player Dialogue topology
from the frozen matrix, using exact deployed D as its Git/Data baseline. The
[topology contract](../features/alternate-start/VALEN_PLAYER_DIALOGUE_TOPOLOGY.md)
owns the vanilla evidence, four adapter records, blank Responses, explicit
root return and stage-dependent Helgen starting topic. No companion changes
are allowed. The wrapper uses the existing private authoring and validated
import path; the record manifest receives exactly four allocated identities.

The saved candidate requires a new human receipt, including Boolean
`ReopenedSavedFile`, `SecondCheckForErrors` and `NoErrorsReported`, in addition
to the existing checks and saved/log/tool hashes. This content iteration has
**no CI and no push**: local tests/audits, human xEdit, validated import, DEV
build/deploy, STOP for human menu smoke. Hadvar validation remains suspended.

## Official xEdit boundary

AutoMod/Mutagen can reorder masters. An AutoMod write is never ready for Git by
itself. The required saved master list is exactly:

```text
Skyrim.esm
Update.esm
Dragonborn.esm
```

The [official scripting API](https://tes5edit.github.io/docs/13-Scripting-Functions.html)
documents `SortMasters(file)` (current load order) and `Check(element)` (error
text). This establishes useful primitives, not this wrapper's ability to traverse
every error and prove a durable save unattended. The
[official CLI/save documentation](https://tes5edit.github.io/docs/2-overview.html)
documents alternate Data/log paths and save confirmation; messages must be
checked because saving can fail. The
[official release notes](https://github.com/TES5Edit/TES5Edit/releases)
document quickedit/autoload/autoexit/direct-save options. None is treated here
as a validated headless save-and-check protocol. Only the dedicated Valen
authoring adapter launches its versioned script; exit code zero alone grants no
approval. Quick Auto Clean is not a replacement for Sort Masters/Check for Errors.

For the actual candidate, a human must:

1. Open official xEdit in SSE mode against the **private** Data directory. For
   example, from PowerShell (replace `$session` with the printed absolute path):

   ```powershell
   & $env:STRE_XEDIT_PATH -SSE "-D:$session\Data\" '-quickedit:STRE_AlternateStart.esp'
   ```

   Check the startup Messages Data path and selected plugin before proceeding.
   Only the candidate and required masters should be loaded. The candidate must
   never be SkyrimTogether.esp. Do not save master changes.
2. On the candidate, run **Sort Masters (to match current load order)**, verify
   the expected list, then **Check for Errors** for the complete plugin. Follow
   [the official error-checking procedure](https://tes5edit.github.io/docs/7-mod-cleaning-and-error-checking.html).
   Any error stops the workflow; there is no automatic waiver/repair.
3. Save only the candidate; inspect save messages and close xEdit completely.
   Reopen that saved file, verify the requested record and all intended changes,
   rerun Check for Errors, then close again. Preserve relevant messages from both
   sessions in `<session>/xedit-validation.log`, including target/path, tool
   version, completed error check, errors/warnings and save outcome.
4. Complete `validation.json` **only after that actual review** (the simple
   adapters create a false-valued template; the Valen adapter creates no receipt):
   `Reviewer`, `PluginSha256`, `LogSha256`, and Boolean `SortMasters`,
   `CheckForErrors`, `SavedAndClosed`. Keep `Schema=1` and the recorded
   `XEditSha256`. Obtain hashes with `Get-FileHash -Algorithm SHA256` on the
   saved candidate and evidence log. Never invent logs or mark these true from
   AutoMod success, a correct master list or old evidence.

This receipt is a human attestation tied to exact bytes, not a machine-certified
xEdit verdict or tamper-proof authorization token. Synthetic test receipts are
explicitly labeled as such and never apply to real content.

## Import, review, deployment and gameplay

```powershell
git fetch origin
if ($LASTEXITCODE -ne 0) { throw 'Fetch failed; stop before mutation.' }
& $tool -Action ImportValidated -Session $session
git diff --stat
git status --short
# Run the feature-owned CK audits and inspect the semantic record diff.
# Only after successful review, use the existing local build/deploy entry point:
./build-and-deploy-dev.ps1
```

Except for the bounded matrix descendant rule above, resume requires the same branch/HEAD, tools, configuration, unchanged original
Data/Git ESP, backups and matching human receipt/log. It independently rereads
the saved TES4 master order and AutoMod master list, format and expected record.
The validated copy is frozen into `validated/`. Only then is Data updated to
those same bytes and **the existing** root `import-from-ck-dev.ps1` invoked with
`-RelativePath STRE_AlternateStart.esp -SourceDataRoot <validated snapshot>`.
The wrapper never passes `-AllowDirtyTarget`. It checks the importer exit and
resulting Git ESP hash. Partial failure is terminal, retains backups, and never
automatically continues to build or retries an import.

Keeping Data aligned matters because the existing local build script snapshots
and imports CK outputs before deployment. Do not run it on a different Data
candidate. The simple adapters import only this ESP. The Valen adapter also
imports its eight fixed companions; generated artifacts from other CK work
still use the established ownership workflow.
Ordinary `STRE_*.psc/.pex` remain repository-authoritative; generated CK fragments
retain their existing source/compiled consistency checks. Data is a working
authoring location, not the final source of truth.

The machine-specific root importer/build scripts remain local and ignored.
Do not force-add them. On a new workstation, connect their existing
`Assert-GameProcessesStopped` to tracked `Tools/Scripts/Stre.ProcessGuard.ps1`,
calling it before snapshots, imports and deployment copies (including after a
long build). Dot-source it **after** any old same-named function definition or
replace that function. A missing helper must stop the local script. Do not copy
machine paths into the generic module.

The local CK collector must exclude Vortex metadata at both boundaries. In
`Get-CkPluginOwnedFiles`'s nested `Add-OwnedFile`, return before adding any file
whose filename ends with `.vortex_backup`, using `StringComparison.OrdinalIgnoreCase`.
In `Test-CkAutoImportPath`, return `$false` immediately for a relative path with
the same suffix. Do not delete or alter those backups. Do not replace this exact
suffix exclusion with a global voice-extension allowlist: WAV, FUZ, LIP, XWM and
other genuine voice artifacts must keep their existing eligibility. The manifest
is populated only from the filtered auto-import set.

Test the actual workstation functions without loading the deployment entry point:

```powershell
./Tools/Scripts/Test-StreCkCollector.ps1 -BuildScriptPath ./build-and-deploy-dev.ps1
```

This local fixture test extracts the function definitions using the PowerShell
parser. It preserves a real PCM WAV, rejects lowercase/mixed-case backup suffixes
at both boundaries, preserves future extensions and non-terminal substrings,
and checks the resulting manifest and untouched fixture backups. It requires
the local ignored script and is not an unattended deployment or a CI stand-in
for a workstation script absent from the repository.

Build/deploy and in-game acceptance remain explicit subsequent steps. This
infrastructure does not prove gameplay, complete quest behavior or a new content
record's runtime behavior. The first Valen content candidate remains subject
to its explicit human xEdit, import, build/deploy and in-game gates.

## Rollback and cleanup

Before promotion, failures affect only the private session: original Data/Git
must retain `BeforeHash`. Preserve the session for diagnosis. After an import
failure or rejected content review, stop all guarded processes, inspect Git
status and snapshot any newer work before restoring. Compare `before.esp` and
`repository-before.esp` against the recorded hash. Restore the specific Data ESP
from its backup, then use the existing explicit CK importer for the repository;
any deliberate overwrite of a dirty target requires human review through that
importer's existing policy. Never use blanket reset/clean or restore unrelated
PSC/PEX/SkyrimTogether.esp. If the branch already committed the content, use a
reviewed revert/change rather than silently discarding it. Redeploy only the
reviewed consistent repository/Data pair.

Do not delete backups automatically. After acceptance or abandonment, with all
tools closed, identify the exact session under this repository's `_audit/modding`
and remove only that directory if its evidence/backups are no longer needed.
Retain any evidence required by the PR. Do not recursively delete Data, Vortex
staging, save directories or the shared audit parent.

## Automated checks

```powershell
pwsh -NoProfile -File Tools/Scripts/Test-StreModding.ps1
pwsh -NoProfile -File Tools/Scripts/Test-StreModding.ps1 -RealTools
pwsh -NoProfile -File Tools/Scripts/Test-StreModding.ps1 -RealTools -HumanFixtureOutput ./_audit/my-new-human-fixture
```

The default suite creates its own synthetic ESPs and disposable Git repositories;
tool responses and human attestations are explicitly simulated. It verifies
current-main ancestry, stale/diverged branches and missing
`origin/main` using real local Git histories without a network remote. It needs Git
and PowerShell, with `dotnet` discoverable for the path-resolution test, but no
game/modding binaries. Windows CI runs it. `-RealTools` additionally uses the
configured installed AutoMod/.NET/xEdit/runtime preflight, runs a real GLOB
dry-run/write/reread **only on a synthetic plugin**, and stops at the human gate.
Its master files are fixtures, not Bethesda content, and xEdit is never launched.
It checks repository and installed production ESP hashes before/after. No test
creates a quest or imports a production ESP. Temporary fixtures are deleted only
after validating their unique path beneath the temporary directory.

`-HumanFixtureOutput` retains only independent synthetic plugin/masters, the
before-image and a clearly labeled machine-evidence JSON in a **new** `_audit`
subdirectory. It exports no human attestation and no importable session. The
current durable file and first human check are identified in the
[1.7 toolchain evidence](../audit/2026-10-22-runtime-1.7-toolchain.md).
The observed AutoMod write removes unused masters from the initially empty GLOB
fixture. Its human xEdit check must use Add Masters and Sort Masters to restore
the exact required three-master order before saved-file validation. A successful
CLI trial does not waive the normalized-format gate or authorize promotion.

For a real Papyrus fixture trial, inspect the selected compiler, assembler and
their child-process behavior first, then run:

```powershell
pwsh -NoProfile -File Tools/Scripts/Test-StrePapyrus.ps1 `
  -AutoModRoot '<local toolkit>' -DataRoot '<installed Skyrim>/Data' `
  -CompilerPath '<compiler reported by PapyrusStatus>/PapyrusCompiler.exe' `
  -FlagsPath '<explicit TESV_Papyrus_Flags.flg>' `
  -OutputRoot './_audit/my-new-papyrus-fixture'
```

This calls `PapyrusStatus` first, requires the reported compiler to match the
explicit selection, creates two synthetic scripts and compiles only those.
Import precedence matches the existing adapter: private source, project
`Source/Scripts`, project `scripts/source`, installed `scripts/source` (SKSE
extended headers), then installed `Source/Scripts` (Bethesda headers). Imported
project scripts are read only. Outputs and diagnostics stay in the new directory,
including paths with spaces. Compiler/assembler/flags/import hashes are checked
before/after. Exit code, stderr, zero-error/zero-warning diagnostics, two PEX files
and their Skyrim PEX header are checked; PEX 3.2/game ID 1 is not a runtime version.
No copy to Data/GameFiles, editor launch, decompilation or production authoring.

Before running a specialized suite, inspect it and its children. The guard-only
`Test-StreValenMatrix`, `Test-StreValenMatrixImport`, `Test-StreValenPresentation`
and `Test-StreValenNavigation` suites use synthetic files/mocked evidence. The
`Test-StreValen*Author` paths launch real xEdit and are **outside an offline-only
mission**. Never stop a maintainer process automatically to satisfy a guard.
Simulated receipts in tests remain labeled synthetic, and a real AutoMod stop at
`HUMAN_XEDIT_REQUIRED` is the expected successful boundary, not editor acceptance.

## Minimal human Creation Kit check

This first check qualifies only CK startup and read-only inspection of the base
master in the installed **Skyrim Steam 1.7.104.0** environment. The current tool
identity and exact input path are recorded in the
[toolchain audit](../audit/2026-10-22-runtime-1.7-toolchain.md#next-human-check-minimal-creation-kit-inspection).
CK's own file version is distinct from the game version. This is the repeatable
procedure; [STATUS](../project/STATUS.md) records the maintainer's bounded PASS.
The procedure itself is not production-authoring authorization.

1. With the game and other modding tools closed, manually start the existing
   inventoried `CreationKit.exe`. Record the version shown. Do not install,
   update, reactivate CKPE or change INI settings as part of this first check.
   If startup fails, retain the precise diagnostic and stop.
2. In **File > Data**, select only the installed **Skyrim.esm**. Leave every
   other plugin unchecked and **no file active**. In particular, do not load
   `STRE_AlternateStart.esp` or `SkyrimTogether.esp`. Load the master and wait for
   completion. Record any message instead of assuming warnings are harmless;
   a blocking error, missing dependency or unexpected plugin load ends the check.
3. In **Object Window > Miscellaneous > Global**, locate `TimeScale`, open its
   record dialog, read its EditorID/type/value, and close with **Cancel** without
   changing anything. Expected: the object list is populated, the record dialog
   is usable and the editor remains responsive. No script compilation, record
   creation or save is part of this check.
4. Exit normally **without saving**; decline any save prompt. Report startup,
   master-load and record-inspection results separately, plus normal exit and
   the actual version/messages. Preserve any existing CK session diagnostics;
   do not invent a validation receipt or start import/deployment.

A successful result is a bounded human CK inspection PASS. It does not qualify
CK write/save/reopen, the xEdit fixture in CK, production plugins, quests,
fragments, assets or in-game behavior. Keep the accepted xEdit fixture unchanged;
its private Data-path options are specific to xEdit and are not CK arguments.
Any later CK write cycle requires a separately prepared independent candidate
and the existing authoring and human-validation gates.

## Future human Creation Kit fixture write cycle

**Protocol only, not executed or staged.** This first write check is limited to
creating one synthetic GLOB, saving a new test ESP, closing CK and reopening that
saved file. It does not qualify editing an existing production record. Use the
same inventoried CK **1.6.1378.1** in the **Steam Skyrim 1.7.104.0** environment.
The maintainer alone performs the later run; Codex creates no fixture and changes
no installed content during documentary preparation.

Fixture identity and scope:

- New plugin: **`STRE_CKWriteFixture.esp`**; GLOB:
  **`STRE_CKWriteFixtureGlobal`**, **Float 7.25**, **Constant unchecked**.
- Do not copy/load `STRE_AlternateStart.esp`, `SkyrimTogether.esp`, or the accepted
  xEdit fixture. Only the vanilla Skyrim.esm is a read-only input. This disposable
  manual test does not extend the wrappers' production writable-plugin allowlist.
- CK's normal Data-file workflow places this new ESP temporarily at
  `C:\Program Files (x86)\Steam\steamapps\common\Skyrim Special Edition\Data\STRE_CKWriteFixture.esp`.
  This future human-only placement is explicit; no xEdit-style private Data
  switch or unsupported CK isolation mode is assumed. Never activate it in the
  game's plugin list, register it in Vortex, deploy it or run an importer.

1. **Prepare the later run:** close other game/modding tools and verify the exact
   test filename does not already exist in Data. If it exists, stop without
   overwriting, deleting or reusing it. Choose a fresh ignored worktree folder
   such as `_audit/ck-write-fixture/<unique-run>/` for copies and evidence. Retain
   before hashes of Skyrim.esm and the installed STRE production ESPs solely for
   preservation comparison; do not open those ESPs in CK. Do not change tools,
   CKPE or INI settings.
2. **Create:** start CK manually; in File > Data, load Skyrim.esm with no active
   file and no other plugin selected. Under Object Window > Miscellaneous >
   Global, use **New**, not Duplicate or an edit of TimeScale. Enter the exact
   new EditorID, Float 7.25 and Constant unchecked, then confirm. Stop if that
   EditorID already exists. No scripts, assets, other records or vanilla overrides.
3. **Save and close:** File > Save must prompt for a **new** plugin. Enter the
   exact `STRE_CKWriteFixture.esp` name and verify the destination above. Any
   overwrite prompt or different destination is a stop. Confirm saving only
   this test ESP, then exit CK normally. Record the saved ESP SHA-256 and keep an
   independent copy in the fresh evidence folder before reopening it.
4. **Reopen:** start a fresh CK process manually. Select only the saved test ESP
   as **Active File** and its required Skyrim.esm master; verify the actual
   dependency list and stop if any unexpected master or STRE plugin is involved.
   Open the new GLOB and verify the same EditorID, Float 7.25 and Constant
   unchecked. Record its actual FormID and master list; do not preassign a FormID
   or change a format field to match the runtime number. Close the dialog with
   **Cancel**, then exit **without saving**.
5. **Preserve the result:** confirm the saved ESP hash still equals the hash
   recorded before reopening, and the protected before hashes are unchanged.
   Retain the independent test ESP, SHA-256, CK version, actual master list,
   record fields and any available session messages. Report create/save/reopen
   and exit outcomes separately. The previously reported MASTERFILE warning is
   not permission to disregard new warnings or a failed save/load.
6. **End the later run:** with CK closed and the independent copy/hash verified,
   remove only this run's positively identified temporary test ESP from Data;
   retain evidence and identify any fixture-only backup separately. Never clean
   Data globally, delete an uncertain file or touch production ESPs. No game
   activation, import, deployment or promotion follows this check.

Expected PASS is narrowly **CK new-GLOB creation/save/reopen on that fixture**,
with preserved fields and no unexpected content changes. Failure or ambiguity
stops the run with the real diagnostic. No validation JSON or positive result is
prepared in advance. Modification of an existing record, broader CK features,
production plugins and their xEdit/import gates remain separately unqualified.
