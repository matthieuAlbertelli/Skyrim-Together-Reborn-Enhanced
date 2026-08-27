# Upstream Base

This file records **baseline identity only**. Integration policy belongs in [`docs/architecture/UPSTREAM_STRATEGY.md`](docs/architecture/UPSTREAM_STRATEGY.md).

## Carried upstream baseline

The current STRE history carries:

- upstream repository: `tiltedphoques/TiltedEvolution`;
- upstream branch: `dev`;
- exact upstream base commit: `ca3f32348d3217e766b45afa9cce8a645f3a6444`;
- previous published STRE player release: `v0.2.0-alpha.1`;
- exact release commit: `edbbc487fbed1f0fa61ef9c05dad664d2368920d`;
- release candidate product version: `0.3.0-alpha.1`;
- intended immutable tag: `stre-v0.3.0-alpha.1`;
- carried baseline verified for the candidate: **27 August 2026**.

The release-preparation commit cannot contain its own future merged/tagged SHA.
Until the PR is accepted, `v0.2.0-alpha.1` and its exact commit above remain the
latest published release record. After merge, the annotated tag and GitHub
Release must record and resolve to the exact accepted `main` SHA before any
artifact is published.

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
