# Onboarding Paths by Profile

> **Status:** maintained paths.

Every profile starts with [`docs/project/STATUS.md`](../project/STATUS.md) for
current state. Old milestone reports under `history/` provide historical context
only.

## STRE Core developer

1. [Vision](../project/VISION.md)
2. [Current Status](../project/STATUS.md)
3. [System Overview](../architecture/SYSTEM_OVERVIEW.md)
4. [Network Protocol](../architecture/NETWORK_PROTOCOL.md)
5. [World Sync](../features/world-sync/README.md)
6. [Trading Technical Design](../features/trading/TECHNICAL_DESIGN.md)
7. [Mod Integration Framework](../architecture/MOD_INTEGRATION_FRAMEWORK.md)
8. ADRs 0001, 0002, 0004, 0007, 0009, and 0014.

**First exercise:** add a failure/recovery test or extract a testable pure policy.

## Creation Kit modder

1. [Alternate Start](../features/alternate-start/README.md)
2. Product Spec
3. CK Implementation
4. Solo Design
5. [CK/STRE Bridge](../architecture/CK_STRE_BRIDGE.md)
6. Campaign State
7. ADRs 0003, 0006, and 0012.

The M7 report is available under `docs/features/alternate-start/history/` for
traceability, but it is not the current source of truth.

**First exercise:** modify a CK record, pass the audits, compile, and provide an
in-game test.

## UI developer

1. [Current Status](../project/STATUS.md)
2. Trading Product Spec
3. Item Preview Current API
4. Item Preview Platform
5. ADR-0011.

## Narrative designer

1. Vision
2. Narrative Bible
3. Valen Character Bible
4. Alternate Start Product Spec
5. Dialogue Script.

## Character artist

1. Valen Character Bible
2. Art Direction
3. Valen Art Brief
4. Asset Pipeline.

## Voice actor / audio

1. Valen Character Bible
2. Dialogue Script
3. Voice Brief
4. Audio Direction/Pipeline.

## QA

1. [Current Status](../project/STATUS.md)
2. [Test Strategy](../testing/TEST_STRATEGY.md)
3. [Acceptance index](../testing/ACCEPTANCE_TESTS.md)
4. [Multiplayer Runbook](../testing/MULTIPLAYER_TEST_RUNBOOK.md)
5. [Compatibility Matrix](../testing/COMPATIBILITY_MATRIX.md)
6. The system's feature-specific `TEST_PLAN.md`.

**First exercise:** run a two-PC scenario with complete SHA, configurations,
and logs.
