# ADR-0006 — No Hard-Coded Plugin FormIDs

- **Status:** Accepted
- **Date:** 2026-07-19

## Context

FormIDs depend on load order and make integrations fragile.

## Decision

Use Papyrus properties, configured references, controlled Editor IDs, and
resolution by mod/base ID where necessary.

## Consequences

- more robust installation;
- more explicit initial configuration;
- property-validation tools are required.

## Implementation state

The M7 catalog resolves `PluginName + LocalFormId`, CK aliases provide seat
references, and audits cross-check local IDs. No loaded load-order prefix is
used for STRE records.
