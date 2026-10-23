<p align="center">
  <img src="docs/assets/stre-banner-video.gif"
       alt="Skyrim Together Reborn Enhanced"
       width="100%">
</p>

# Skyrim Together Reborn Enhanced

<p align="center">
  <a href="docs/project/STATUS.md"><strong>Current development</strong></a>
</p>

<p align="center">
  <a href="docs/testing/COMPATIBILITY_MATRIX.md"><img src="https://img.shields.io/badge/Skyrim%20SE-1.7.104.0-365F7B?style=flat-square&amp;labelColor=303640" alt="Skyrim SE: 1.7.104.0"></a>
  <a href="docs/project/STATUS.md"><img src="https://img.shields.io/badge/Development-Alpha-8A6826?style=flat-square&amp;labelColor=303640" alt="Development: Alpha"></a>
  <a href="LICENSE"><img src="https://img.shields.io/badge/License-GPLv3-545B66?style=flat-square&amp;labelColor=303640" alt="License: GPLv3"></a>
  <br>
  <a href="docs/project/VISION.md"><img src="https://img.shields.io/badge/Players-1%E2%80%9310%20target-8A6826?style=flat-square&amp;labelColor=303640" alt="Players: 1–10 target"></a>
  <a href="docs/architecture/CAMPAIGN_STATE.md"><img src="https://img.shields.io/badge/Shared%20Campaign-2F6B64?style=flat-square&amp;labelColor=303640" alt="Shared Campaign"></a>
  <a href="docs/project/VISION.md"><img src="https://img.shields.io/badge/Immersion--first-2F6B64?style=flat-square&amp;labelColor=303640" alt="Immersion-first"></a>
  <br>
  <a href="docs/features/alternate-start/README.md"><img src="https://img.shields.io/badge/Alternate%20Start-2F6B64?style=flat-square&amp;labelColor=303640" alt="Alternate Start"></a>
  <a href="docs/features/alternate-start/README.md"><img src="https://img.shields.io/badge/Classes%20%26%20Loadouts-2F6B64?style=flat-square&amp;labelColor=303640" alt="Classes &amp; Loadouts"></a>
  <a href="docs/features/alternate-start/HEADQUARTERS_INN.md"><img src="https://img.shields.io/badge/Shared%20Inn%20Hub-Alpha-8A6826?style=flat-square&amp;labelColor=303640" alt="Shared Inn Hub: Alpha"></a>
  <a href="docs/features/trading/README.md"><img src="https://img.shields.io/badge/Player%20Trading-2F6B64?style=flat-square&amp;labelColor=303640" alt="Player Trading"></a>
  <br>
  <a href="docs/features/world-sync/BETTER_GRABBING_INTEGRATION.md"><img src="https://img.shields.io/badge/Integration-Better%20Grabbing%202.0-2F6B64?style=flat-square&amp;labelColor=303640" alt="Integration: Better Grabbing 2.0"></a>
</p>

> An immersive, systems-driven cooperative fork of Skyrim Together Reborn.

**Skyrim Together Reborn Enhanced (STRE)** extends Skyrim Together Reborn with cooperative mechanics, explicit server authority and systems designed for coherent small-group campaigns without turning Skyrim into an MMO.

> **Skyrim 1.7 port:** STRE has been ported to **Skyrim Special Edition
> 1.7.104.0 (Steam, Windows x64)**. The port is integrated into `main`,
> and current development targets this runtime exclusively. Skyrim 1.6
> is no longer maintained by the current development branch.
>
> This is a development milestone, not a new public release announcement.
> See the [current project status](docs/project/STATUS.md) and
> [compatibility matrix](docs/testing/COMPATIBILITY_MATRIX.md) for the
> validated scope and remaining limitations.

## Install and play

<p>
  <a href="https://github.com/matthieuAlbertelli/Skyrim-Together-Reborn-Enhanced/releases"><img src="https://img.shields.io/badge/Legacy%20release-0.4.0--alpha.1-545B66?style=flat-square&amp;labelColor=303640" alt="Legacy release: 0.4.0-alpha.1"></a>
</p>

Current development targets **Skyrim Special Edition `1.7.104.0` (Steam, Windows x64)**.
The historical public build **STRE `0.4.0-alpha.1`** targets Skyrim `1.6.1170`
and must not be used with `1.7.104.0`.

- [Download STRE releases](https://github.com/matthieuAlbertelli/Skyrim-Together-Reborn-Enhanced/releases)
- [Player installation guide](docs/user/INSTALLATION.md)

For historical public releases, download the `STRE-v<version>-windows-x64.zip` release asset, not GitHub's automatically generated source archives.

See the installation guide and STATUS for the development setup, validated scope and remaining limitations. Address Library, matching SKSE64, Better Grabbing and EEKs Fireplace Resource are external dependencies installed separately. STRE does not redistribute Better Grabbing or EEK assets; Embers HD is not required for the Ilinalta fireplace.

## Current capabilities

STRE currently has four substantial first-party verticals:

- **World Sync** — dropped objects use stable `WorldEntityId` identities, local Havok and server-authoritative settlement; movable placed references are lazily adopted; Better Grabbing integration supports multiplayer object manipulation; ownership/stolen provenance is preserved through the supported world/inventory flow.
- **Player-to-player Trading** — authoritative trade sessions, deterministic mutation plans, idempotent client application, reconciliation and native 3D item preview.
- **Alternate Start / Character Build** — custom inn bootstrap, RaceMenu + Angular character creation, shared class/loadout catalog, canonical inventory/spells and a solo fallback.
- **Campaign Continuity** — durable campaign identity, sealed fixed rosters, coordinated native checkpoints, cold-session Resume, and full-roster collective recovery from the exact last committed checkpoint.

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
