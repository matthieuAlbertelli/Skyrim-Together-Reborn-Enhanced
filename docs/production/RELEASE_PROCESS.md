# STRE Versioning and Release Process

> **Status:** canonical release policy.

This document owns release mechanics. Product outcomes and release gates live in
[`ROADMAP.md`](../../ROADMAP.md); compatibility evidence lives in
[`COMPATIBILITY_MATRIX.md`](../testing/COMPATIBILITY_MATRIX.md); released changes
live in [`CHANGELOG.md`](../../CHANGELOG.md).

## Version model

STRE follows [Semantic Versioning 2.0.0](https://semver.org/):

- `MAJOR` changes when a stable public compatibility contract is broken;
- `MINOR` adds backward-compatible product capability;
- `PATCH` fixes backward-compatible defects;
- a pre-release suffix marks a build that does not yet carry the compatibility
  guarantee of the corresponding normal version.

Before `1.0.0`, minor releases may still change experimental contracts. Those
changes must nevertheless be explicit about protocol, catalog, persistence and
save compatibility; `0.x` is not permission for silent breakage.

### Development and pre-release meaning

| Identifier | Meaning |
|---|---|
| Development build | Untagged commit/PR artifact. Identified by Git revision; not a player release and may be short-lived. |
| `X.Y.Z-alpha.N` | Feature-incomplete, experimental player release. Save/protocol compatibility may change when declared. |
| `X.Y.Z-beta.N` | Intended feature scope is substantially present; stabilization, compatibility and broader validation remain. |
| `X.Y.Z-rc.N` | Release candidate for exactly `X.Y.Z`; product gate is complete and only release-blocking fixes/evidence may change. |
| `X.Y.Z` | Final immutable product release satisfying its Roadmap gate. |

No release calendar is implied. A new `0.x` Milestone is created only for an
achievable, dependency-coherent release slice.

## Product version versus build revision

- [`VERSION`](../../VERSION) contains the product SemVer only, for example
  `0.2.0-alpha.1`.
- `BUILD_COMMIT` / `git describe --tags` identify the exact binary revision used
  by client/server compatibility checks and diagnostics.
- `BuildVersion`, protocol schema versions, campaign/persistence schema versions,
  adapter versions and the ESP/plugin version are compatibility contracts, not
  substitutes for the product version.
- Two artifacts with the same product version but different commits are not the
  same tested build. Evidence records both the product version and full Git SHA.

An untagged build may display the nearest tag plus distance/SHA. That string is an
internal build identity and must not be advertised as a published product version.

## Branches and merge policy

- `main` is the protected integration and release source branch.
- Work uses short focused branches and pull requests targeting `main`.
- There is no canonical long-lived `dev` release branch.
- Release fixes follow the same PR and required-check policy; branch protection is
  not bypassed to manufacture a release.

## STRE tag namespace

New STRE releases use annotated tags:

```text
stre-v<SemVer>
```

Examples: `stre-v0.3.0-alpha.1`, `stre-v1.0.0-rc.1`, `stre-v1.0.0`.

The prefix distinguishes STRE releases from inherited TiltedEvolution tags,
including an existing upstream `v1.0.0`. Historical STRE tags
`v0.1.0-alpha.1` and `v0.2.0-alpha.1` are retained as immutable legacy names.
Never move or reuse a release tag.

## Compatibility review

Every release candidate records the effect on:

- client/server protocol and exact-build negotiation;
- Character Build/catalog version and CK/ESP records;
- campaign, character, room and WorldEntity persistence schemas;
- save/load, reconnect, downgrade and migration behavior;
- supported Skyrim runtime, SKSE, Address Library, native plugins and load-order
  constraints;
- upstream baseline and hook/ABI-sensitive changes.

If compatibility is broken, the release notes state the unsupported direction
(upgrade, downgrade, mixed client/server, old save, or old plugin), the failure
mode and any migration or recovery procedure. Silent best-effort loading of
incompatible canonical state is not acceptable.

## Release evidence gate

Before tagging:

1. The target Milestone and `ROADMAP.md` gate are satisfied.
2. Required PR checks are green on the accepted source.
3. Applicable native, serialization, UI, audit and clean-install checks pass.
4. Applicable solo and multiplayer matrices cover host/client/observer,
   save/load, post-seal join/replacement rejection, disconnect barriers,
   coordinated checkpoints, collective restore and failure/recovery behavior;
   WorldEntity validation separately covers late materialization where relevant.
5. `COMPATIBILITY_MATRIX.md`, installation instructions and support limitations
   match the candidate.
6. `VERSION`, `CHANGELOG.md`, release notes and package naming agree.
7. `UPSTREAM.md` records the exact full upstream base SHA, target product/tag and
   validation date. Because a tracked candidate file cannot contain the SHA of
   its own future merged commit, the final accepted release SHA is verified and
   recorded non-self-referentially by the immutable annotated tag and GitHub
   Release metadata before publication.
8. Licences, provenance, required external dependencies and package contents are
   reviewed.
9. Checksums are produced for distributed artifacts and a clean install is smoke
   tested.

Evidence must identify the full commit, environment, artifact/plugin hashes and
test roles. A successful build alone is not release acceptance.

## Release blockers

- Any open known `priority: P0` issue blocks a release candidate and final release.
- A newly reproduced crash, corruption, unrecoverable divergence or progression
  blocker is triaged before release even if it has not yet received a priority.
- A failing required check or missing campaign-critical evidence blocks tagging.
- P1–P3 issues require an explicit release disposition and player-facing known
  limitation where applicable; their existence is not automatically a release
  veto.
- Priority is never lowered merely to clear a gate.

## Changelog and release notes

[`CHANGELOG.md`](../../CHANGELOG.md) is the canonical STRE change history.

- User-visible STRE changes enter `[Unreleased]` with the implementing PR.
- A release PR moves the accepted entries under the exact product version/date.
- Upstream STR history is not copied wholesale; STRE documents inherited changes
  only when they affect its users, compatibility or risk.
- GitHub Release notes summarize the same accepted change set and link detailed
  compatibility, installation and known limitations rather than becoming a
  competing changelog.

## Tag, package and GitHub Release

1. Merge the reviewed release-preparation PR to `main`.
2. Verify the final main SHA and required checks.
3. Create and push the annotated `stre-v<SemVer>` tag on that exact SHA.
4. Let the playable-build workflow produce the Windows player artifact.
5. Verify archive layout, binaries, `VERSION`, installation guide, licences,
   plugin/scripts and checksums from the tagged artifact.
6. Create a GitHub Release for the same tag. Mark alpha/beta/rc versions as
   pre-releases; attach the verified player artifact and checksums.
7. Do not present source archives or transient Actions artifacts as the canonical
   player package.

The inherited Docker workflow/registry is not currently a canonical STRE player
release channel. It requires a separately approved ownership, registry and
support policy before being advertised as one.

## Rollback and recovery

Every release states:

- whether previous saves/persistence/catalog data can be opened;
- whether downgrade is supported;
- which migrations are irreversible;
- how to restore a backup or return to the previous supported release;
- which server/client mixtures fail closed.

A broken release tag remains immutable. Publish a new SemVer patch/pre-release or
withdraw the GitHub Release with a clear advisory; never repoint the tag.

## 0.4.0-alpha.1 packaging preparation (not publication)

The audited source delta from `stre-v0.3.0-alpha.1` to functional checkpoint
`1301f7e40de3185777a6f7f2e1c8bd5452b7ec6d` contains seven commits / 159 files:
previous-release bookkeeping, instanced-headquarters scope, Ilinalta's Vigil and
Valen prototype, automatic final appearance, explicit markers, individual seating,
and binding replay/admission fixes. CHANGELOG describes the cumulative outcome.

The previous public ZIP was inspected, not executed. Its SHA256 is
`cdf9244f64eadf9422a484a5369be84f17be19fe16b5fa3a16e5678bd662f053`, matching its
published sidecar. It has a Data-root layout: both ESPs, `scripts/`,
`SkyrimTogetherReborn/` executables/UI/dependencies, VERSION, INSTALLATION.md,
LICENSE and NOTICE.md. It is a playable distribution, not a Git source archive.

The existing `.github/workflows/windows-playable-build.yml` triggers on `stre-v*`
tag pushes or manual dispatch. It calls `windows.yml` with `build-mode: release`,
checks out full history, builds with xmake, installs runtime dependencies, builds
the production UI, and packages the exact checked-out `GameFiles/Skyrim/` tree.
It copies VERSION verbatim; build identity/artifact names use `git describe
--tags`, not VERSION. It uploads an Actions artifact, not a named GitHub Release
ZIP or checksum. It excludes PDB/LIB/EXP/test executables from playable binaries.
Release mode must not be confused with the separate MASTER branch macros.

After an independently authorized merge/tag and all release gates, use the
following procedure in a clean checkout of the final tagged main commit. These
commands are instructions for that later mission, not evidence of execution:

```powershell
$repo = 'matthieuAlbertelli/Skyrim-Together-Reborn-Enhanced'
$tag = 'stre-v0.4.0-alpha.1'
git fetch origin --tags
git rev-parse "$tag^{commit}"
git merge-base --is-ancestor "$tag^{commit}" origin/main
# Require the exact approved main SHA, VERSION=0.4.0-alpha.1, and clean checkout.
gh run list --repo $repo --workflow windows-playable-build.yml --branch $tag --json databaseId,headSha,status,conclusion
# Select the successful tag-triggered run with that exact SHA; do not use PR CI.
# If no run exists, a separately authorized manual dispatch can use:
# gh workflow run windows-playable-build.yml --repo $repo --ref $tag
$runId = Read-Host 'Verified successful tagged playable workflow run ID'
$out = Join-Path $env:TEMP ('stre-040-release-' + [guid]::NewGuid())
$pack = Join-Path $out 'package'
New-Item -ItemType Directory -Path $pack -Force | Out-Null
gh run download $runId --repo $repo --name "Skyrim Together Build ($tag)" --dir $pack
if ($LASTEXITCODE) { throw 'Artifact download failed' }
if ((Get-Content (Join-Path $pack 'VERSION') -Raw).Trim() -ne '0.4.0-alpha.1') { throw 'Wrong package version' }
# Perform the content/provenance and release smoke gates below before distribution.
$zip = Join-Path $out 'STRE-v0.4.0-alpha.1-windows-x64.zip'
Compress-Archive -Path (Join-Path $pack '*') -DestinationPath $zip -CompressionLevel Optimal
$hash = (Get-FileHash -LiteralPath $zip -Algorithm SHA256).Hash.ToLowerInvariant()
[IO.File]::WriteAllText((Join-Path $out 'STRE-v0.4.0-alpha.1-windows-x64.sha256'),
    "$hash  STRE-v0.4.0-alpha.1-windows-x64.zip`n", [Text.Encoding]::ASCII)
```

Verify workflow checkout SHA/tag in logs, exact tagged ESP/PEX/PSC hashes, the
19 manifest-managed files, both plugins, native executables/dependencies and UI,
and the versioned Valen NIF/DDS copied by the GameFiles boundary. The manifest
is an integrity subset, not a whitelist excluding versioned meshes/textures.
Run `audit_ck_packaging.py`, `audit_seat_approach_markers.py`, and the strict
record audit with `CK_RECORDS_M7_IMPLEMENTED.json` and `--reject-unexpected` on
that source. Validate extracted layout/checksum and perform the separately
approved clean-install/runtime smoke. Never substitute `_audit` or Debug binaries.
No byte-reproducible-build guarantee is inferred from this source-bound procedure.

Before publication, verify the external EEK dependency and clean-install texture
completeness. EEKs Fireplace Resource is installed separately; STRE redistributes
no EEK NIF/DDS/archive or Embers HD asset. The selected Vanilla Textured fireplace
has no direct Embers HD dependency. Inspect the tagged ZIP for absence of those
assets and verify the documented external configuration in a tagged-package
clean-install smoke, including all fireplace textures. The two EEK-path Whiterun
carving textures retain the maintainer-accepted alpha provenance limitation in
STATUS; do not infer vanilla origin or redistribution permission.

The 2026-09-19 maintainer visual smoke passed in the development environment; it
is not clean-install evidence. The CK manifest audit does not prove availability
of external textures. Review the packaged install guide for the new download and
version and complete compatibility/evidence gates before publication.

Version-reference classification during preparation: VERSION is advanced;
UPSTREAM gains the candidate record without rewriting the published identity.
README's version/current-public-build line, packaged installation guide and
reference compatibility matrix are prepared for 0.4.0-alpha.1 before tagging.
This preparation does not assert publication or tagged-package validation.
The 0.3 changelog and UPSTREAM's last-published identity remain historical.
The bug-report placeholder and RELEASE_PROCESS tag example are generic examples
and remain unchanged. Historical tags, SHAs and evidence are immutable.

Do not use GitHub's automatically generated Source code archives as the playable STRE build.
