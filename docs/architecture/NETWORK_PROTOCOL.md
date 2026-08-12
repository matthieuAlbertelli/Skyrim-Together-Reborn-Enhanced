# STRE Network Protocol Rules

> **Status:** active cross-cutting contract.

This document defines the STRE protocol's **shared rules**. Feature-specific
message lists, fields, and transitions belong in
`docs/features/<feature>/PROTOCOL_REFERENCE.md`.

## General model

```text
Client
  → intent / application result
Server
  → validation / canonical transition
Server
  → notification or snapshot
Client
  → local Skyrim/UI projection
```

A client does not become the source of truth for shared state merely because it
observed or displayed that state.

## Categories

### Client → server intents

An intent describes what the player is trying to do.

It contains only the identifiers and data required for validation, with bounded
collections.

### Client → server results and acknowledgements

When the server requests local application that can fail, the client may return
a correlated result so the server can commit, reconcile, or explicitly abandon
the operation.

### Server → client notifications

A notification describes an accepted transition or canonical state. Clients
apply the local projection idempotently when required by the domain.

### Snapshots

A reconnectable system must provide a canonical snapshot path covering the state
required for local reconstruction.

## Identity

- loaded Skyrim `FormID` values are not generic network identifiers;
- persistent forms use server-space identities (`GameId`) where appropriate;
- dynamic world instances use dedicated identities such as `WorldEntityId`;
- request and revision identifiers must be explicit when concurrency or
  retransmission requires them.

## Bounds and validation

- every decoded collection has a bound;
- every received enum is validated;
- every identity is resolved before mutation;
- malformed payloads are rejected;
- a feature must not silently accept data it cannot preserve.

## Threading

Receiving a network message does not authorize arbitrary Skyrim engine calls.

Handlers that mutate the game must marshal work to an appropriate context, such
as STRE's `RunnerService`/`OnUpdate` path.

## Compatibility

Every schema change specifies:

- client/server compatibility;
- rejection or migration strategy;
- round-trip tests;
- behavior for old or malformed payloads.

Append-only changes are not automatically safe: encode/decode behavior must
remain consistent on both sides.

## Feature references

- [Trading protocol](../features/trading/PROTOCOL_REFERENCE.md)
- [World Sync protocol](../features/world-sync/PROTOCOL_REFERENCE.md)
- Alternate Start / Character Build: document details in its feature directory;
  do not copy the message list here.

## Future Mod Integration

A future generic adapter envelope must remain versioned, bounded, and
negotiable. It does not replace first-party protocols until several real
integrations have validated it.
