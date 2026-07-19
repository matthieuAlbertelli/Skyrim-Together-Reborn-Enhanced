# Changelog

All notable STRE-specific changes are documented here. Upstream Skyrim Together Reborn changes remain available in the upstream history.

## [Unreleased]

### Added

- Project architecture, production, narrative, art, audio and QA documentation.

### Changed

- Consolidated project documentation into canonical feature and architecture folders.
- Replaced placeholder roadmap and one-line build documentation.

### Fixed

- Removed duplicate and obsolete documentation entry points.

## [0.1.0-alpha.1] - 2026-07-19

### Added

- Player-to-player item trading.
- Authoritative trade session protocol and server service.
- Inventory validation, mutation plans and reconciliation flows.
- Native 3D preview for traded items.
- Automatic preview framing and raster-based refinement.
- Angular/CEF trading interface.
- Trading domain and protocol tests.

### Changed

- Refactored item preview responsibilities into dedicated controller, native-session, host-session, bridge, solver and raster-measurement components.
- Decoupled preview hosting from the core trading domain.

### Known limitations

- The trading system remains experimental alpha software.
- Stack splitting and gold exchange are not yet implemented.
- Server-side trade state is not persisted across a server restart.
- Reconnect behavior during an active transaction requires additional integration testing.
- The preview API is currently internal and single-consumer; it is not yet a stable third-party SDK.
