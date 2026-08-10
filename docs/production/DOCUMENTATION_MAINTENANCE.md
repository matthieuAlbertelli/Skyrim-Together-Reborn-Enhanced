# Documentation Maintenance

> **Status:** canonical repository policy.

## Goal

Keep the documentation navigable, non-duplicated and explicit about whether a statement is current implementation, accepted design, proposal or history.

## Single-source-of-truth rule

Mutable information must have exactly one canonical location.

| Information | Source of truth |
|---|---|
| Current implementation/validation | `docs/project/STATUS.md` |
| Priority/progress | `ROADMAP.md` |
| Release history | `CHANGELOG.md` |
| Compatibility | `docs/testing/COMPATIBILITY_MATRIX.md` |
| Technical risks | `docs/production/RISK_REGISTER.md` |
| Upstream baseline SHA/version | `UPSTREAM.md` |
| Feature-specific protocol/tests/design | `docs/features/<feature>/` |
| Cross-feature architecture policy | `docs/architecture/` + ADRs |

Other documents link to these sources instead of copying their mutable content.

## Canonical locations

- public entry points: repository root;
- project framing/current status: `docs/project/`;
- architecture and ADRs: `docs/architecture/`;
- one directory per feature: `docs/features/<feature>/`;
- global testing policy/runbook/compatibility: `docs/testing/`;
- planning/support processes: `docs/production/`;
- build instructions: `docs/development/`;
- narrative/art/audio: dedicated directories;
- dated evidence no longer current: `docs/audit/` or a feature-local `history/`.

## Feature boundary

A feature must not maintain a second parallel tree under `docs/architecture/` or `docs/testing/`.

Cross-feature documents may define common rules, but concrete feature messages, state machines, test cases and known limitations live with the feature.

## Historical documents

A dated audit or completed milestone may be preserved when useful, but:

- mark it `Historical`;
- place it under a `history/` or audit archive;
- remove it from current-state navigation unless traceability requires the link;
- never update it to pretend it is current.

Git history is sufficient for one-time migration/hotfix instructions that have no continuing documentation value.

## No-duplicate rules

- no `docs/features/foo.md` next to `docs/features/foo/README.md`;
- no second progress list outside `ROADMAP.md`;
- no second current-state summary outside `docs/project/STATUS.md`;
- no copy of a feature protocol in `architecture/NETWORK_PROTOCOL.md`;
- no copy of feature acceptance scenarios in `docs/testing/ACCEPTANCE_TESTS.md`;
- ADRs are superseded by follow-up ADRs, not silently rewritten;
- one-time overlay/apply instructions are removed after integration.

## Link discipline

- relative Markdown links;
- update inbound links in the same change as a move;
- prefer `git mv` for reorganizations;
- run a documentation link check before merge.

## Review checklist

A docs change is complete when:

- the canonical source is obvious;
- status labels are correct;
- no stale parallel statement remains;
- moved paths have updated inbound links;
- historical evidence is clearly separated from current truth.
