# Documentation Maintenance

> **Status:** canonical repository policy.

## Goals

The documentation tree must remain navigable, non-duplicated and explicit about whether a statement describes implemented code or a target design.

## Canonical locations

- public entry points: repository root (`README.md`, `ROADMAP.md`, `CONTRIBUTING.md`);
- project framing: `docs/project/`;
- architecture and ADRs: `docs/architecture/`;
- one directory per feature: `docs/features/<feature>/`;
- narrative/art/audio briefs: their dedicated directories;
- planning, responsibilities and open missions: `docs/production/`;
- build/development instructions: `docs/development/`;
- inherited upstream-specific notes: `docs/upstream/`.

## No-duplicate rules

- Do not create both `docs/features/foo.md` and `docs/features/foo/README.md`.
- Do not create a second architecture summary when an existing canonical document can be extended.
- ADRs are never silently overwritten; mark them superseded or add a follow-up ADR.
- One-time migration instructions do not remain in the canonical documentation after integration.

## Status headers

Design documents must state one of: Implemented, Accepted, Proposed or Open. Mixed documents must clearly identify which sections are observations and which are targets.

## Link discipline

- Prefer relative Markdown links for repository documents.
- Run the documentation link check before merging.
- When moving a document, update inbound links in the same commit.
- Preserve git history through `git mv` when applying manual reorganizations.

## Review ownership

- product framing: project lead;
- architecture/ADRs: STRE Core or Mod Integration lead;
- CK documents: CK lead;
- narrative/art/audio: corresponding discipline lead;
- tests: QA lead;
- public entry points: project lead plus one technical reviewer.
