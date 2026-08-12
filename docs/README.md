# STRE Documentation

This directory is the canonical documentation portal for Skyrim Together Reborn Enhanced.

## Status vocabulary

- **Implemented / Implémenté** — confirmed in current source.
- **Validated in game / Validé en jeu** — exercised in Skyrim; scope/date must be stated.
- **Accepted / Décidé** — product or architecture direction selected for STRE.
- **Proposed / Proposé** — recommended design awaiting implementation or ratification.
- **Open / À valider** — unresolved decision or prototype requirement.
- **Historical / Historique** — dated evidence preserved for traceability, not current truth.

## Sources of truth

| Information | Canonical location |
|---|---|
| Public introduction | [`README.md`](../README.md) |
| Player installation | [`user/INSTALLATION.md`](user/INSTALLATION.md) |
| Current implementation/validation status | [`project/STATUS.md`](project/STATUS.md) |
| Product vision | [`project/VISION.md`](project/VISION.md) |
| Project scope/governance | [`project/PROJECT_CHARTER.md`](project/PROJECT_CHARTER.md) |
| Product direction/release objectives/gates | [`ROADMAP.md`](../ROADMAP.md) |
| Live issue/work progress and GitHub operations | GitHub Project + [`production/GITHUB_GOVERNANCE.md`](production/GITHUB_GOVERNANCE.md) |
| Development handoff contract | [`CONTRIBUTING.md`](../CONTRIBUTING.md#development-handoff-contract) |
| Versioning, tags and release mechanics | [`production/RELEASE_PROCESS.md`](production/RELEASE_PROCESS.md) |
| Release history | [`CHANGELOG.md`](../CHANGELOG.md) |
| Cross-feature architecture | [`architecture/`](architecture/) |
| Architecture decision policy/index | [`architecture/ADRs/README.md`](architecture/ADRs/README.md) + [`production/DECISION_REGISTER.md`](production/DECISION_REGISTER.md) |
| Feature behavior/design | [`features/<feature>/`](features/) |
| Cross-feature test policy | [`testing/`](testing/) |
| Compatibility | [`testing/COMPATIBILITY_MATRIX.md`](testing/COMPATIBILITY_MATRIX.md) |
| Work decomposition | [`production/WORK_BREAKDOWN_STRUCTURE.md`](production/WORK_BREAKDOWN_STRUCTURE.md) |
| Technical risks | [`production/RISK_REGISTER.md`](production/RISK_REGISTER.md) |
| Historical audits | [`audit/`](audit/) |
| Upstream baseline | [`UPSTREAM.md`](../UPSTREAM.md) |
| Upstream integration policy | [`architecture/UPSTREAM_STRATEGY.md`](architecture/UPSTREAM_STRATEGY.md) |

Other documents should **link to these sources instead of restating mutable status or planning data**.

## Players

- [Install and launch STRE](user/INSTALLATION.md)
- [Compatibility matrix](testing/COMPATIBILITY_MATRIX.md)
- [Current project status](project/STATUS.md)

Player-facing installation details belong in `docs/user/INSTALLATION.md`. Feature documents should not duplicate the installation procedure.

## Architecture

- [System overview](architecture/SYSTEM_OVERVIEW.md)
- [Network protocol rules](architecture/NETWORK_PROTOCOL.md)
- [Mod Integration Framework](architecture/MOD_INTEGRATION_FRAMEWORK.md)
- [Creation Kit / STRE bridge](architecture/CK_STRE_BRIDGE.md)
- [Campaign State](architecture/CAMPAIGN_STATE.md)
- [Item Preview Platform](architecture/ITEM_PREVIEW_PLATFORM.md)
- [Observability](architecture/OBSERVABILITY.md)
- [Upstream strategy](architecture/UPSTREAM_STRATEGY.md)
- [Architecture Decision Records](architecture/ADRs/README.md)

## Features

- [World Sync](features/world-sync/README.md)
- [Trading](features/trading/README.md)
- [Item Preview](features/item-preview/README.md)
- [Alternate Start](features/alternate-start/README.md)
- [Downed State](features/downed-state/README.md)

Each feature owns its product/technical/protocol/test details. Global architecture and testing documents must not duplicate those details.

## Testing

- [Test strategy](testing/TEST_STRATEGY.md)
- [Feature acceptance index](testing/ACCEPTANCE_TESTS.md)
- [Multiplayer runbook](testing/MULTIPLAYER_TEST_RUNBOOK.md)
- [Compatibility matrix](testing/COMPATIBILITY_MATRIX.md)

## Production and contribution

- [Work Breakdown Structure](production/WORK_BREAKDOWN_STRUCTURE.md)
- [Dependency map](production/DEPENDENCY_MAP.md)
- [Risk register](production/RISK_REGISTER.md)
- [Decision register](production/DECISION_REGISTER.md)
- [RACI](production/RACI.md)
- [Onboarding paths](production/ONBOARDING_PATHS.md)
- [Contributor role profiles](production/OPEN_ROLES.md)
- [Documentation maintenance](production/DOCUMENTATION_MAINTENANCE.md)
- [GitHub governance](production/GITHUB_GOVERNANCE.md)
- [Versioning and release process](production/RELEASE_PROCESS.md)
- [Development handoff contract](../CONTRIBUTING.md#development-handoff-contract)

## Narrative, art and audio

- [Narrative Bible](narrative/NARRATIVE_BIBLE.md)
- [Valen Character Bible](narrative/VALEN_CHARACTER_BIBLE.md)
- [Dialogue Script](narrative/DIALOGUE_SCRIPT.md)
- [Art Direction](art/ART_DIRECTION.md)
- [Valen Art Brief](art/VALEN_ART_BRIEF.md)
- [Audio Direction](audio/AUDIO_DIRECTION.md)
- [Valen Voice Brief](audio/VALEN_VOICE_BRIEF.md)

## Development and inherited notes

- [Building STRE](development/BUILDING.md)
- [Animation mod compatibility notes](upstream/ANIMATION_MODS.md)

## Canonical-location rule

A feature owns one directory under `docs/features/<feature>/`.

Do not create parallel feature trees under `docs/architecture/`, `docs/testing/` or the repository root. Cross-feature documents may describe contracts and policy, but they must link to feature-specific details rather than copy them.
