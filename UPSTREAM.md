# Upstream Base

This file records **baseline identity only**. Integration policy belongs in [`docs/architecture/UPSTREAM_STRATEGY.md`](docs/architecture/UPSTREAM_STRATEGY.md).

## Carried upstream baseline

The current STRE history carries:

- upstream repository: `tiltedphoques/TiltedEvolution`;
- upstream branch: `dev`;
- exact upstream base commit: `ca3f32348d3217e766b45afa9cce8a645f3a6444`;
- latest recorded STRE player release: `v0.2.0-alpha.1`;
- exact release commit: `edbbc487fbed1f0fa61ef9c05dad664d2368920d`;
- declared product version: `0.2.0-alpha.1`;
- baseline verified: **12 August 2026**.

At that verification, current `upstream/dev`
(`9d81ef07d68e4bb2bd94fca246e798a564b7fb92`) had the same source tree as the
recorded base but different parent history. The recorded full base SHA remains
the ancestry anchor; a merge-base against a history-rewritten upstream ref must
not silently replace it. A future upstream integration records the reviewed new
base explicitly.

## Release rule

Each release updates this record with:

- the exact full upstream base SHA;
- the exact STRE release SHA;
- the release/tag identifier;
- the validation date.

New STRE releases use the `stre-v<SemVer>` namespace defined in the
[release process](docs/production/RELEASE_PROCESS.md). The two unprefixed STRE
alpha tags above are immutable legacy names; TiltedEvolution already owns other
unprefixed `v*` names in this repository.

Do not duplicate merge/rebase rules here. See [Upstream strategy](docs/architecture/UPSTREAM_STRATEGY.md).
