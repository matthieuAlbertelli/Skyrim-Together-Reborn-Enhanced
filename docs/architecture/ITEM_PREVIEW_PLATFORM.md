# Item Preview Platform Architecture

> **Status:** core reused by two consumers; leased runtime proposed.

## Current state

The code has solid generic components:

- native session;
- controller;
- solver;
- raster measurement;
- host session;
- RAII host bridge.

Two first-party consumers are active:

- `TradeItemPreviewService` for Trading;
- Character Creation for class and skill equipment.

Each consumer resolves a real Skyrim object, supplies an Angular/CEF region, and
uses the same loading, measurement, and framing pipeline.

## Remaining problem

`ItemPreviewHostBridge` still accepts only one `ItemPreviewHostClient` at a time.
Sequential reuse is validated, but the runtime handles neither concurrency,
priority, explicit ownership, nor resource timeout.

## Target API

```cpp
struct ItemPreviewRequest
{
    PreviewOwnerId Owner;
    GameId Item;
    PreviewRegion Region;
    LightScheme Light;
    PreviewPriority Priority;
};

class ItemPreviewLease
{
public:
    void Update(const ItemPreviewRequest&) noexcept;
    void Release() noexcept;
};

class ItemPreviewRuntime
{
public:
    PreviewAcquireResult Acquire(const ItemPreviewRequest&) noexcept;
};
```

## Arbitration

- one active native session;
- explicit priority;
- notified preemption;
- idempotent closing;
- restoration of the surface and input;
- no consumer manipulates the host menu directly.

## Target separation

```text
ItemPreviewRuntime
├─ LeaseManager
├─ ItemResolver
├─ Controller
├─ NativeSession
├─ HostMenuAdapter
├─ RasterMeasurementAdapter
└─ Telemetry
```

Trading and Character Creation become runtime consumer adapters.

## Third-party mod API

1. stabilize leases between first-party consumers;
2. add an internal `item-preview/1` capability;
3. provide a UI/Mod Adapter bridge using canonical `GameId` values;
4. publish a versioned third-party SDK.

An external mod must never receive a native Skyrim pointer or call
`Inventory3DManager` directly.

## Required tests

- pure solver: extreme sizes, screen edges, and clamps;
- idempotent acquisition and release;
- Trading ↔ Character Creation preemption;
- closing during reload;
- rapid item changes;
- region resizing;
- device loss or missing manager;
- conflicts with native menus.
