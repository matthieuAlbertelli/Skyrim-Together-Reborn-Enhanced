# Changelog

All notable STRE-specific changes are documented here. Upstream Skyrim Together Reborn changes remain available in the upstream history.

## [Unreleased]

### Added

- Stable in-memory `WorldEntityId` lifecycle for synchronized dropped objects.
- Server-authoritative settlement with local client Havok and bounded final-position reconciliation.
- Snapshot/late-join support for synchronized world items.
- Lazy WorldEntity adoption for existing placed `TESObjectREFR` references using stable server-space reference `GameId`.
- Generic native SKSE plugin reporting and server `ModPolicy:sRequiredNativePlugins`.
- Better Grabbing multiplayer integration without redistributing or linking Better Grabbing source.
- Server manipulation authority states and timeout/disconnect recovery.
- Ownership/stolen provenance through supported inventory and WorldEntity flows.
- Skyrim theft alarm integration when a player grabs an owned reference they are not authorized to manipulate.
- Forced local grab release when the Skyrim Dialogue Menu opens.
- Versioned `STRE_AlternateStart.esp`, Papyrus source and compiled quest script under `GameFiles/Skyrim`.
- Alternate Start character-creation flow in the custom inn with RaceMenu, Angular class/loadout selection, summary and final submission.
- Shared `CharacterBuildCatalog` for logical class/loadout selections.
- Server-authoritative canonical spell grants and normalized spell hashing.
- Starter outfits and weak skill enchantments authored in the Creation Kit.
- Destruction and Alteration starter spells for the Mage class.
- Targeted cooperative buffs for mineral armor, water breathing and carry weight.
- Strict ESP-record and catalog-to-ESP audit scripts.
- Character-build catalog, hash and protocol tests.

### Changed

- Remote held WorldEntities are hidden while another player manipulates them; intermediate grab transforms are not streamed.
- Better Grabbing remains responsible for local input/manipulation while STRE owns network lifecycle and authority.
- Placed-reference release uses STR's existing `MoveTo` primitive instead of custom position/angle engine wrappers.
- Network-triggered world-reference mutations are marshalled through `RunnerService` / the game update path.
- `Inventory::Entry` carries stable ownership metadata and does not merge otherwise-identical entries with different owners.
- Owned container/reference provenance is preserved when available.
- Trading fails closed for ownership-bearing entries until its transfer protocol can preserve instance metadata.
- Character-build protocol advanced to `BuildVersion = 5`.
- Character Creation became a second first-party consumer of the internal 3D preview platform.
- Skyrim UI production build no longer depends on Google Fonts network access.

### Fixed

- Observer crashes when a moved placed reference was released after lazy adoption.
- Stuck Better Grabbing state when guard/arrest dialogue opened while an item was still grabbed.
- Remote/local world-item convergence after drops without continuously fighting Havok.
- Swimming regression introduced by STRE world/health synchronization work.
- Targeted STRE cooperative buff spells affecting remote players.
- Character Build test include-order and dependency issues under MSVC.
- Obsolete/duplicated documentation entry points and stale one-time overlay instructions.

### Validated

- Two-player dropped-object materialization and pickup.
- Local Havok followed by server-authoritative final settlement.
- Two-player grab/release of synchronized dropped WorldEntities.
- Lazy adoption and grab/release of pre-placed movable references.
- Ownership-triggered theft behavior with vanilla guard response.
- Dialogue-triggered forced release after a theft arrest/conversation.
- Swimming behavior after the STRE regression fix.
- Windows xmake build and local deployment.
- Mage character-build smoke flow.
- Cooperative targeted buffs between two PCs.

### Known limitations

- Player-custom item display names (`ExtraTextDisplayData`) are not yet synchronized safely.
- Trading does not yet transfer arbitrary per-instance metadata; unsupported entries are rejected rather than degraded.
- Broader scripted/quest-object World Sync behavior still needs a dedicated validation matrix.
- Durable WorldEntity persistence across server restarts/save branches remains future work.
- Alternate Start does not yet replace the complete vanilla new-game/Helgen flow.
- Character builds are not yet persisted durably across reconnects or server restarts.

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
