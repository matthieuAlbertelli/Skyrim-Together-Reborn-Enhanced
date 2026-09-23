# Changelog

All notable STRE-specific changes are documented here. Upstream Skyrim Together Reborn changes remain available in the upstream history.

## [Unreleased]

### Added

- Native startup/Main Menu presentation for Skyrim 1.6.1170 / SKSE 2.2.6:
  once-per-process optional trailer, configurable keyboard/controller skip,
  native Scaleform keyboard prompt, intro-only cursor/input suppression and
  bounded vanilla fallback. No 1.7.x support is claimed.
- Distributed silent forward/reverse ping-pong background, independent
  center-left emblem/SKYRIM assets, localized subtitle and feathered backdrop.
  The local intro is excluded while its music permission is pending; packages
  without it open directly on the background and unchanged vanilla menu.
  Contract and asset provenance: [Main Menu](docs/features/main-menu/README.md).
- Policy, decoder and packaging coverage, with Main Menu Video GPL attribution.
  Actual automated and human validation: [STATUS](docs/project/STATUS.md).

## [0.4.0-alpha.1] - 2026-09-19

### Added

- Ilinalta's Vigil headquarters checkpoint: interior shell/circulation, Lake
  Ilinalta exterior and working load doors, tavern music, initial lighting,
  interior navmesh and ten stable starting-seat references.
- Versioned Messire Valen full-body NIF/DDS prototype, with in-game locomotion
  and general-animation smoke evidence; this is not the completed Valen feature.
- Ten explicit existing XMarkerHeading creation positions, selected by the
  lexical rank of sealed durable PlayerIds and paired with Seat01..10.
- Individual post-build seating: each owning PlayerCharacter, after its own
  authoritative Applied, uses MarkerXX -> verified arrival -> marker heading ->
  later engine update -> native SeatXX activation. This local approach remains
  non-MASTER; no all-Applied barrier or remote MoveTo is introduced.
- Shared client/server STR replay rules and bounded consumed-action history for
  animation continuity across a committed native binding replacement.

### Changed

- Official Character Creation keeps intermediate RaceMenu changes local and
  publishes one canonical final appearance after the build is sealed. Observers
  automatically reconstruct the final remote Actor/private base through the
  natural-join materializer, retaining server identity and logical ECS identity.
  No server respawn, reassignment, hot-apply fallback, SwitchRace or Reset3D is
  part of this final flow. Later manual showracemenu changes remain out of scope.
- Creation starts at explicit markers without mandatory seating. Neither creation
  entry nor final rematerialization depends on sitting/standing/furniture state.
- Remote seating uses STR actions and AnimationSystem replay, never native
  Activate(remoteActor). Already-consumed persistent actions are refined before
  pending actions on the new binding; exits invalidate obsolete history.
- The v1 headquarters target is an instanced CK interior; a seamless open-world
  headquarters remains a later enhancement. Physical layout, appearance and
  seating contracts, diagnostics, tests and ADRs now document these boundaries.
- Appearance messages and the versioned Character Build notification require
  matching updated peers. Exact client/server build negotiation remains required;
  no mixed-release compatibility is claimed.

### Fixed

- Replaced the Ilinalta fireplace HDEmbers model with the EEK Vanilla Textured
  variant, removing its direct Embers HD dependency. EEKs Fireplace Resource
  remains separately installed; no EEK or Embers HD assets are redistributed.

- Final rematerialization no longer permanently abandons an otherwise admissible
  actor solely because weapon states 4/5 are transient. A bounded pre-reservation
  wait rechecks all guards and requires an actually safe state before one
  transaction; it does not treat WantToSheathe/Sheathing as safe.
- A chair-enter action consumed on a placeholder before candidate-commit can be
  reconstructed on the final Actor/token using the existing instant counterpart,
  without duplicating pending actions or losing their causal order.
- Local marker approaches resolve the observed native furniture-entry failure;
  logical furniture/camera state alone is no longer reported as visual success.

### Validated

- Maintainer fireplace runtime smoke on 2026-09-19: no missing/purple textures,
  correct carvings/embers/wood, visually unchanged size/origin, unchanged
  collision and unaffected circulation in the development environment.

- Maintainer acceptance in two fresh roster-2 campaigns, both completion orders:
  reciprocal correct final appearances without the placeholder/black Viking,
  individual native local seating and visible remote seating in both directions.
  The A-first trace records weapon 4 -> 0, candidate commit and refined chair
  replay on the new binding; the reverse order is a human attestation.
- Functional checkpoint: TPTests 43,905 assertions / 400 cases; seating 1,529 / 16;
  admission 34,089 / 17; binding replay 340 / 12; 92 structural tests; Windows
  Debug client/launcher/server builds; six CK/ESP audits and diff checks PASS.
  These are recorded checkpoint results, not tests of a future tagged package.
- Headquarters load-door/layout and temporary-NPC navmesh smoke tests, plus
  Valen prototype locomotion/general animations, as recorded in STATUS.

### Known limitations

- Ranks 2..9 have static mapping coverage, not seating runtime acceptance.
  MASTER in-game seating, recovery/reconnect for this specific checkpoint and
  native retirement/lifetime completion are not validated. The producer of
  transient weapon 4/5 remains unknown; persistent unsafe states still time out.
- The full race/sex/state-projection matrix and later manual appearance edits
  are not covered by the reciprocal roster-2 acceptance.
- Valen, Departure, room ownership/housing, all classes/personal quests and the
  complete v1 campaign program remain unfinished. Headquarters decoration,
  rooms, exterior road access and signage remain incomplete.
