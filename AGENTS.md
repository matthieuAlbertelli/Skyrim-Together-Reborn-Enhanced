# Repository agent guidance

Before changing this repository, read `CONTRIBUTING.md`, `ROADMAP.md`,
`docs/project/STATUS.md`, the ADRs for the affected domain, and the feature
documentation that owns the contract. Keep planning, implementation truth, and
historical evidence in their canonical documents.

## Campaign v1 invariants

Follow [ADR-0018](docs/architecture/ADRs/ADR-0018-fixed-roster-coordinated-checkpoint-recovery.md):

- the STRE server is persistent authority for shared STRE state;
- the Session Manager/host is not persistence authority;
- the campaign roster is sealed and immutable after start;
- the full roster is required while a campaign progresses;
- campaign late join and player replacement are forbidden in v1;
- disconnect enters recovery lock;
- recovery collectively restores the same committed `CampaignCheckpoint`;
- each roster member restores its own native Skyrim `.ess`;
- `.ess` restores local Skyrim/Papyrus/quest runtime, not shared-state authority;
- never reconstruct vanilla quest/Papyrus state from quest stages alone;
- never add partial-roster catch-up unless a later ADR supersedes this rule;
- campaign late join is distinct from WorldEntity late materialization.

`docs/project/STATUS.md` is the only source of truth for what is implemented and
validated.

## CK modding toolchain

Follow [the modding runbook](docs/development/MODDING_TOOLCHAIN.md) and use
`Tools/Scripts/Invoke-StreModding.ps1` for supported AutoMod operations.

- Target Skyrim SE Steam **1.6.1170.0**; no runtime migration or ESL conversion.
- Require AutoMod `--json`, parse Boolean `success`, and stop on any failure.
- Run `--dry-run` before writes when supported; verify unchanged file hashes.
- Run `papyrus status` before the first Papyrus operation; verify required tools.
- Prefix new records with `STRE_`.
- Initially only `STRE_AlternateStart.esp` is writable; `SkyrimTogether.esp`
  is read-only for this toolchain, including in xEdit.
- Use clean dedicated feature branches, backups and isolated working copies.
- Require official xEdit normalization, saved-file/error validation and the
  expected master order after every AutoMod write. Never fabricate an attestation.
- Data is not final authority. Use the existing CK importer and preserve the
  different authority of ordinary STRE scripts and CK-generated artifacts.
