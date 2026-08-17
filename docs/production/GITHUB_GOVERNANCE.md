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
| Git tag `stre-vX.Y.Z` | Immutable STRE released source version |
| GitHub Release | Distributed player release and release assets |

Markdown must not copy transient Project state, progress percentages, assignee state, or issue-by-issue completion. Link to the Project or Milestone instead. Update `STATUS.md` only when implementation or validation evidence changes; update `ROADMAP.md` only when product direction or a release gate changes.

## Intake and triage

Community bug reports use the STRE Issue Form. Reporters describe their environment, reproduction and impact; maintainers assign priority, area and origin after evidence review. Mods do not invalidate a report. `needs: vanilla-reproduction` requests a comparison run and is never a reason to close an otherwise useful report automatically.

Feature ideas begin in GitHub Discussions / Ideas. When an idea is accepted and can be expressed as an observable outcome, a maintainer creates an issue and links the discussion.

Blank community issues remain disabled. GitHub still presents the blank issue option to users with Write, Maintain or Admin access as a maintainers-only escape hatch for repository administration and planned work; maintainers then apply the same type, priority, area, origin and milestone rules.

Large systems use a parent issue with independently reviewable sub-issues. A shared architectural family should be created only after evidence supports it; similar symptoms must not be forced into separate ad-hoc fixes or a speculative common cause.

Use GitHub's native `blocked by` relationships for **completion dependencies**:
the blocked issue cannot satisfy its acceptance criteria until the blocker is
complete. Do not encode every collaboration or shared file as a dependency, do
not duplicate parent/sub-issue decomposition as blocker edges, and do not create
cycles for contracts that must be designed together. The maintained high-level
graph lives in [`DEPENDENCY_MAP.md`](DEPENDENCY_MAP.md).

## Label taxonomy

Labels are orthogonal metadata. Apply one type, one origin after audit, and every area that materially owns the work. Assign one priority to defects and operational risks after impact triage. Feature sequencing is represented by Project status, dependencies, Milestone, and Roadmap gates rather than by severity labels. Apply triage and community labels only when useful.

### Type

| Label | Meaning |
|---|---|
| `type: bug` | Observed behavior differs from the supported expected behavior |
| `type: feature` | Accepted product outcome or capability |
| `type: tech-debt` | Maintainability, testability or architecture work with no immediate product behavior change |
| `type: docs` | Documentation-only outcome |

### Priority

Priority labels express demonstrated severity or operational risk. They are not a general feature-ordering mechanism and are not required on ordinary feature or documentation issues. Priority is assigned by maintainers from demonstrated impact, not selected by community reporters.

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
- `area: campaign`
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

The `STRE — Road to v1.0.0` Project is the live operational board and the sole owner of transient workflow state.

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

The permanent inherited-STR stabilization track uses evidence-first symptom
issues. A release-specific parent may group those symptoms for triage and release
disposition without changing their priority or claiming a shared root cause.
When evidence supports a common authority, lifecycle, persistence,
reconciliation, or presentation contract, create one systemic design/fix unit
and retain the symptom issues for reproduction and acceptance evidence.

An issue moves to `Done` only when its implementation/review unit has landed and its acceptance evidence is recorded. Released state is represented separately by an immutable version tag and GitHub Release.

## Pull requests

Pull requests link an actionable issue unless the change is a trivial documentation or repository-maintenance correction. The PR template scales down for documentation-only work but requires authority, protocol/compatibility, validation, diagnostic, documentation and ADR impact to be considered whenever relevant.

PRs target `main`, use focused commits and remain unmerged until the applicable review policy, resolved conversations and required checks permit integration. Structural decisions use an ADR; feature-local behavior and protocols remain in the feature's canonical documentation directory.

Use `Refs #...` for incremental PRs that contribute to an issue without satisfying its full acceptance criteria. Use `Closes #...` only when the PR, once merged, completes the linked issue's complete acceptance contract. A parent or umbrella issue must not be closed merely because one independently reviewable increment lands.

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
6. The accepted source is tagged `stre-vX.Y.Z`.
7. A GitHub Release distributes the player artifact, release notes and required installation information.

A tag is never repointed to a different source state. Advanced or nightly artifacts are not presented as immutable player releases.

STRE uses the `stre-v` namespace because this fork retains upstream
TiltedEvolution tags such as `v1.0.0`; an unqualified future STRE `v1.0.0` tag
would collide with inherited history. Historical STRE tags
`v0.1.0-alpha.1` and `v0.2.0-alpha.1` remain immutable legacy names. Product
versions in `VERSION` and `CHANGELOG.md` do not include the Git namespace prefix.

## Remote configuration baseline

The repository configuration must preserve the policy above:

1. Issue Forms use the exact canonical labels; inherited aliases do not compete with the type taxonomy.
2. Discussions exposes `Ideas` and `Q&A`, and support/security routes point to their intended channels.
3. The `v1.0.0` Milestone links its release gate to `ROADMAP.md`.
4. `STRE — Road to v1.0.0` tracks accepted v1 issues with `Triage`, `Backlog`, `Ready`, `In progress`, `Review`, `Testing`, `Blocked` and `Done` states.
5. Inherited STR symptoms remain actionable issues. A finite v1 stabilization parent may organize them without replacing the permanent upstream track.
6. `main` retains the single-maintainer protection policy above, including required Windows/Linux checks, resolved conversations, and force-push/deletion protection.
7. Private vulnerability reporting remains enabled and the private advisory link in `SECURITY.md` remains valid.
8. Repository roles, default branch, merge methods and permissions remain consistent with the project charter.

Changes to these settings are reviewed like repository policy changes even when GitHub does not store them in the repository. The GitHub Project owns live item state; this section defines the durable configuration contract rather than mirroring current counts.
