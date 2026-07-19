# STRE Roadmap

> **Status:** proposed sequencing based on the source audit of 19 July 2026.

The roadmap is organized around demonstrable outcomes. A milestone is complete only when its user flow, failure cases and recovery behavior can be shown in game.

## R0 — Repository ready

**Outcome:** a new contributor can understand, build and test STRE from a clean checkout.

- [ ] verify and document the supported toolchain;
- [ ] keep the exact upstream base recorded for every release;
- [ ] add CI coverage for trading tests and the Angular UI;
- [ ] correct inherited package/repository metadata;
- [ ] publish the supported Skyrim/STRE version matrix;
- [ ] separate source, distribution and local deployment outputs.

## R1 — Trading 0.2

**Outcome:** trading is reliable enough for real campaign play.

- [ ] client/server integration tests;
- [ ] telemetry for application and reconciliation failures;
- [ ] explicit disconnect/reconnect policy during a trade;
- [ ] stack splitting;
- [ ] gold exchange;
- [ ] player-facing error and recovery UX;
- [ ] documented transferability rules and known limitations.

## R2 — Item Preview Platform

**Outcome:** multiple STRE features can use the same internal preview runtime safely.

- [ ] replace the single-client bridge with leases and owner tokens;
- [ ] formalize request, priority and lifecycle contracts;
- [ ] isolate the CEF/native host bridge;
- [ ] add solver and lifecycle tests;
- [ ] define arbitration with native Skyrim menus;
- [ ] demonstrate a second consumer outside trading.

## R3 — Alternate Start Solo

**Outcome:** a new game can skip Helgen and start in the inn without STRE running.

- [ ] controlled new-game flow;
- [ ] character creation at the campaign table;
- [ ] Valen and a minimal introduction quest;
- [ ] class selection and starter loadouts;
- [ ] coherent departure into Skyrim;
- [ ] verified vanilla main-quest continuation.

## R4 — Mod Integration Framework MVP

**Outcome:** Alternate Start works in a group through a first-party adapter.

- [ ] `Capability`, `Intent`, `CanonicalState`, `Event` and `Snapshot` contracts;
- [ ] compiled adapter registry;
- [ ] Creation Kit/Papyrus bridge;
- [ ] versioned Campaign State;
- [ ] synchronized ready check;
- [ ] reconnect snapshot and idempotent replay;
- [ ] typed errors and structured logs.

## R5 — Playable cooperative campaign start

**Outcome:** 2–10 players create campaign characters and leave the inn together.

- [ ] campaign roster and character binding;
- [ ] secret Dragonborn assignment;
- [ ] synchronized introduction;
- [ ] late-join policy;
- [ ] persistence between sessions;
- [ ] functional 2-, 4- and 10-player validation.

## R6 — Experimental third-party SDK

**Outcome:** an external mod author can implement a documented adapter.

- [ ] versioned generic envelopes and schemas;
- [ ] adapter manifest and validation tooling;
- [ ] permissions/sandbox model;
- [ ] Papyrus and C++ examples;
- [ ] compatibility and deprecation policy.

## R7 — Additional cooperative systems

- downed, out-of-combat and post-combat recovery states;
- cooperative classes and proximity-based talents;
- persistent consequences;
- group votes and shared decisions;
- additional pilot mod integrations.

See [production milestones](docs/production/MILESTONES.md) and [work breakdown](docs/production/WORK_BREAKDOWN_STRUCTURE.md).
