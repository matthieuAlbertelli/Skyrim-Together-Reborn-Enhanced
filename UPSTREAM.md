# Upstream Base

This file records **baseline identity only**. Integration policy belongs in [`docs/architecture/UPSTREAM_STRATEGY.md`](docs/architecture/UPSTREAM_STRATEGY.md).

## Carried upstream baseline

The current STRE history carries:

- upstream repository: `tiltedphoques/TiltedEvolution`;
- upstream branch: `dev`;
- exact upstream base commit, carried unchanged for this release:
  `ca3f32348d3217e766b45afa9cce8a645f3a6444`;
- latest published STRE player release: `stre-v0.3.0-alpha.1`;
- exact released commit: `65387e7e59cf5e2fb3a04f12888ef671268f0323`;
- immutable annotated tag object: `f822346c92a41f2092e2c406ae10b5e1d2f39dc5`;
- product version: `0.3.0-alpha.1`;
- published: **28 August 2026**;
- baseline verified for this release: **27 August 2026**;
- previous published STRE player release: `v0.2.0-alpha.1`;
- previous release commit: `edbbc487fbed1f0fa61ef9c05dad664d2368920d`.

The release-preparation commit could not self-record its future merged SHA.
This post-publication record captures the accepted commit and annotated tag
object. The immutable annotated tag and GitHub Release are the authoritative
release identity; a published tag must never be repointed.

At that verification, current `upstream/dev`
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
alpha tags above are immutable legacy names; TiltedEvolution already owns other
unprefixed `v*` names in this repository.

Do not duplicate merge/rebase rules here. See [Upstream strategy](docs/architecture/UPSTREAM_STRATEGY.md).
