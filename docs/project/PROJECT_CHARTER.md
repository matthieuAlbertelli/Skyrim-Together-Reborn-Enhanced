# Project Charter

> **Status:** active charter.

## Mission

Develop and maintain an open-source fork of Skyrim Together Reborn that
strengthens immersive cooperation, provides a shared campaign foundation, and
builds maintainable adaptation contracts for originally single-player Skyrim
content.

## Scope

### Included

- STRE client and server;
- network protocol and authoritative state;
- cooperative UI;
- synchronization of STRE world entities;
- Alternate Start plugin;
- CK/Papyrus ↔ STRE bridge;
- mod-integration contracts;
- project-owned narrative and art content;
- test tools, logs, documentation, and packaging.

### Initially out of scope

- an MMO-style persistent world;
- universal compatibility without mod-author involvement;
- a complete rewrite of every vanilla quest;
- guaranteed support for every Skyrim version and load order;
- redistribution of proprietary assets without permission.

## Governance

### Product decisions

Product direction decides vision, scope, and priority after consulting the
responsible contributors.

### Architecture decisions

Every durable structural decision is recorded in an ADR. Feature details remain
in the feature's canonical directory.

### Protocol changes

Any significant change to an opcode, serialized schema, or persistent state
requires:

- explicit bounds;
- round-trip tests;
- a compatibility strategy;
- client/server review;
- an update to the feature's protocol reference.

## Operating rules

- An issue describes an observable outcome.
- A feature is not complete without tests, useful logs, and documentation.
- A branch avoids mixing a large refactor with a functional change without
  justification.
- Every critical datum has a source of truth and a recovery strategy.
- Every mutable documentation fact has one canonical source.
- Historical documents are archived as such and do not announce current state.

## Engineering principles

- KISS: reuse reliable engine and STR mechanisms before inventing a layer.
- DRY: do not maintain a business rule, protocol contract, or mutable status in
  parallel in several places.
- API-friendly: depend on observable, stable contracts instead of third-party
  internals where possible.
- Fail closed: reject an operation when required metadata cannot be preserved
  correctly.
- Explicit authority: every mutated shared state declares its authority.
- Engine-safe: network-triggered Skyrim mutations run in an appropriate engine
  context.

## Health criteria

- reproducible builds;
- relevant automated tests;
- identifiable upstream base;
- recorded architecture decisions;
- a roadmap tied to demonstrable outcomes;
- documentation without competing sources of truth;
- playable demonstrations at every significant milestone.

For implemented and validated state, see [`docs/project/STATUS.md`](STATUS.md).
Product direction and release gates belong in [`ROADMAP.md`](../../ROADMAP.md);
operational progress belongs in the GitHub Project governed by
[`docs/production/GITHUB_GOVERNANCE.md`](../production/GITHUB_GOVERNANCE.md).
