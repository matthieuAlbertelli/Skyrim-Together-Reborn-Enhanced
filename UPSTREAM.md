# Upstream Base

This file records **baseline identity only**. Integration policy belongs in [`docs/architecture/UPSTREAM_STRATEGY.md`](docs/architecture/UPSTREAM_STRATEGY.md).

## Carried upstream baseline

The current STRE history carries:

- upstream repository: `tiltedphoques/TiltedEvolution`;
- upstream branch: `dev`;
- exact upstream base commit, carried unchanged for this release:
  `ca3f32348d3217e766b45afa9cce8a645f3a6444`;
- latest published STRE player release: `stre-v0.4.0-alpha.1`;
- exact released commit: `6dfba1594e35852d106be0b4785dd5280a58fa8d`;
- immutable annotated tag object: `d6c8b8a356838e03007a67ffbd6f74857d451544`;
- product version: `0.4.0-alpha.1`;
- published: **19 September 2026**;
- candidate identity/ancestry reviewed: **19 September 2026**.

Previous published STRE player release (historical identity):

- tag: `stre-v0.3.0-alpha.1`;
- commit: `65387e7e59cf5e2fb3a04f12888ef671268f0323`;
- annotated tag object: `f822346c92a41f2092e2c406ae10b5e1d2f39dc5`;
- published: **28 August 2026**;
- baseline verified for that release: **27 August 2026**.

The earlier `v0.2.0-alpha.1` release remains at commit
`edbbc487fbed1f0fa61ef9c05dad664d2368920d`.

The release-preparation commit could not self-record its future merged SHA.
This post-publication record captures the accepted commit and annotated tag
object. The immutable annotated tag and GitHub Release are the authoritative
release identity; a published tag must never be repointed.

At the 27 August 2026 verification, then-current `upstream/dev`
(`9d81ef07d68e4bb2bd94fca246e798a564b7fb92`) had the same source tree as the
recorded base but different parent history. The recorded full base SHA remains
the ancestry anchor; a merge-base against a history-rewritten upstream ref must
not silently replace it. A future upstream integration records the reviewed new
base explicitly.

## Release rule

Each release candidate updates this record with:

- the exact full upstream base SHA;
- the intended release/tag identifier and product version;
- the validation date.

The final exact STRE release SHA is verified after the release-preparation PR is
merged and is recorded non-self-referentially by the immutable annotated tag and
GitHub Release metadata. A later repository update may roll the latest-published
summary forward; it must never guess the future release SHA or repoint a tag.

New STRE releases use the `stre-v<SemVer>` namespace defined in the
[release process](docs/production/RELEASE_PROCESS.md). The two unprefixed STRE
alpha tags `v0.1.0-alpha.1` and `v0.2.0-alpha.1` are immutable legacy names;
TiltedEvolution already owns other
unprefixed `v*` names in this repository.

Do not duplicate merge/rebase rules here. See [Upstream strategy](docs/architecture/UPSTREAM_STRATEGY.md).

## Published 0.4.0-alpha.1 identity (2026-09-19)

- Tag: `stre-v0.4.0-alpha.1`.
- Release commit: `6dfba1594e35852d106be0b4785dd5280a58fa8d`.
- Annotated tag object: `d6c8b8a356838e03007a67ffbd6f74857d451544`.
- [GitHub Release](https://github.com/matthieuAlbertelli/Skyrim-Together-Reborn-Enhanced/releases/tag/stre-v0.4.0-alpha.1)
  published: `2026-09-19T10:57:15Z` (`prerelease=true`, `draft=false`).
- [Playable workflow 35437245976](https://github.com/matthieuAlbertelli/Skyrim-Together-Reborn-Enhanced/actions/runs/35437245976):
  completed/success on the release commit.
- ZIP: `STRE-v0.4.0-alpha.1-windows-x64.zip`.
- ZIP SHA256: `57dcc435ab6728782a4e4242e28e7286dff7fc75bbb1e5b7ecc6e4c55f66bf3c`.
- Tagged clean-install human smoke: **PASS**, for this exact ZIP; evidence and
  unchanged limitations are recorded in [STATUS](docs/project/STATUS.md#stre-040-alpha1-published-release-checkpoint--pass-2026-09-19).

The carried upstream base remained unchanged:
`ca3f32348d3217e766b45afa9cce8a645f3a6444`; no upstream integration occurred in
this release. The immutable annotated tag and published GitHub Release are the
authoritative identity of this player release. Subsequent documentation commits
on main do not move the tag or replace any published asset.
