# Trading — Test Plan

> **Status:** unit tests exist; integration remains incomplete.

## Existing coverage

Forty-four test cases cover the state machine, canonicalization, revisions,
inventory validation, mutation planning, application, reconciliation, and
serialization.

## Next priority

- complete client/server harness;
- duplicated and reordered packets;
- apply and reconcile timeouts;
- initiator and recipient disconnect at every state;
- failed server commit;
- client journal retransmitted after several operations;
- offer-update load test;
- Playwright UI test in browser mode;
- in-game test of excluded item types.
