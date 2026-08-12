# Contributing to STRE

Thank you for contributing to Skyrim Together Reborn Enhanced.

## Start here

Read the documents relevant to your role:

1. [Project vision](docs/project/VISION.md)
2. [Project charter](docs/project/PROJECT_CHARTER.md)
3. [Current project status](docs/project/STATUS.md)
4. [System overview](docs/architecture/SYSTEM_OVERVIEW.md)
5. [GitHub governance](docs/production/GITHUB_GOVERNANCE.md)
6. [Role-specific onboarding paths](docs/production/ONBOARDING_PATHS.md)

[Contributor role profiles](docs/production/OPEN_ROLES.md) describe recurring skills and contribution areas. Live work belongs to the GitHub Project and actionable issues labelled `help wanted` or `good first issue`.

## Technical environment

The repository uses C++20, xmake, Angular 16, EnTT and the Skyrim Together Reborn client/server architecture. Alternate Start additionally requires Creation Kit and Papyrus work.

Follow [Building STRE](docs/development/BUILDING.md) before opening a code PR.

## Branch and commit conventions

Recommended branches:

- `feat/<topic>`
- `fix/<topic>`
- `refactor/<topic>`
- `docs/<topic>`
- `test/<topic>`
- `chore/<topic>`
- `art/<topic>`
- `audio/<topic>`

Use Conventional Commit prefixes such as `feat:`, `fix:`, `refactor:`, `docs:`, `test:` and `chore:`.

## Issues, ideas and priority

- Report reproducible defects through the STRE bug Issue Form. Modded reports are welcome when the environment is described.
- Start feature proposals in [GitHub Discussions / Ideas](https://github.com/matthieuAlbertelli/Skyrim-Together-Reborn-Enhanced/discussions/categories/ideas); accepted ideas become actionable issues.
- Do not assign your own P0–P3 priority or speculate about the owning subsystem. Maintainers apply the label taxonomy after triage.
- Use parent issues and sub-issues to decompose large systems without turning a PR into an epic.

See [GitHub governance](docs/production/GITHUB_GOVERNANCE.md) for artifact ownership, labels, Projects, Milestones and release flow. For setup help, see [SUPPORT.md](SUPPORT.md); report vulnerabilities through [SECURITY.md](SECURITY.md), never a public issue.

## Development handoff contract

An implementation handoff starts from one canonical GitHub issue. Before coding,
the issue and its linked design must make the following discoverable:

- the observable outcome and the scope that is deliberately excluded;
- current evidence, accepted behavior and the canonical specifications/ADRs;
- the owning domain, authority boundary and relevant upstream constraint;
- parent/sub-issue structure and explicit blockers;
- acceptance criteria, failure behavior and compatibility or migration impact;
- the automated checks and manual host/client/observer evidence required;
- documentation, changelog, version/schema and release-gate impact.

The implementer confirms a clean branch from current `origin/main`, reads the
linked sources, and moves the issue through the Project workflow without copying
that transient state into Markdown. Work stays on a focused branch, uses
Conventional Commits and reaches `main` through a pull request with the required
checks and resolved review conversations. Structural decisions add or supersede
an ADR; they do not silently rewrite one. A handoff is incomplete when the next
person must infer authority, acceptance, dependencies or validation from code.

The issue owns live delivery status. The PR owns the proposed diff and evidence.
Canonical documents own durable behavior and policy.

## Pull-request requirements

A PR should state:

- the related issue, or why a trivial documentation-only change does not need one;
- the problem and expected user outcome;
- the solution and important trade-offs;
- the modified subsystems;
- the authority for any new shared state;
- protocol, compatibility and upstream implications;
- tests added or the reason they are not applicable;
- manual in-game validation across relevant host/client/observer roles;
- diagnostic logs or traces introduced;
- screenshots/video for UI, Creation Kit, art or animation work;
- canonical documentation impact;
- the ADR affected by structural decisions.

## C++ and networking

- Follow [Code guidelines](CODE_GUIDELINES.md).
- Keep domain, protocol and Skyrim adaptation concerns separate.
- Do not mutate authoritative shared state locally before validation.
- Make handlers idempotent when retransmission or duplicate activation is possible.
- Bound collections during deserialization.
- Marshal Skyrim engine mutations onto an engine-safe/game-update path.
- Log identifiers, revisions and rejection reasons for critical transitions.
- Fail closed when a transfer path cannot preserve required instance metadata.
- Treat distributed inventory exchange as a saga with reconciliation, not as an ACID transaction.

## Angular / CEF

- Use typed event contracts.
- Do not promote legacy debug commands into public APIs.
- Clean up observers, subscriptions and event listeners.
- Validate all arguments crossing the CEF/native boundary.
- Test keyboard, controller and multiple resolutions.

## Creation Kit / Papyrus

- Prefix new records with `STRE_`.
- Avoid modifying vanilla records unless the dependency is documented.
- Use configured script properties rather than hard-coded plugin FormIDs.
- Test the mod without STRE running when the feature has a standalone path.
- Emit multiplayer intents through the bridge and apply shared consequences only after canonical validation.

## Documentation

Follow [Documentation maintenance](docs/production/DOCUMENTATION_MAINTENANCE.md).

In particular:

- do not create a second source of truth for status, roadmap, protocol or tests;
- feature-specific details live under `docs/features/<feature>/`;
- cross-feature architectural decisions live under `docs/architecture/` and ADRs;
- historical audits/jalons must be clearly archived rather than presented as current state.

## Definition of Done

A contribution is complete when the result is demonstrable, relevant tests pass, failure cases are covered, logs are useful, documentation is updated and dependent teams have reviewed the integration.
