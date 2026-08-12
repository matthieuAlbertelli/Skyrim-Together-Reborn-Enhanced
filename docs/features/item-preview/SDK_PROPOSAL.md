# Item Preview — SDK Proposal

> **Status:** proposed; Character Creation demonstration completed without a
> public SDK.

## Current evidence

Character Creation already displays starting equipment through the same native
core as Trading. This validates internal reuse, but still uses first-party
services and the single-client bridge.

## Target capability

`stre.item-preview/1`

## Logical request

```json
{
  "owner": "stre.alternate-start.class-menu",
  "item": { "mod": 1, "base": 12345 },
  "region": { "left": 0.62, "top": 0.18, "width": 0.31, "height": 0.58 },
  "lightScheme": 1,
  "priority": "standard"
}
```

## Target lifecycle

1. `AcquirePreview` → token;
2. `UpdatePreview` → item and region;
3. `PreviewSuspended` when preempted;
4. `PreviewResumed`;
5. `ReleasePreview`;
6. automatic release on destruction or timeout.

## Permissions

- compatible capability and version;
- locally resolved item;
- authorized UI surface;
- rate limit;
- no access to a native pointer or texture.

## Errors

- `UNSUPPORTED`
- `RESOURCE_BUSY`
- `INVALID_ITEM`
- `INVALID_REGION`
- `OWNER_NOT_ACTIVE`
- `NATIVE_MANAGER_UNAVAILABLE`
- `HOST_MENU_FAILURE`
- `DEVICE_FAILURE`

## Next demonstration

Run Trading and Character Creation with real leases, then induce and verify
controlled preemption. This test must precede any third-party SDK announcement.
