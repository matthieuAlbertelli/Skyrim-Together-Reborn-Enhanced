# GitHub Governance

> **Status:** canonical repository policy for GitHub planning, triage and delivery.

## Purpose

GitHub provides STRE's live operational view without becoming a second product specification or implementation-status document. This policy defines what each artifact owns and the minimum taxonomy maintainers use to keep planning actionable.

## Sources of truth

| Artifact | Owns |
|---|---|
| [`ROADMAP.md`](../../ROADMAP.md) | Product direction, release objectives and release gates |
| [`docs/project/STATUS.md`](../project/STATUS.md) | Implemented and validated repository state |
| GitHub Project | Live work state and operational progress |
| GitHub Milestone | Target product version |
| GitHub Issue | One bug, feature, technical-debt item, documentation outcome, or other actionable unit |
| Parent issue and sub-issues | Decomposition of a large system or epic |
| Pull request | Implementation and review unit |
| Git tag `vX.Y.Z` | Immutable released source version |
| GitHub Release | Distributed player release and release assets |

Markdown must not copy transient Project state, progress percentages, assignee state, or issue-by-issue completion. Link to the Project or Milestone instead. Update `STATUS.md` only when implementation or validation evidence changes; update `ROADMAP.md` only when product direction or a release gate changes.

## Intake and triage

Community bug reports use the STRE Issue Form. Reporters describe their environment, reproduction and impact; maintainers assign priority, area and origin after evidence review. Mods do not invalidate a report. `needs: vanilla-reproduction` requests a comparison run and is never a reason to close an otherwise useful report automatically.

Feature ideas begin in GitHub Discussions / Ideas. When an idea is accepted and can be expressed as an observable outcome, a maintainer creates an issue and links the discussion.

Blank community issues remain disabled. GitHub still presents the blank issue option to users with Write, Maintain or Admin access as a maintainers-only escape hatch for repository administration and planned work; maintainers then apply the same type, priority, area, origin and milestone rules.

Large systems use a parent issue with independently reviewable sub-issues. A shared architectural family should be created only after evidence supports it; similar symptoms must not be forced into separate ad-hoc fixes or a speculative common cause.

## Label taxonomy

Labels are orthogonal metadata. Apply one type, one priority after triage, one origin after audit, and every area that materially owns the work. Apply triage and community labels only when useful.

### Type

| Label | Meaning |
|---|---|
| `type: bug` | Observed behavior differs from the supported expected behavior |
| `type: feature` | Accepted product outcome or capability |
| `type: tech-debt` | Maintainability, testability or architecture work with no immediate product behavior change |
| `type: docs` | Documentation-only outcome |

### Priority

Priority is assigned by maintainers from demonstrated impact, not selected by community reporters.

| Label | Meaning |
|---|---|
| `priority: P0` | Release or campaign blocker: impossible progression, serious state corruption/loss, major crash, unrecoverable multiplayer divergence, or a fundamental mechanic that is unusable |
| `priority: P1` | Major multiplayer defect with meaningful gameplay impact, where campaign recovery or a workaround remains possible |
| `priority: P2` | Normal defect with limited impact |
| `priority: P3` | Minor or cosmetic defect |

### Area

Use the smallest useful set from:

- `area: networking`
- `area: world-sync`
- `area: actors`
- `area: mounts`
- `area: alternate-start`
- `area: classes`
- `area: quests`
- `area: valen`
- `area: housing`
- `area: trading`
- `area: ui`
- `area: build-ci`
- `area: docs`

### Origin

| Label | Meaning |
|---|---|
| `origin: STRE` | Introduced by or owned in STRE-specific code/content |
| `origin: upstream-STR` | Confirmed as inherited from Skyrim Together Reborn |

Do not assign origin from a reporter's guess. A symptom first observed in STRE remains unlabelled until the audit identifies ownership.

### Triage

- `needs: reproduction`
- `needs: logs`
- `needs: save`
- `needs: vanilla-reproduction`

### Community

- `good first issue`
- `help wanted`

Do not create labels such as `status: todo`, `in progress`, or `done`. Workflow state belongs to the GitHub Project.

## Project and milestone flow

The `STRE road to v1.0.0` Project is the live operational board and the sole owner of transient workflow state.

