# Native/UI DEV workflow for two PCs

This procedure owns the reusable deployment mechanics for issue #99.
[STATUS](../project/STATUS.md) owns executed validation; the
[runtime migration guide](EXPERIMENTAL_RUNTIME_1_7.md) owns native compatibility.
The lot-6 preparation authorizes **local script/fixture work only**. A maintainer
must authorize and execute a later real run. Pc-vero remains **NON AUDITÉ PAR
CODEX**; its readiness is the maintainer's responsibility. No incompatibility is
inferred from that absence of an audit.

## Routing and scope

The historical ignored `build-and-deploy-dev.ps1` selects the primary checkout,
imports CK content, builds into its ordinary distribution and copies content and
native files into two Vortex stagings. Its remote share is explicitly a Vortex
mod directory, not the installed game. Its hashes establish staging agreement.

The lot-4 explicit local wrapper introduced a selected 1.7 worktree, fresh xmake
outputs, compiled identity, native-only copies and unlink-before-copy protection.
The new tracked `Tools/Scripts/Stre.Runtime17Dev.psm1` extends those mechanics.
It uses the same configure/build/install commands, native launcher/server/UI
package and Vortex ownership model. It does not invoke the CK importer.

The personal entry routes `-Runtime17TwoPc` to
`Tools/Scripts/Invoke-StreRuntime17Dev.ps1` in the selected experimental worktree.
The earlier explicit lot-4 parameters remain available. An invocation without an
explicit mode now fails; the old body requires `-Legacy`, which is **not a 1.7
deployment procedure**. Neither legacy path supports WhatIf; they reject it.

The complete operation is one PowerShell command with two human handoffs:

```powershell
pwsh -NoProfile -File .\build-and-deploy-dev.ps1 -Runtime17TwoPc
```

Run from the directory containing the personal entry. At `READY`, confirm that
both PCs' game, launcher, server, Vortex, Steam, CK and xEdit are closed and remain
closed during staging. After `AWAITING_HUMAN_VORTEX`, deploy the STRE DEV mod
manually in Vortex on **both** PCs, close Vortex, then enter `DEPLOYED`. The same
command resumes and hashes both actual installations. The script never launches
or controls these applications. This pause is required to preserve Vortex's
deployment ownership; staging success alone is never installation success.

## One-time private configuration

Use PowerShell 7.2+, Git and the existing xmake/MSVC/Node/pnpm build environment
from the migration guide. No compiler installation is performed by this helper.
Copy `Tools/Scripts/runtime17-dev.example.json` to the ignored
`_audit/runtime17-dev.local.json` beside the personal entry. Keep this file and
its referenced accepted manifest in private backups, outside Git.

| Field | Required meaning |
|---|---|
| `repository` | Absolute local root of `feat/skyrim-1.7-runtime`, not its parent checkout |
| `receipts` | Existing absolute local directory outside that worktree and all destinations |
| `referencePath`, `referenceSha256` | Local accepted qualification manifest and its pinned SHA-256 |
| `targets[0]` / `targets[1]` | IDs `A` and `B`, in that order |
| `stagingRoot` | Existing STRE Vortex mod root containing content and `SkyrimTogetherReborn` |
| `gameRoot` | Actual Skyrim root containing `SkyrimSE.exe` and `Data`, never a staging alias |

For the maintained machine, A and B staging paths come from the historical
script. **The script contains no actual game path for B.** The prepared local
configuration initially left B's `gameRoot` null. The maintainer subsequently
supplied that path; it was saved with local JSON validation only, without probing
the share. A new configuration still requires an existing accessible share to
the actual installation before any real run.
No path is guessed, share created, SMB permission changed or access attempted
during preparation. Missing gameRoot fails before source/build/destination work.

Paths must be distinct, non-overlapping ordinary directories. Junctions/symlinks
and equal directory file IDs are rejected. Directory file-ID queries must be
supported by the filesystem/share. No remote execution or process enumeration
is used; readiness on B is human-attested. Local process enumeration is read-only.

