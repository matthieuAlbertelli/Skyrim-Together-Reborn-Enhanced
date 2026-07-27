# Changelog

All notable STRE-specific changes are documented here. Upstream Skyrim Together Reborn changes remain available in the upstream history.

## [Unreleased]

### Added

- Versioned `STRE_AlternateStart.esp`, Papyrus source and compiled quest script under `GameFiles/Skyrim`.
- Alternate Start character-creation flow in the custom inn with RaceMenu, Angular class/loadout selection, summary and final submission.
- Shared `CharacterBuildCatalog` for logical class/loadout selections.
- Server-authoritative canonical spell grants and normalized spell hashing.
- Starter outfits and weak skill enchantments authored in the Creation Kit.
- Destruction and Alteration starter spells for the Mage class.
- Targeted cooperative buffs for mineral armor, water breathing and carry weight.
- Strict ESP-record and catalog-to-ESP audit scripts.
- Character-build catalog, hash and protocol tests.
- Project architecture, production, narrative, art, audio and QA documentation.

### Changed

- Character-build protocol advanced to `BuildVersion = 5`.
- `CharacterBuildSnapshotData` now carries `CanonicalSpells` and `SpellHash`.
- `CharacterBuildAppliedRequest` now acknowledges inventory and spell hashes.
- Warrior, Mage and Thief starter equipment now uses canonical CK/vanilla records.
- Lockpicking starter reward simplified to ten vanilla lockpicks.
- Character Creation became a second first-party consumer of the internal 3D preview platform.
- Consolidated project documentation into canonical feature and architecture folders.
- Updated repository documentation to reflect the implemented Alternate Start vertical slice.

### Fixed

- Allowed STRE targeted buff spells to affect and synchronize on remote players.
- Resolved Character Build test include-order and dependency issues under MSVC.
- Removed obsolete CK batch documentation and generated audit reports from version control.
- Removed duplicate and obsolete documentation entry points.

### Validated

- Strict CK manifest audit: 47 expected records conform.
- Catalog-to-ESP audit: 41 code references conform.
- Windows xmake build and local deployment completed successfully.
- Mage character-build flow smoke-tested in Skyrim.
- Cooperative targeted buffs smoke-tested between two PCs.

### Known limitations

- Alternate Start does not yet automatically replace the complete vanilla new-game/Helgen flow.
- Valen, the shared introduction, roster, ready check and departure flow remain pending.
- Character builds are authoritative during the active server session but are not yet persisted durably across reconnects or server restarts.
- Invocation, Illusion and Restoration rewards are still design/UI placeholders.
- Enchanting starter kits and the remaining skill packages still need catalog materialization and balancing.

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
- The preview API is currently internal and single-consumer at the bridge boundary; it is not yet a stable third-party SDK.
