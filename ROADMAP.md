# STRE Roadmap

> **Status:** updated on 27 July 2026 from the repository state and in-game M7 validation.

The roadmap is organized around demonstrable outcomes. A milestone is complete only when its user flow, failure cases and recovery behavior can be shown in game.

## R0 — Repository ready

**Outcome:** a new contributor can understand, build and test STRE from a clean checkout.

- [x] record the audited upstream baseline;
- [x] document current build entry points;
- [x] version CK-authored Alternate Start files;
- [ ] validate a clean-machine prerequisite matrix;
- [ ] add CI coverage for native tests and the Angular UI;
- [ ] publish the supported Skyrim/STRE version matrix;
- [ ] establish one canonical automated test command.

## R1 — Trading 0.2

**Outcome:** trading is reliable enough for real campaign play.

- [x] authoritative trade protocol and reconciliation model;
- [x] dedicated domain/protocol tests;
- [ ] client/server integration tests;
- [ ] telemetry for application and reconciliation failures;
- [ ] explicit disconnect/reconnect policy during a trade;
- [ ] stack splitting;
- [ ] gold exchange;
- [ ] player-facing error and recovery UX.

## R2 — Item Preview Platform

**Outcome:** multiple STRE features can use the same internal preview runtime safely.

- [x] modular native session, controller, bridge, solver and raster measurement;
- [x] automatic fitting of real Skyrim objects;
- [x] demonstrate a second first-party consumer in Character Creation;
- [ ] replace the single-client bridge with leases and owner tokens;
- [ ] formalize request, priority and lifecycle contracts;
- [ ] add solver and lifecycle tests;
- [ ] define arbitration with native Skyrim menus.

## R3 — Alternate Start character bootstrap

**Outcome:** a player can create a clean campaign character in the custom inn, in solo or through an authoritative server session.

- [x] version the ESP, PSC and PEX files;
- [x] custom inn cell, quest aliases, seats and stages `0/10/20`;
- [x] RaceMenu and Angular character-creation flow;
- [x] Warrior, Mage and Thief class/loadout selection;
- [x] anti-import cleanup, level reset and canonical equipment application;
- [x] offline/local fallback without a connected server;
- [x] Destruction and Alteration starter spells;
- [x] targeted cooperative buffs validated between two PCs;
- [ ] automatic new-game interception and complete Helgen bypass;
- [ ] Valen and minimal introduction quest;
- [ ] coherent departure into Skyrim and verified vanilla quest continuation.

## R4 — Authoritative character builds and first-party integration

**Outcome:** client choices become a validated canonical character build without trusting arbitrary FormIDs.

- [x] logical class/loadout selections;
- [x] shared catalog at `BuildVersion = 5`;
- [x] canonical inventory and inventory hash;
- [x] canonical spells and spell hash;
- [x] applied acknowledgement and server state broadcast;
- [x] strict ESP/catalog audits;
- [ ] durable build persistence;
- [ ] restoration after reconnect/server restart;
- [ ] remaining skill kits and magic schools;
- [ ] generic adapter registry and version negotiation.

## R5 — Playable cooperative campaign start

**Outcome:** 2–10 players create campaign characters and leave the inn together.

- [ ] campaign roster and character binding;
- [ ] shared phase and ready check;
- [ ] synchronized Valen introduction;
- [ ] secret Dragonborn assignment;
- [ ] late-join policy;
- [ ] persistence between sessions;
- [ ] functional 2-, 4- and 10-player validation.

## R6 — Experimental third-party SDK

**Outcome:** an external mod author can implement a documented adapter.

- [ ] versioned generic envelopes and schemas;
- [ ] adapter manifest and validation tooling;
- [ ] permissions/sandbox model;
- [ ] Papyrus and C++ examples;
- [ ] compatibility and deprecation policy;
- [ ] at least one additional first-party integration before freezing contracts.

## R7 — Additional cooperative systems

- downed, out-of-combat and post-combat recovery states;
- cooperative classes and proximity-based talents;
- persistent consequences;
- group votes and shared decisions;
- additional pilot mod integrations.

See [production milestones](docs/production/MILESTONES.md) and [work breakdown](docs/production/WORK_BREAKDOWN_STRUCTURE.md).