The private reference schema has `schema: 1`, `runtime: "1.7.104.0"`,
`compiledCommit`, `nativeFiles`, `contentFiles`, `dependencies` and provenance.
Every row has relative `path`, `bytes` and `sha256`. Native paths are relative to
`SkyrimTogetherReborn`; content paths to Data; dependency paths to the game root.
Content rows additionally pin `repositoryBytes` and `repositorySha256` against
`GameFiles/Skyrim`. The full tracked content path inventory must agree.

The prepared reference is derived from the retained lot-4/Better Grabbing
receipts: 293 historical native/UI files, 914 accepted contents, Skyrim 1.7.104,
SKSE 2.3.1, Address Library V5 and Better Grabbing 2.0. It preserves the 16
accepted source/installed divergences (14 Valen WAVs and two PSC line endings)
as separate hashes, not broad exceptions. Any new source/content/dependency
divergence requires human requalification and a newly pinned reference; do not
regenerate it from unreviewed files just to make preflight pass.

Back up the private configuration, reference and local entry together. To restore
the workflow on a prepared machine, restore those files and the selected Git
worktree, review the paths, and run WhatIf. The tracked
`Start-StreRuntime17Dev.example.ps1` is an optional standalone replacement entry
for a machine without the historical wrapper; copy it beside `_audit` under the
habitual filename. It does not contain personal paths or the legacy CK body.

## Server configuration preservation

The canonical path relative to each runtime directory is **`config/STServer.ini`**,
as used by `DediRunner::LoadSettings`. A root-level `STServer.ini` is not a
substitute. The INI is excluded from native packaging, copying and restoration.

- The installed `Data/SkyrimTogetherReborn/config/STServer.ini` is required by
  this workflow to verify the existing Better Grabbing policy. Absence blocks
  with `Required installed server configuration is absent`; never start the
  server or create an empty/default INI to bypass this check.
- The staging `SkyrimTogetherReborn/config/STServer.ini` is optional. Its initial
  absence is recorded as `presence: absent` with a null hash. If present, its
  policy is checked and its SHA-256 retained. The installed copy remains required
  even when a staging copy exists.
- Verification compares both presence and hash: an initially absent staging INI
  appearing, an existing INI disappearing, or changed bytes all block. Neither
  contents nor credentials are printed or copied into receipts.
- Only explicit file/directory-not-found errors establish absence. Denied
  metadata/read access and other IO failures are distinct blocking errors,
  including for the optional staging copy. Reparse paths are refused.

## Exact run sequence

1. Read configuration. WhatIf returns here, without destination resolution,
   existence checks, directory-ID queries, process enumeration, build or writes:

   ```powershell
   pwsh -NoProfile -File .\build-and-deploy-dev.ps1 -Runtime17TwoPc -WhatIf
   ```

2. Check the actual repository root, canonical origin, exact branch, clean Git
   state including untracked files, gitlink commits and submodule origins/clean
   state, and the source runtime/SKSE/build-identity contract. Only actual Git
   gitlinks count; obsolete `.gitmodules` entries do not fabricate dependencies.
   No fetch is performed. Optional `-Runtime17ExpectedCommit <full-SHA>` pins a
   specific source; otherwise the clean HEAD is captured and rechecked after
   compilation. A later documentation commit never relabels an earlier binary.
3. Preflight **both** destinations before compilation or any target write:
   accessible distinct directories, accepted content in both staging and Data,
   exact external dependency hashes, qualified plugin set, no old CKPE proxy and
   retained required Better Grabbing server policy. Current staging and installed
   native/UI payloads must agree on each PC; resolve pending Vortex conflicts
   before rerunning. They need not be the historical 293-file build.
4. After human readiness, create a unique local receipt directory. Run
   `xmake config -P . -m debug -y -o <fresh-build>`,
   `xmake build -P . -y -j <jobs>`, `xmake install -P . -o <fresh-distrib>`
   from the selected worktree. Capture the log and fail on any exit error.
