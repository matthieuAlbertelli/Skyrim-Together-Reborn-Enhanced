# Item Preview — Current Internal API

> **Status:** implemented for Trading and Character Creation; not stable for
> third parties.

## Consumers

### Trading

`TradeItemPreviewService` resolves a `Trade::ItemId`, builds the native entry,
and controls the preview.

### Character Creation

`CharacterCreationService` maps `previewKey` values to vanilla forms or
`STRE_AlternateStart.esp`, then sends the selection and region to the preview
pipeline.

## Generic core

### `ItemPreviewController`

Stores selection, region, revisions, fitting, reload, and active state.

### `ItemPreviewNativeSession`

Encapsulates `Inventory3DManager::Begin3D`, load/restart/clear, and `End3D`.

### `ItemPreviewHostSession`

An atomic show/hide state machine that absorbs concurrent messages.

### `ItemPreviewHostBridge`

A thread-safe singleton to which only one `ItemPreviewHostClient` can be bound.
`ItemPreviewHostBinding` manages bind/unbind through RAII.

### `ItemPreviewFitSolver`

A pure function that calculates position and scale from raster bounds.

### `ItemPreviewRasterMeasurer`

Captures D3D11 before and after rendering and measures the model in the target
region.

## What this API already supports

- share the pipeline between two first-party features;
- display real Skyrim objects in an Angular region;
- recalculate framing after resize or rapid changes;
- test the solver independently;
- isolate the host menu from the consumer.

## What it does not guarantee

- concurrent coexistence of several consumers;
- leases, ownership, or priority;
- a stable ABI;
- calls from Papyrus or an external mod;
- cross-version compatibility;
- safety for a third-party network payload;
- a useful 3D preview for every `MagicItem`.

Public communication must say **“reusable internal API foundation”**, not
**“third-party mod SDK.”**
