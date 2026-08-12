# Trading — Protocol Reference

> **Status:** implemented.

## Identifiers

| Field | Type | Role |
|---|---|---|
| `SessionId` | uint64 | logical transaction |
| `Revision` | uint64 | global offer version |
| `ApplyId` | uint64 | application attempt |
| `ReconcileId` | uint64 | reconciliation attempt |
| `PlayerId` | uint32 | participant |
| `ItemId` | uint64 | `ModId << 32 | BaseId` |

## Client → server

| Message | Data | Precondition |
|---|---|---|
| Invite | target player | no active trade |
| InviteResponse | session, accepted | recipient |
| OfferUpdate | session, expected revision, full offer | negotiating |
| Confirm | session, revision | current revision |
| Cancel | session | participant, not applying |
| ApplyResult | session, revision, apply ID, result | expected application |
| ReconcileResult | session, revision, apply/reconcile IDs, result | expected reconciliation |

## Server → client

| Message | Role |
|---|---|
| Invite | display invitation |
| Started | create local context |
| State | authoritative session snapshot |
| Apply | signed local mutations |
| Reconcile | absolute target quantities |
| Cancelled | cancellation reason |

## Bounds

- 64 lines per offer;
- 64 mutations per plan;
- 64 targets per reconciliation;
- decoder invalidates the structure when a bound is exceeded.

## Future compatibility

Every structural change specifies:

- protocol version;
- minimum clients;
- downgrade strategy;
- behavior of an older server;
- round-trip and malformed-payload tests.
