# ADR-0001 — Ports and Adapters for Mod Integrations

- **Status:** Accepted
- **Date:** 2026-07-19

## Context

STRE wants to let a single-player mod describe its multiplayer semantics without
integrating every detail of that mod into the network core.

## Decision

Adopt a Plugin Architecture/Microkernel in which every integration is an
`STRE Mod Adapter`. The core exposes ports for capabilities, intents, canonical
state, events, snapshots, and policies. Adapters translate the mod's concepts.

## Consequences

- clear separation and testability;
- preserved single-player operation;
- initial runtime design cost;
- contracts must be versioned;
- no promise of automatic compatibility.
