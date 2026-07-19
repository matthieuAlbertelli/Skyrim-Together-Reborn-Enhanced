# Upstream Base and Integration Policy

## Audited baseline

The source archive exported on **19 July 2026** recorded:

- upstream repository: `tiltedphoques/TiltedEvolution`;
- upstream branch: `dev`;
- upstream base commit: `ca3f3234` (short SHA recorded by the audit);
- STRE branch: `main`;
- audited STRE head: `a9f55908`;
- declared STRE version: `0.1.0-alpha.1`.

Each release must replace these values with the exact full upstream SHA and the STRE head used to produce the build.

## Integration policy

- Keep STRE features isolated in dedicated services and directories where practical.
- Avoid unrelated formatting changes in upstream files.
- Record protocol-breaking, hook-sensitive and UI-invasive changes explicitly.
- Integrate upstream regularly rather than accumulating a large one-off merge.
- Run trading, preview and protocol regression tests before and after every upstream update.
- Update this file, the changelog and compatibility matrix in the same release PR.

See [Upstream strategy](docs/architecture/UPSTREAM_STRATEGY.md) for patch classification and review requirements.