5. Verify generated BuildInfo and embedded client/server identity
   `runtime17-<full-source-commit>`, required launcher/native/CEF files and UI.
   Generate the SHA-256/size manifest from the actual distribution. File count
   is dynamic; PDB/ILK/LIB/EXP and test executables are excluded. Unreviewed root
   payload names are rejected. CEF/Discord libraries and UI sound assets belong
   to this native/UI package; CK audio, ESP/PSC/PEX, saves and settings do not.
6. Repeat both destination preflights after the long build. Back up **both**
   staging and installed native/UI inventories to the local receipt and verify
   backup hashes before changing either target. Snapshot server-configuration
   presence/hashes without copying or editing their contents.
7. Copy the same manifest to A and B staging. Unlink each destination file before
   copying, including obsolete owned UI/assets/locales; do not overwrite a
   hardlinked inode. Verify exact file sets, sizes and hashes. Preserve content,
   dependencies, settings, caches, logs, server state and saves. Receipts
   `staged-A.json` and `staged-B.json` explicitly say installation is unverified.
8. Wait for the human Vortex handoff, then check both staging and actual
   `Data/SkyrimTogetherReborn` payloads, content/dependencies and unchanged server
   configuration again. Write distinct `verified-A.json` / `verified-B.json`
   with compiled commit, current verification HEAD, files and UTC. Only both
   passing yields `INSTALLED_VERIFIED`. No runtime loading, Solo, admission or
   World Sync result follows from these hashes.

## Failure, resumption and restoration

Any failure stops the run; no automatic build reuse, retry or game launch occurs.
The printed receipt directory contains `receipt.json`, `manifest.json`, build
log, per-PC results, fresh distribution and `backup/A|B/staging|installed`.
Never delete it to conceal partial success. Read its overall status and both
per-PC results; A can have passed while B failed. Network availability and Vortex
deployment are not a cross-PC transaction.

If staging succeeded and the human deployment was deferred or failed, correct
that deployment manually and verify the retained candidate without rebuilding:

```powershell
pwsh -NoProfile -File .\build-and-deploy-dev.ps1 -Runtime17TwoPc -Runtime17Mode Verify -Runtime17ReceiptRoot '<receipt-directory>'
```

To abandon a partially staged or deployed candidate:

```powershell
pwsh -NoProfile -File .\build-and-deploy-dev.ps1 -Runtime17TwoPc -Runtime17Mode RestoreStaging -Runtime17ReceiptRoot '<receipt-directory>'
```

This validates both backups and original directory identities before either
restore. It accepts only the recorded old/new file hashes (or missing files)
in a partial staging, and refuses unrelated intervening changes. It restores
native/UI staging bytes only, using unlink-before-copy. It can operate when
the current source is dirty; the old compiled bytes remain identified by their
backup manifest. If B is unavailable, restore stops: recover connectivity and
rerun; do not start either client. Never force-copy over unknown changes.

Then deploy the restored mod manually through Vortex on both PCs and close it.
Verify the actual restored files:

```powershell
pwsh -NoProfile -File .\build-and-deploy-dev.ps1 -Runtime17TwoPc -Runtime17Mode VerifyRestored -Runtime17ReceiptRoot '<receipt-directory>'
```

Only `RESTORED_VERIFIED` establishes that both actual native/UI installations
again match the pre-run backup. Installed backups are also retained as evidence
and for supervised recovery; the helper never bypasses Vortex to write Data.
Do not alter the private configuration between a run and its recovery: receipts
pin its hash. A changed installation identity or corrupted backup blocks recovery
until investigated. Restoration cannot undo unrelated external content changes.

## Local validation

```powershell
pwsh -NoProfile -File Tools/Scripts/Test-StreRuntime17Dev.ps1 -OutputRoot '<fresh-local-fixture-directory>'
```

The tests create only local Git repositories and fake payloads. xmake, process
enumeration and human Vortex actions are simulated for the full-flow tests;
filesystem, hashes, manifests, hardlinks, backup and restoration are real local
operations. No native executable is run. See the dated preparation evidence for
executed cases. A fixture PASS is not an audit/deployment on B.
