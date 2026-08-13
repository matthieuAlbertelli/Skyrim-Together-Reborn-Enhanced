# Current STRE Status

> **Status:** source of truth for implemented and validated state.
> **Last updated:** August 13, 2026.

This document describes **the repository's actual current state**. Product
direction and release gates belong in [`ROADMAP.md`](../../ROADMAP.md),
operational progress belongs in the GitHub Project governed by
[`docs/production/GITHUB_GOVERNANCE.md`](../production/GITHUB_GOVERNANCE.md),
and technical detail belongs in each feature's documentation.

## World Sync

### Implemented and validated in game

- dropped objects receive a stable network identity, `WorldEntityId`;
- each client retains its local Havok simulation;
- the player initiating an action retains settlement authority until the final
  transform;
- remote clients are corrected only when divergence is significant;
- snapshots support late materialization and binding of WorldEntities;
- movable references already present in the world are lazily adopted through
  their `PlacedReferenceId`;
- a placed reference is bound to the existing local Skyrim reference and is
  never duplicated;
- Better Grabbing is required by default in multiplayer through the generic
  native SKSE plugin policy;
- during a remote grab, observers hide the object instead of continuously
  streaming it;
- on release, placed references use STR's existing `MoveTo` path on the game
  thread through `RunnerService`;
- ownership and provenance are carried through supported paths;
- grabbing an owned object without being its owner triggers Skyrim's vanilla
  theft system;
- opening the `Dialogue Menu` cleanly ends a grab to avoid blocking guard or
  arrest dialogue.

### Known limitations

- custom names based on `ExtraTextDisplayData` are not synchronized yet;
- scripted references and quest objects still require a dedicated validation
  campaign;
- durable world persistence across server restart or save branches is not yet
  implemented;
- the WorldEntity model is not yet generalized to every type of world reference.

See [`docs/features/world-sync/`](../features/world-sync/).

## Trading

### Implemented

- dedicated session domain;
- authoritative server protocol;
- revisioned offers;
- deterministic mutation plans;
- idempotent client application;
- reconciliation to absolute quantities;
- Angular/CEF UI;
- native 3D preview.

### Limitations

- divisible stacks and gold are not supported yet;
- reconnecting an active trade needs further hardening;
- the MVP protocol does not carry all instance metadata;
- objects with ownership that cannot be represented are rejected instead of
  being transferred with data loss.

See [`docs/features/trading/`](../features/trading/).

## Item Preview

### Implemented

- native session;
- controller;
- host bridge and session;
- framing solver;
- raster measurement;
- Trading and Character Creation consumers.

### Structural limitation

The bridge still supports only one active consumer. An explicit lease and
ownership system remains necessary before declaring a stable third-party API.

See [`docs/features/item-preview/`](../features/item-preview/).

## Alternate Start / Character Build

### Implemented and smoke-tested

- versioned `STRE_AlternateStart.esp` with PSC/PEX files;
- inn, quest, aliases, and seats;
- RaceMenu and Angular Character Creation;
- shared Warrior/Mage/Thief catalog;
- canonical inventory and spells;
- hashes and application acknowledgement;
- local fallback without a server;
- Mage Destruction and Alteration;
- targeted cooperative buffs tested between two PCs.

The current catalog uses `BuildVersion = 5`.

### Limitations

- complete new-game interception and Helgen bypass remain unfinished;
- Valen and the narrative departure are not finalized;
- the live Character Build service is not yet bound to durable campaign identity
  or reconnect restoration;
- several schools and kits remain to be materialized;
- skill, perk, and attribute-history reset remains incomplete.

### Durable campaign persistence foundation

- a dedicated campaign persistence port and SQLite adapter are implemented;
- the locked server setting defaults to `state/stre-server.sqlite3`;
- the server opens, migrates, and integrity-checks the store before constructing
  its `World`, and persistence startup failure fails closed;
- schema version 1 stores multiple campaign identities, roster slots,
  `PlayerId`/`CharacterBinding` records, versioned Character Build state,
  audience-tagged adapter state, immutable snapshots, Candidate/Committed
  checkpoint metadata, per-slot native-save metadata, an append-only journal,
  and a transactional outbox;
- optimistic revisions and `MutationId` idempotency protect atomic current-state
  + journal + outbox mutations;
- checkpoint restore materializes the exact immutable snapshot at a new
  monotonic revision and supersedes obsolete pending outbox work;
- file-backed automated tests cover restart, migration, partial-write rollback,
  multiple campaigns, identity mismatch, checkpoint lifecycle, exact restore,
  malformed persisted data, audience filtering, and prepared data statements.

The durable server campaign/checkpoint persistence substrate is implemented and
automated-tested; the live fixed-roster campaign flow, coordinated native saves,
and collective reconnect recovery remain unimplemented. `CharacterBuildService`
continues to use session state until #28 supplies campaign identity/binding
callers. Coordinated native-save/checkpoint work remains #55, and disconnect
recovery lock plus collective restore/reload remains #56. No native `.ess`
payload, save/load engine call, recovery UI, or WorldEntity persistence is part
of this foundation; durable WorldEntity persistence remains separate future
work rather than part of #55.

See [`docs/features/alternate-start/`](../features/alternate-start/).

## Important fixed regressions

- a swimming regression introduced during STRE work;
- an observer crash while repositioning a placed reference;
- a stuck grab state during guard or arrest dialogue;
- a Google Fonts dependency that blocked offline Angular builds.

## Communication rule

Do not infer project state from an old milestone report or dated audit.

- **Current state:** this document.
- **Product direction and release gates:** [`ROADMAP.md`](../../ROADMAP.md).
- **Operational progress:** the GitHub Project governed by
  [`docs/production/GITHUB_GOVERNANCE.md`](../production/GITHUB_GOVERNANCE.md).
- **History:** [`CHANGELOG.md`](../../CHANGELOG.md) and `docs/audit/`.
