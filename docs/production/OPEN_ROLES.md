# Contributor Role Profiles

> **Status:** stable capability reference; not a live work or assignment tracker.

This page describes recurring contribution profiles for STRE. It does not claim that a particular task is open, assigned or scheduled.

- Product direction and release gates live in [`ROADMAP.md`](../../ROADMAP.md).
- Implemented/validated state lives in [`docs/project/STATUS.md`](../project/STATUS.md).
- Live opportunities belong to the GitHub Project and actionable issues labelled `help wanted` or `good first issue`, following [`GITHUB_GOVERNANCE.md`](GITHUB_GOVERNANCE.md).

Every actionable contribution should have an owner/reviewer, target Milestone when applicable, acceptance evidence and clear asset/licensing requirements.

## C++ network and persistence engineering

Typical scope includes versioned storage, character/campaign binding, canonical snapshots, reconnect/idempotency, migration, bounded serialization, actor/world lifecycle, shared-state authority and recovery tests.

Useful experience: C++20, EnTT, distributed systems, protocol design, persistence and multiplayer diagnostics.

## C++ gameplay systems and native integration

Typical scope includes Skyrim-facing adapters, cooperative capability classification, target validation, stacking/friendly-fire rules, game-thread marshalling, native-plugin integration, item preview lifecycle and focused native tests.

Useful experience: reverse engineering, SKSE/CommonLibSSE-style APIs, engine-safe mutation, C++ testing and observability.

## Creation Kit and Papyrus development

Typical scope includes Alternate Start, quests/aliases/scenes, dialogue, inn and room content, navmesh, canonical records, solo fallback, Papyrus diagnostics and reproducible PSC-to-PEX delivery.

Useful experience: Skyrim Creation Kit, Papyrus, quest state, FormID-safe configuration and multiplayer-aware content authoring.

## Gameplay and class design

Typical scope includes class identity, starting loadouts, cooperative perks/abilities, personal quests, balance, combination testing and alignment between the logical catalog, CK records, UI and authoritative application.

Useful experience: Skyrim gameplay systems, economy/balance, cooperative encounter design and structured acceptance criteria.

## Narrative, voice and character art

Typical scope includes Messire Valen, the headquarters inn, class stories, dialogue, performance, character/environment art and coherent integration with groups of different sizes.

All contributions require explicit provenance, a compatible licence and review against the project narrative/art/audio direction.

## UI/UX design and implementation

Typical scope includes Trading, Character Creation, lobby/roster, class/room flows, recovery messaging, controller/keyboard navigation, accessibility and 16:9/21:9 validation across Angular, CEF and native bridges.

Useful experience: Angular, typed event contracts, interaction design and in-game UI diagnostics.

## Multiplayer QA

Typical scope includes reproducible solo and multi-PC scenarios, host/client/observer comparison, clean and modded environment matrices, sealed-roster rejection, disconnect/collective-restore and WorldEntity late-materialization coverage, logs, saves and regression evidence.

Useful experience: disciplined reproduction, network reasoning, Skyrim mod profiles and concise issue reporting.

## Build, release and documentation

Typical scope includes clean-machine prerequisites, CI, packaging, compatibility evidence, installation guidance, link/schema validation and keeping each mutable fact in its canonical document.

Useful experience: GitHub Actions, xmake, Windows/Linux build environments, technical writing and release verification.
