# Contributing to STRE

Thank you for contributing to Skyrim Together Reborn Enhanced.

## Start here

Read the documents relevant to your role:

1. [Project vision](docs/project/VISION.md)
2. [Project charter](docs/project/PROJECT_CHARTER.md)
3. [Team map](docs/project/TEAM_MAP.md)
4. [Current-state audit](docs/audit/CURRENT_STATE_AUDIT.md)
5. [Role-specific onboarding paths](docs/production/ONBOARDING_PATHS.md)

Open work packages are listed in [Open contributor missions](docs/production/OPEN_ROLES.md).

## Technical environment

The audited repository uses C++20, xmake, Angular 16, EnTT and the Skyrim Together Reborn client/server architecture. Alternate Start additionally requires Creation Kit and Papyrus work.

Follow [Building STRE](docs/development/BUILDING.md) before opening a code PR.

## Branch and commit conventions

Recommended branches:

- `feat/<topic>`
- `fix/<topic>`
- `refactor/<topic>`
- `docs/<topic>`
- `test/<topic>`
- `art/<topic>`
- `audio/<topic>`

Use Conventional Commit prefixes such as `feat:`, `fix:`, `refactor:`, `docs:`, `test:` and `chore:`.

## Pull-request requirements

A PR should state:

- the problem and expected user outcome;
- the modified subsystems;
- the authority for any new shared state;
- protocol, compatibility and upstream implications;
- tests added or the reason they are not applicable;
- diagnostic logs or traces introduced;
- screenshots/video for UI, Creation Kit, art or animation work;
- the ADR affected by structural decisions.

## C++ and networking

- Follow [Code guidelines](CODE_GUIDELINES.md).
- Keep domain, protocol and Skyrim adaptation concerns separate.
- Do not mutate authoritative shared state locally before validation.
- Make handlers idempotent when retransmission or duplicate activation is possible.
- Bound collections during deserialization.
- Log identifiers, revisions and rejection reasons for critical transitions.
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
- Test the mod without STRE running.
- Emit multiplayer intents through the bridge and apply consequences only after canonical validation.

## Art and audio

- Record asset provenance and license.
- Keep source files separate from game-ready exports.
- Agree technical budgets before production.
- Follow the naming and delivery rules in the relevant pipeline document.

## Definition of Done

A contribution is complete when the result is demonstrable, relevant tests pass, failure cases are covered, logs are useful, documentation is updated and dependent teams have reviewed the integration.
