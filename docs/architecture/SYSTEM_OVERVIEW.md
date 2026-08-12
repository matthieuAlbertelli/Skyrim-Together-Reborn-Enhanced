# System Overview

> **Status:** current cross-cutting architecture.

This document describes general boundaries. Feature details belong in
`docs/features/<feature>/`.

## Overview

```text
Skyrim / Creation Kit / SKSE plugins
        │ local events + projections
        ▼
STRE client adapters/services
        │ intents / results
        ▼
versioned network protocol
        │
        ▼
STRE server authority
        │ canonical notifications / snapshots
        ▼
STRE client projection
        │
        ▼
Skyrim / UI / local engine simulation
```

## Runtime

Client and server `World` instances register their services in the EnTT context.
The `entt::dispatcher` bus connects network messages, updates, and game events.

Messages are static types registered in protocol factories.

## Current first-party verticals

### World Sync

World Sync adds stable identity to synchronized world instances without trying
to turn the server into a Havok engine.

```text
Skyrim object event
→ client WorldEntity lifecycle
→ server identity/authority
→ remote binding/materialization
→ local engine simulation
→ authoritative settlement
```

See [`features/world-sync/README.md`](../features/world-sync/README.md).

### Trading

```text
Angular action
→ client Trade services
→ server TradeService
→ Trade domain
→ canonical apply/reconcile messages
→ client inventory/UI projection
```

See [`features/trading/`](../features/trading/).

### Alternate Start / Character Build

```text
CK quest/RaceMenu
→ CharacterCreationService
→ Angular logical selections
→ CharacterBuildService or local fallback
→ canonical inventory/spells
→ local application + acknowledgement
```

See [`features/alternate-start/`](../features/alternate-start/).

### Item Preview

Preview is an internal native resource shared by Trading and Character Creation.
Its lease and arbitration target is documented separately.

See [`ITEM_PREVIEW_PLATFORM.md`](ITEM_PREVIEW_PLATFORM.md).

## Boundaries

### Skyrim adapters

Responsible for engine-facing calls, form resolution, TES events,
materialization, and local application.

They must not become the canonical source of shared state merely because they
hold a native reference.

### STRE client services

Responsible for local orchestration, event-to-intent translation, and applying
results and snapshots.

A network-triggered engine mutation must be marshalled to a safe context where
necessary.

### Shared domain/protocol

Responsible for portable identities, serialized structures, shared business
rules, and bounds.

No native Skyrim pointer crosses this boundary.

### STRE server

Responsible for validating and authoritatively owning shared state explicitly
entrusted to the server.

## Cross-cutting principles

- explicit authority;
- network identity distinct from local FormIDs where necessary;
- KISS: use the local engine for what it already does correctly;
- DRY: one source of truth for every rule or mutable state;
- fail closed when an operation cannot preserve required metadata;
- snapshots for systems that must reconstruct state after join/reconnect;
- no third-party API is declared stable before sufficient first-party validation.

## Future architecture

Campaign State, durable persistence, and a generic Mod Integration Runtime will
extend these same boundaries without replacing proven first-party contracts.
