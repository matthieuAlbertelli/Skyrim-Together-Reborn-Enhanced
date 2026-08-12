# ADR-0013 — Refactor Preview into Dedicated Components

> **Status:** Implemented
> **Origin:** migration and enrichment of the former
> `docs/decisions/ADR-0001-preview-refactor.md`.

## Context

The first 3D preview implementation was tightly coupled to Trading. This made it
difficult to test responsibilities, stabilize the native lifecycle, and reuse
the preview in future STRE screens.

## Decision

Gradually extract responsibilities into dedicated components:

- `ItemPreviewController` for orchestration and fitting state;
- `ItemPreviewNativeSession` for the `Inventory3DManager` lifecycle;
- `ItemPreviewHostSession` for idempotent host-menu opening and closing;
- `ItemPreviewHostBridge` for RAII binding to the consumer;
- `ItemPreviewFitSolver` for transform calculation;
- `ItemPreviewRasterMeasurer` for D3D11 measurement;
- `TradePreviewHostMenu` as the invisible native Scaleform host.

The Trading service becomes an adapter that consumes these components instead
of owning every rendering responsibility.

## Positive consequences

- easier-to-audit responsibilities;
- isolated native lifecycle;
- a real foundation for a second consumer;
- preparation for the future leased runtime;
- less coupling to the Trading business domain.

## Remaining limitations

- the bridge still accepts only one consumer;
- the host and some logs remain named after Trading;
- no stable third-party mod contract is exposed;
- solver and multi-consumer arbitration tests remain to be added.

## Relationship to other decisions

ADR-0013 describes the **implemented** refactor. ADR-0008 describes the
**proposed** evolution toward a leased multi-consumer runtime.

## Additional implementation state

Character Creation is now a second first-party consumer of the preview core. The
bridge's single-client limitation and lack of leases remain unchanged.
