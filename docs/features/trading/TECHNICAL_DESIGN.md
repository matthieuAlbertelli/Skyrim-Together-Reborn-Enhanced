# Trading — Technical Design

> **Status:** implemented.

## Architecture

```text
Trade domain (pure)
├─ Session
├─ Inventory planning
├─ Application
└─ Reconciliation

Protocol
├─ 7 client requests
└─ 6 server notifications

Server TradeService
├─ session registry
├─ player/session index
├─ application registry
├─ reconciliation baselines
└─ cleanup/expiry

Client
├─ TradeService
├─ TradeMenuService
├─ TradeItemPreviewService
└─ Angular Trade UI
```

## State machine

```text
PendingAcceptance
  ├─ accept → Negotiating
  └─ reject/expire/disconnect → Cancelled

Negotiating
  ├─ offer update → Negotiating + revision++ + confirmations reset
  ├─ both confirm current revision → Locked
  └─ cancel/disconnect → Cancelled

Locked
  ├─ valid inventory plan → Applying
  └─ validation failure → Failed

Applying
  ├─ both success + server commit → Completed
  ├─ uncertain/failure → reconciliation
  └─ timeout/disconnect → reconciliation or terminal failure
```

## Idempotence

- identical offer: success without change;
- repeated acceptance: success without change;
- repeated confirmation of the same revision: success without change;
- client application: result remembered by `ApplyId`;
- client reconciliation: result remembered by `ReconcileId`;
- complete offer snapshots instead of UI deltas.

## Inventory validation

The server builds one `InventorySnapshot` per player, validates lines, detects
ambiguity and incompatibility, then creates two `PlayerMutationPlan` values.
Reciprocal offers for the same item are netted deterministically.

## Application and commit

The server sends each participant only their plan. Clients apply mutations to
their local Skyrim inventory and return a structured result. When both succeed,
the server applies the plan to its `InventoryComponent` values.

## Reconciliation

An absolute baseline is captured at the start of application. Under uncertainty,
the server creates or reuses a `ReconciliationPlan` containing target quantities
per item. Clients converge to those values instead of replaying a delta.

## Cleanup

- retain a terminal session briefly to absorb late messages;
- release the player/session index;
- remove applications and baselines;
- expire periodically.

## Extension points

- `TransferPolicy` to move `IsMvpTransferable` out of services;
- `InventoryAdapter` to test application without Skyrim;
- optional saga persistence;
- structured telemetry;
- future framework capability `player-trade/1`.
