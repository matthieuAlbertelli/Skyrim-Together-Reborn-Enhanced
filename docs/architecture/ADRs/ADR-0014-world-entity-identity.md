# ADR-0014 — Network Identity Independent of Local FormIDs

- **Status:** Accepted
- **Date:** 2026-07-30

## Context

Temporary references created by Skyrim receive different local FormIDs on each
client and may change after loading.

## Decision

Every synchronized dynamic entity receives a server-assigned `WorldEntityId`.
Local FormIDs are kept only in a client binding registry versioned by generation.

## Consequences

- business messages never use a temporary FormID as durable identity;
- every client maintains `WorldEntityId ↔ local FormID`;
- bindings are rebuilt after loading;
- ambiguous reattachment is exposed as a reconciliation conflict.
