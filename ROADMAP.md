# STRE Roadmap

> **Status:** canonical progress roadmap.
> **Last updated:** 10 August 2026.

This file is the single source of truth for **priority and progress**. Feature documents describe behavior and design; the WBS describes work decomposition without duplicating progress state.

A milestone is complete only when its user-visible outcome, failure cases and recovery behavior are demonstrable.

## R0 — Repository ready

**Outcome:** a contributor can understand, build and test STRE from a documented checkout.

- [x] audited upstream baseline recorded;
- [x] build entry points documented;
- [x] CK-authored Alternate Start files versioned;
- [x] canonical documentation structure established;
- [ ] clean-machine prerequisite matrix;
- [ ] broader CI coverage for native tests and Angular UI;
- [ ] one canonical automated test command;
- [ ] release-grade supported-version matrix.

## R1 — Trading stabilization

**Outcome:** trading is reliable enough for normal campaign use.

- [x] authoritative trade protocol;
- [x] deterministic mutation plans;
- [x] idempotent client application;
- [x] absolute reconciliation path;
- [x] Angular/CEF UI and native item preview;
- [ ] full client/server integration harness;
- [ ] reconnect policy during active trades;
- [ ] stack splitting and gold;
- [ ] instance-metadata-aware transfer beyond the current MVP;
- [ ] player-facing recovery UX.

## R2 — World Sync foundation

**Outcome:** physical world items can be shared without continuous remote physics streaming.

- [x] stable in-memory `WorldEntityId` for dropped objects;
- [x] local Havok with server-authoritative settlement;
- [x] bounded final-position reconciliation;
- [x] snapshot/late-join path for world entities;
- [x] lazy adoption of movable placed references;
- [x] Better Grabbing native-plugin policy and multiplayer bridge;
- [x] hidden remote representation while another player grabs an object;
- [x] placed-reference release via STR `MoveTo` on the game update path;
- [x] ownership/stolen provenance in supported inventory/world flows;
- [x] grab of unauthorized owned references treated as Skyrim theft;
- [x] forced release when dialogue opens to avoid stuck grab state;
- [ ] broader validation of scripted/quest-owned movable references;
- [ ] custom player-renamed item metadata;
- [ ] durable world persistence across server restarts/save branches;
- [ ] extend the same entity model to additional world object classes.

## R3 — Item Preview Platform

**Outcome:** multiple STRE features safely reuse one native preview runtime.

- [x] modular native session, controller, host bridge, solver and raster measurement;
- [x] Trading consumer;
- [x] Character Creation consumer;
- [ ] lease/owner arbitration;
- [ ] lifecycle/concurrency test coverage;
- [ ] stable internal request contract;
- [ ] third-party API only after first-party stabilization.

## R4 — Alternate Start character bootstrap

**Outcome:** a player creates a clean campaign character in the custom inn, in solo or through an authoritative STRE session.

- [x] versioned ESP/PSC/PEX;
- [x] custom inn, quest aliases and seat flow;
- [x] RaceMenu + Angular character creation;
- [x] Warrior, Mage and Thief catalog path;
- [x] canonical inventory and spell application;
- [x] offline/local fallback;
- [x] Destruction and Alteration starter spell vertical slice;
- [x] targeted cooperative buffs smoke-tested between two PCs;
- [ ] automatic new-game interception and exhaustive Helgen bypass;
- [ ] remaining character kits and schools;
- [ ] full reset policy for skills/perks/attribute history;
- [ ] Valen introduction and coherent vanilla departure.

## R5 — Persistent cooperative campaign

**Outcome:** campaign state and character binding survive reconnects and server restarts.

- [ ] durable character binding;
- [ ] versioned persistence;
- [ ] canonical campaign snapshot;
- [ ] reconnect restoration;
- [ ] roster and ready state;
- [ ] late-join policy;
- [ ] shared introduction/departure phases;
- [ ] 2- and 4-player validation before broader scale targets.

## R6 — Experimental mod-integration SDK

**Outcome:** an external mod author can implement a documented STRE adapter.

- [ ] generic versioned adapter envelopes;
- [ ] adapter manifest and negotiation;
- [ ] permissions/sandbox policy;
- [ ] Papyrus and C++ examples;
- [ ] compatibility/deprecation policy;
- [ ] at least one additional first-party integration before freezing contracts.

## R7 — Additional cooperative systems

Potential later systems include:

- downed/out-of-combat/recovery states;
- cooperative classes and proximity talents;
- persistent consequences;
- group votes and shared decisions;
- additional pilot mod integrations.

See [Work Breakdown Structure](docs/production/WORK_BREAKDOWN_STRUCTURE.md) for decomposition, not progress tracking.
