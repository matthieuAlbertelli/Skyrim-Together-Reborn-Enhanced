# Technical Risk Register

> **Status:** source of truth for active technical risks.
> **Last updated:** August 10, 2026.

| ID | Risk | Impact | Current mitigation |
|---|---|---:|---|
| R-01 | Significant upstream divergence | High | isolated patches, ADRs, regular integrations |
| R-02 | Engine mutation from the wrong thread | High | marshal through `RunnerService`/game update |
| R-03 | Reverse-engineered wrapper with assumed ABI | Critical | prefer validated STR primitives; require signature evidence before a new wrapper |
| R-04 | Remote Havok fought by transform streaming | High | ADR-0017: local Havok and point-in-time settlement |
| R-05 | Placed reference duplicated during adoption | High | `PlacedReferenceId -> WorldEntityId` and binding to the existing local reference |
| R-06 | Scripted or quest object affected by hide/enable/reposition | High | dedicated validation campaign before guaranteed support |
| R-07 | Instance metadata lost during transfer | High | enriched `Inventory::Entry`; fail closed when a protocol cannot preserve data |
| R-08 | Custom name lost | Medium | explicitly unsupported `ExtraTextDisplayData` until crash-safe |
| R-09 | WorldEntity state lost after restart or save branch | High | future versioned persistence and checkpoints |
| R-10 | Better Grabbing changes behavior or internal API | Medium | depend on Skyrim events and behavior, not internals |
| R-11 | Required native plugin missing or incompatible | High | generic NativePlugins policy; version constraints remain to be studied |
| R-12 | Trading saga leaves uncertain state on failure | High | idempotence and absolute reconciliation |
| R-13 | Single-client preview blocks real concurrency | High | future lease manager |
| R-14 | Character Build lost after reconnect or restart | High | persistence and versioning before complete Campaign State |
| R-15 | Helgen bypass leaves vanilla state inconsistent | High | stage/global matrix and recovery tests |
| R-16 | Incomplete anti-import cleanup | High | skills/perks/attributes policy and before/after tests |
| R-17 | Remote buffs rely on a nominal allowlist | Medium | extensible capability classification before expansion |
| R-18 | Documentation diverges through duplication | High | source-of-truth matrix, feature-local docs, history archive |

## Rule

A resolved risk is not kept here indefinitely as active state. Its resolution
belongs in the changelog, an ADR, or Git history as appropriate.
