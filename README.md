<p align="center">
  <img src="docs/banner.png" alt="Skyrim Together Reborn Enhanced">
</p>

# Skyrim Together Reborn Enhanced

<p align="center">

![Version](https://img.shields.io/badge/version-0.2.0--alpha.1-orange)
![Status](https://img.shields.io/badge/status-Alpha-red)
![License](https://img.shields.io/badge/license-GPLv3-blue)

</p>

> An immersive, systems-driven cooperative fork of Skyrim Together Reborn.

**Skyrim Together Reborn Enhanced (STRE)** extends Skyrim Together Reborn with cooperative mechanics, explicit server authority and systems designed for coherent small-group campaigns without turning Skyrim into an MMO.

## Install and play

The current public build is **STRE `v0.2.0-alpha.1`**.

- [Download STRE releases](https://github.com/matthieuAlbertelli/Skyrim-Together-Reborn-Enhanced/releases)
- [Player installation guide](docs/user/INSTALLATION.md)

Download the `STRE-v<version>-windows-x64.zip` release asset, not GitHub's automatically generated source archives.

The current alpha installation target is Steam Skyrim Special Edition runtime `1.6.1170` on Windows x64. Address Library, matching SKSE64 and Better Grabbing are external dependencies; Better Grabbing is not redistributed by STRE.

## Current capabilities

STRE currently has three substantial first-party verticals:

- **World Sync** — dropped objects use stable `WorldEntityId` identities, local Havok and server-authoritative settlement; movable placed references are lazily adopted; Better Grabbing integration supports multiplayer object manipulation; ownership/stolen provenance is preserved through the supported world/inventory flow.
- **Player-to-player Trading** — authoritative trade sessions, deterministic mutation plans, idempotent client application, reconciliation and native 3D item preview.
- **Alternate Start / Character Build** — custom inn bootstrap, RaceMenu + Angular character creation, shared class/loadout catalog, canonical inventory/spells and a solo fallback.

See [Current project status](docs/project/STATUS.md) for the maintained implementation/validation snapshot.

<p align="center">
  <img src="docs/features/trading/assets/trade-demo-ui.jpg" alt="STRE trading interface" width="900">
</p>

<p align="center">
  <img src="docs/features/trading/assets/trade-demo.gif" alt="STRE trading demonstration" width="900">
</p>

## Architecture principles

STRE follows a few explicit rules:

- shared mutable state has a declared authority;
- network messages express intents and canonical outcomes rather than trusting local presentation state;
- Skyrim/CK/UI layers project state but do not silently become its source of truth;
- network-triggered Skyrim mutations run on the game update path;
- local engine simulation is reused when it is already the correct abstraction;
- third-party integration contracts are generalized only after first-party use validates them;
- features fail closed when metadata cannot be preserved safely.

Read [System overview](docs/architecture/SYSTEM_OVERVIEW.md), [Network protocol rules](docs/architecture/NETWORK_PROTOCOL.md) and the [ADR index](docs/architecture/ADRs/).

## Documentation

- [Player installation](docs/user/INSTALLATION.md)
- [Documentation portal](docs/README.md)
- [Current project status](docs/project/STATUS.md)
- [Vision](docs/project/VISION.md)
- [Roadmap](ROADMAP.md)
- [World Sync](docs/features/world-sync/README.md)
- [Trading](docs/features/trading/README.md)
- [Alternate Start](docs/features/alternate-start/README.md)
- [Compatibility matrix](docs/testing/COMPATIBILITY_MATRIX.md)
- [Changelog](CHANGELOG.md)
- [Contributing](CONTRIBUTING.md)
- [Support](SUPPORT.md)
- [Security policy](SECURITY.md)

English is the canonical language for STRE engineering and project documentation. Localized player-facing content may use its target language.

## Contributing

New to STRE?

1. Read [Contributing](CONTRIBUTING.md).
2. Review the [current project status](docs/project/STATUS.md).
3. Choose your [onboarding path](docs/production/ONBOARDING_PATHS.md).
4. Pick an actionable issue from the GitHub Project.
## Build and development

See [Building STRE](docs/development/BUILDING.md), [Contributing](CONTRIBUTING.md) and [Code guidelines](CODE_GUIDELINES.md).

## Upstream relationship

STRE is maintained as an independent community fork of `tiltedphoques/TiltedEvolution`.

- [Current upstream baseline](UPSTREAM.md)
- [Upstream integration strategy](docs/architecture/UPSTREAM_STRATEGY.md)

## Credits and license

Skyrim Together Reborn Enhanced builds on the work of the Tilted Phoques team and all Skyrim Together Reborn contributors. It is not affiliated with or endorsed by the original team, Bethesda Game Studios or ZeniMax Media.

The project is distributed under the GNU General Public License v3.0. See the repository license file and [NOTICE.md](NOTICE.md).