- Two EEK-path textures (`wrcastlecarvings.dds`, `wrcastlecarvings_n.dds`) retain
  unresolved exact original provenance, accepted by the maintainer for this alpha
  after visual validation. They are not redistributed and are not claimed to be
  vanilla or independently redistributable. Tagged release-mode package checks
  and clean-install smoke, including external texture availability, remain required.
- Unresolved 0.3 alpha limitations remain, including broader recovery UX/live
  matrices, durable Character Build restoration, WorldEntity persistence and
  unsupported campaign downgrade. Back up server state and every player's saves;
  this checkpoint does not establish an old-save upgrade guarantee.

## [0.3.0-alpha.1] - 2026-08-28

### Added

- Durable, server-authoritative campaign identities, character bindings,
  immutable sealed rosters, canonical snapshots, journal/outbox evidence, and
  SQLite persistence with explicit schema migration and fail-closed startup.
- Campaign Create/Join/Resume admission and full-roster
  `WAITING_FOR_ROSTER`/`ACTIVE` behavior without host persistence authority,
  late campaign join, player replacement, or partial-roster progression.
- Coordinated `CampaignCheckpoint` creation: one server-owned Candidate binds
  an immutable canonical revision to one exact local `.ess + .skse` bundle per
  sealed-roster slot and commits only after every required acknowledgement.
- Collective disconnect recovery from the exact `LastCommittedCheckpoint`,
  including native-load and restored-snapshot full-roster barriers, strict
  attempt/checkpoint/revision correlation, idempotent replay, and persistent
  recovery rehydration after restart.
- Cold-session campaign Resume from bounded local binding/marker candidates,
  Manual Load and Main Menu Continue routing for marked campaign saves, and an
  interrupted-campaign surface with Stay and recover or Return to Main Menu.
- Gameplay-facing campaign bootstrap plus the post-Helgen investigation,
  survivor, rubble, bridge, and delayed bandit-occupation projections delivered
  since the previous alpha.

### Changed

- Manual NewSlot and QuickSave/F5 during an active campaign now request the
  shared checkpoint flow instead of creating an uncoordinated local rollback
  point. Auto, unknown, and unproved overwrite provenance remain fail-closed.
- Player-initiated local load/rollback is blocked while admitted to a campaign;
  the exact correlated collective restore remains the only load bypass.
- Returning to Main Menu clears volatile admission and transport state but
  retains the durable campaign binding for a later Continue/Resume.
- Campaign persistence schema v1 migrates transactionally to schema v2 so
  accepted idempotent no-op mutations can be journaled without advancing the
  canonical campaign revision.
- Papyrus packaging now enforces one canonical source/compiled-artifact
  boundary for the STRE Alternate Start plugin.

### Fixed

- Recovery replay no longer creates a second durable restore revision, skips a
  crashed client's required native load, loses already accepted barrier ACKs,
  or leaves completion/gate state stuck across the covered crash windows.
- A disconnect in `WAITING_FOR_ROSTER` no longer creates a recovery attempt;
  disconnects during an existing attempt still retain and replay that attempt.
- Manual Load is consumed before Skyrim enters its fade/loading transition, and
  blocked Journal loads close through the normal UI path instead of leaving a
  non-interactive menu.
- Main Menu departure no longer projects a gameplay recovery gate over the Main
  Menu, and Return to Main Menu no longer leaves a zombie input lock.
- ResumeRequired now closes after authoritative recovery completion instead of
  falling through to the ordinary campaign selector.
- The Linux build again resolves the pinned TiltedCore dependency.

### Validated

- Nominal two-player coordinated checkpoint creation, exact per-player bundle
  hashing, and Candidate -> Committed only after both roster acknowledgements.
- One- and two-player collective restore, successive checkpoint/recovery,
  persisted recovery rehydration, and exact native/snapshot barrier replay.
- Two-player Stay and recover and Return to Main Menu -> Continue/Resume flows.
- Abrupt server termination after a fresh committed checkpoint preserved the
  exact committed identity, revision, snapshot and both slot artifacts across
  restart and readmission.
- Deterministic 2/4/10-slot checkpoint barriers, partial-Candidate preservation,
  duplicate/conflicting ACK behavior, exact no-overwrite replay, and both
  checkpoint/disconnect orderings.
- Alternate Start New Game bootstrap, two-PC Create/Join through Character
  Creation and inn arrival, plus the implemented multiplayer Helgen occupation
  vertical slice.

### Known limitations

- This is a campaign-continuity vertical slice, not v1 feature completion.
  Valen, final Departure, durable Character Build restore, all classes/personal
  quests, headquarters housing, and the complete Alternate Start narrative
  remain unfinished.
- Issue #57 remains open for terminal recovery failure presentation, complete
  keyboard/controller and supported-resolution validation, and the broader
  negative UX/diagnostic matrix.
- Live recovery/checkpoint validation is primarily two-player. Three- and
  four-player live matrices remain incomplete; the narrow mid-ACK disconnect,
  first-ACK packet-loss, and pre-commit force-kill races are covered by
  deterministic ordering/transaction evidence but were not manually reproduced.
- Manual ExistingSlot save overwrite and AutoSave provenance remain unproved
  and fail-closed during campaigns. Skyrim's native Gameplay settings rows are
  not visually disabled; the STR Settings projection is informational.
- Mixed release builds are rejected by exact client/server build negotiation.
  Campaign database downgrade from schema v2 is unsupported.
- Older SkyUI Journal movies are not in the supported campaign-load matrix and
  can be incompatible with Skyrim AE `1.6.1170` callbacks.
- Durable WorldEntity persistence, arbitrary Trading instance metadata, and the
  broader existing-system stabilization matrix remain future work.

## [0.2.0-alpha.1] - 2026-08-10

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
- CI-built Windows playable release artifact.

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
- GitHub Actions Windows release build.
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
