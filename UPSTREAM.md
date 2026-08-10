# Upstream Base

This file records **baseline identity only**. Integration policy belongs in [`docs/architecture/UPSTREAM_STRATEGY.md`](docs/architecture/UPSTREAM_STRATEGY.md).

## Audited baseline

The source audit exported on **19 July 2026** recorded:

- upstream repository: `tiltedphoques/TiltedEvolution`;
- upstream branch: `dev`;
- upstream base commit: `ca3f3234` (short SHA recorded by the historical audit);
- audited STRE head: `a9f55908`;
- declared STRE version: `0.1.0-alpha.1`.

## Release rule

Each release must replace these historical values with:

- the exact full upstream base SHA;
- the exact STRE release SHA;
- the release/tag identifier;
- the validation date.

Do not duplicate merge/rebase rules here. See [Upstream strategy](docs/architecture/UPSTREAM_STRATEGY.md).
