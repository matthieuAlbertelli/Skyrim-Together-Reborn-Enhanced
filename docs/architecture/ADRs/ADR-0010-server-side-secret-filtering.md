# ADR-0010 — Narrative Secrets Are Filtered by the Server

- **Status:** Accepted
- **Date:** 2026-07-19

## Context

Hiding the Dragonborn's identity only in the UI does not protect it from logs or
memory inspection.

## Decision

The server produces audience-specific snapshot views. An unauthorized client
never receives secret data.

## Consequences

- stronger narrative integrity;
- public/private schemas or filterable fields;
- audience tests;
- protected server logs.
