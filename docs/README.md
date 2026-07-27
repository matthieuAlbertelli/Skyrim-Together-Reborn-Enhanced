# STRE Documentation

This directory is the canonical documentation portal for Skyrim Together Reborn Enhanced.

## Status vocabulary

- **Implemented / Implémenté** — confirmed in the current source.
- **Validated in game / Validé en jeu** — exercised in Skyrim; the documented scope and test date must be stated.
- **Accepted / Décidé** — product or architecture direction selected for STRE.
- **Proposed / Proposé** — recommended design awaiting implementation or formal ratification.
- **Open / À valider** — unresolved decision or prototype requirement.
- **Historical / Historique** — a dated audit or decision preserved for traceability, not a statement of current implementation.

## Current delivery snapshot

As of 27 July 2026:

- Trading is implemented as an experimental authoritative alpha.
- The internal item-preview platform is used by Trading and Character Creation.
- Alternate Start contains an implemented character-bootstrap vertical slice with a versioned CK plugin, RaceMenu/Angular flow, canonical inventory and spell application, and a solo fallback.
- `BuildVersion = 5` carries `CanonicalSpells` and `SpellHash`.
- Destruction and Alteration starter spells are integrated; three targeted cooperative buffs have been smoke-tested between two PCs.
- Full Helgen replacement, Valen, Campaign State, ready checks, durable persistence and reconnection restoration remain future work.

## Start here

- [Main README](../README.md)
- [Executive summary](project/EXECUTIVE_SUMMARY.md)
- [Current-state audit](audit/CURRENT_STATE_AUDIT.md)
- [Documentation audit — 27 July 2026](audit/DOCUMENTATION_AUDIT_20260727.md)
- [Vision](project/VISION.md)
- [Project charter](project/PROJECT_CHARTER.md)
- [System overview](architecture/SYSTEM_OVERVIEW.md)
- [Roadmap](../ROADMAP.md)
- [Changelog](../CHANGELOG.md)
- [Contributing](../CONTRIBUTING.md)

## Architecture

- [Mod Integration Framework](architecture/MOD_INTEGRATION_FRAMEWORK.md)
- [Creation Kit / STRE bridge](architecture/CK_STRE_BRIDGE.md)
- [Campaign State](architecture/CAMPAIGN_STATE.md)
- [Network protocol](architecture/NETWORK_PROTOCOL.md)
- [Item Preview Platform](architecture/ITEM_PREVIEW_PLATFORM.md)
- [Observability](architecture/OBSERVABILITY.md)
- [Upstream strategy](architecture/UPSTREAM_STRATEGY.md)
- [Architecture Decision Records](architecture/ADRs/)

## Features

- [Trading](features/trading/README.md)
- [Item Preview](features/item-preview/README.md)
- [Alternate Start](features/alternate-start/README.md)
- [Downed State](features/downed-state/README.md)

## Narrative, art and audio

- [Narrative Bible](narrative/NARRATIVE_BIBLE.md)
- [Valen Character Bible](narrative/VALEN_CHARACTER_BIBLE.md)
- [Dialogue Script](narrative/DIALOGUE_SCRIPT.md)
- [Art Direction](art/ART_DIRECTION.md)
- [Valen Art Brief](art/VALEN_ART_BRIEF.md)
- [Audio Direction](audio/AUDIO_DIRECTION.md)
- [Valen Voice Brief](audio/VALEN_VOICE_BRIEF.md)

## Production and contribution

- [Open roles](production/OPEN_ROLES.md)
- [Milestones](production/MILESTONES.md)
- [Onboarding paths](production/ONBOARDING_PATHS.md)
- [Decision register](production/DECISION_REGISTER.md)
- [RACI](production/RACI.md)
- [Dependency map](production/DEPENDENCY_MAP.md)
- [Work Breakdown Structure](production/WORK_BREAKDOWN_STRUCTURE.md)
- [Documentation maintenance](production/DOCUMENTATION_MAINTENANCE.md)

## Testing

- [Test strategy](testing/TEST_STRATEGY.md)
- [Acceptance tests](testing/ACCEPTANCE_TESTS.md)
- [Multiplayer runbook](testing/MULTIPLAYER_TEST_RUNBOOK.md)
- [Compatibility matrix](testing/COMPATIBILITY_MATRIX.md)

## Development and inherited notes

- [Building STRE](development/BUILDING.md)
- [Animation mod compatibility notes](upstream/ANIMATION_MODS.md)

## Canonical-location rule

Each feature owns one directory under `docs/features/<feature>/`. Do not recreate single-file feature summaries such as `docs/features/trading.md`; add or update the feature directory instead.
