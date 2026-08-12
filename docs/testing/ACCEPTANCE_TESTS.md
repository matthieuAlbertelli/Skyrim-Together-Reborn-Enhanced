# Acceptance-test index

> **Status: Canonical index; detailed scenarios live with each feature**

This file does not duplicate test cases. It points to the detailed acceptance criteria.

| Feature | Canonical plan |
|---|---|
| World Sync | [`../features/world-sync/TEST_PLAN.md`](../features/world-sync/TEST_PLAN.md) |
| Trading | [`../features/trading/TEST_PLAN.md`](../features/trading/TEST_PLAN.md) |
| Item Preview | [`../features/item-preview/TEST_PLAN.md`](../features/item-preview/TEST_PLAN.md) |
| Alternate Start / Character Build | [`../features/alternate-start/TEST_PLAN.md`](../features/alternate-start/TEST_PLAN.md) |
| Downed State | document the plan under `../features/downed-state/` when implementation begins |

## Cross-cutting tests

Common environment, evidence, logging, compatibility, and reproducibility rules live in:

- [`TEST_STRATEGY.md`](TEST_STRATEGY.md)
- [`MULTIPLAYER_TEST_RUNBOOK.md`](MULTIPLAYER_TEST_RUNBOOK.md)
- [`COMPATIBILITY_MATRIX.md`](COMPATIBILITY_MATRIX.md)

When a new scenario concerns only one feature, add it to that feature's `TEST_PLAN.md` instead of this index.
