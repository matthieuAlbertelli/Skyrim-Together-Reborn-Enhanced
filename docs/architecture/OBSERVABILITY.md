# Observability and Logging

> **Status:** active cross-cutting policy; each feature must complete its coverage.

Feature-specific markers may live in its `TEST_PLAN.md`. This document defines
what STRE logs must diagnose globally.

## Correlatable identifiers

A critical transition includes, as applicable:

- subsystem;
- player or server ID;
- session, build, or WorldEntity ID;
- request, apply, or reconcile ID when present;
- revision or version;
- `GameId` or `PlacedReferenceId` when required for identity;
- result or rejection code;
- explicit fallback or timeout;
- duration when relevant.

## Current subsystems

### World Sync

Logs must correlate:

```text
client authority
↔ server
↔ observer client
```

across:

- creation and adoption;
- binding and materialization;
- manipulation authority;
- hide and release;
- settlement;
- reconciliation;
- ownership and theft;
- forced release;
- timeout and disconnect.

### Trading

Session, revision, apply, and reconcile IDs must trace a complete saga and its
recovery.

### Character Build

Logs must correlate:

- logical selections;
- `BuildVersion`;
- inventory and spell hashes;
- accepted, applied, and rejected outcomes;
- form resolution;
- local application.

### Item Preview

Detailed rendering and raster logs must remain filterable and must not obscure
functional transitions.

## Levels

- `info` — important normal transition;
- `warn` — fallback, timeout, stale state, or recoverable incompatibility;
- `error` — broken invariant, impossible application, or inconsistent snapshot;
- `debug/trace` — high-frequency detail or targeted diagnostics.

## Temporary logs

Very verbose diagnostics added to isolate a crash must be:

- removed after validation;
- converted into an appropriately leveled stable log;
- or retained only behind a debug/trace level.

Hotfix markers must not become a permanent documentation API.

## Support bundle

For a multiplayer reproduction, retain at least:

- STRE SHA;
- Skyrim runtime;
- server configuration;
- relevant SKSE plugin versions;
- load order where relevant;
- each player's client logs;
- server log;
- test steps and time.

Future campaign secrets must be filtered from standard bundles.