| State | Meaning |
|---|---|
| `Triage` | Newly reported or unclassified work; evidence and ownership are still being assessed. |
| `Backlog` | Accepted work that is not currently scheduled for implementation. |
| `Ready` | Work that is sufficiently specified and ready to be worked on. |
| `In progress` | Implementation is actively underway. |
| `Review` | Implementation is in pull-request/review stage. |
| `Testing` | Implementation exists and is undergoing automated, manual or in-game validation. |
| `Blocked` | Work cannot progress because of an identified dependency or blocker. |
| `Done` | Accepted implementation and required validation have landed. |

Use built-in issue fields plus Milestone, labels and parent/sub-issue relationships instead of duplicating priority, area, target version or workflow state in custom fields or Markdown. Do not publish progress percentages or mirror the live board in repository documents.

The `v1.0.0` Milestone contains work required by the release definition in `ROADMAP.md`. Earlier `0.x` milestones are deliberately provisional and should be created only when their scope is justified by an achievable integration/release slice.

An issue moves to `Done` only when its implementation/review unit has landed and its acceptance evidence is recorded. Released state is represented separately by an immutable version tag and GitHub Release.

## Pull requests

Pull requests link an actionable issue unless the change is a trivial documentation or repository-maintenance correction. The PR template scales down for documentation-only work but requires authority, protocol/compatibility, validation, diagnostic, documentation and ADR impact to be considered whenever relevant.

PRs target `main`, use focused commits and remain unmerged until the applicable review policy, resolved conversations and required checks permit integration. Structural decisions use an ADR; feature-local behavior and protocols remain in the feature's canonical documentation directory.

## Main branch protection

Branch protection must preserve PR traceability and required CI without making the repository impossible to maintain.

During the current single-maintainer phase:

- changes reach `main` through pull requests;
- relevant required CI/status checks must pass;
- review conversations must be resolved;
- force-pushes to `main` are prohibited;
- deletion of `main` is prohibited;
- direct normal pushes to `main` are prevented where GitHub rules permit an appropriate maintainer/admin bypass for exceptional recovery;
- automatic merge is not required;
- independent approval is not a mandatory branch-rule requirement, so the sole maintainer may merge their own reviewed and tested PR after the safeguards above pass.

When at least one additional active maintainer or reviewer can provide meaningful review without blocking the repository, require at least one independent approval before merge. Keep this policy focused on PR traceability, CI and review quality rather than adding unnecessary role layers.

## Release flow

1. Product direction and gates are accepted in `ROADMAP.md`.
2. Actionable work is assigned to a version Milestone and tracked live in the Project.
3. Pull requests implement and validate the work.
4. `STATUS.md` records the resulting implemented/validated state.
5. A release candidate is evaluated against the Roadmap gates and compatibility evidence.
6. The accepted source is tagged `vX.Y.Z`.
7. A GitHub Release distributes the player artifact, release notes and required installation information.

A tag is never repointed to a different source state. Advanced or nightly artifacts are not presented as immutable player releases.

## One-time GitHub UI setup

Repository maintainers must complete these settings outside versioned files:

1. Create any missing labels in this taxonomy with the exact spelling above before relying on Issue Form auto-labelling. Migrate open issues, then retire inherited aliases such as `bug`, `documentation`, `enhancement` and `question` so they do not compete with the type taxonomy; retain existing exact matches such as `good first issue` and `help wanted`.
2. Enable GitHub Discussions and create `Ideas` and `Q&A` categories so the Issue chooser and support links resolve.
3. Create the `v1.0.0` Milestone and link its description to `ROADMAP.md`; avoid fabricated `0.x` scopes.
4. Create the `STRE road to v1.0.0` Project, link it to the repository, configure `Triage`, `Backlog`, `Ready`, `In progress`, `Review`, `Testing`, `Blocked` and `Done`, and add the accepted v1 issues.
5. Create one issue per observable STR stabilization symptom listed in `ROADMAP.md`. Use parent/sub-issues only after investigation supports a common architectural family, and assign final priority from reproduced campaign impact.
6. Protect `main` using the current single-maintainer policy above: require pull requests and relevant successful checks, require resolved conversations, prohibit force-push/deletion, and prevent direct normal pushes with an appropriate exceptional-recovery bypass. Add mandatory independent approval when another active reviewer is available. Automatic merge is not required.
7. Enable private vulnerability reporting and verify the private advisory link in `SECURITY.md`.
8. Review repository roles, default branch, merge methods and issue/Discussion permissions against the project charter.

Changes to these settings should be reviewed like repository policy changes even when GitHub does not store them in the repository.
