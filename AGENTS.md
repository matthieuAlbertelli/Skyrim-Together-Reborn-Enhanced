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
