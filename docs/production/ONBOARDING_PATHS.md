# Parcours d’onboarding par profil

> **Statut : parcours maintenus**

Tous les profils commencent par [`docs/project/STATUS.md`](../project/STATUS.md) pour l’état courant. Les anciens rapports de jalon sous `history/` servent uniquement de contexte historique.

## Développeur STRE Core

1. [Vision](../project/VISION.md)
2. [Current Status](../project/STATUS.md)
3. [System Overview](../architecture/SYSTEM_OVERVIEW.md)
4. [Network Protocol](../architecture/NETWORK_PROTOCOL.md)
5. [World Sync](../features/world-sync/README.md)
6. [Trading Technical Design](../features/trading/TECHNICAL_DESIGN.md)
7. [Mod Integration Framework](../architecture/MOD_INTEGRATION_FRAMEWORK.md)
8. ADRs 0001, 0002, 0004, 0007, 0009 et 0014.

**Premier exercice :** ajouter un test de panne/recovery ou extraire une policy pure testable.

## Moddeur Creation Kit

1. [Alternate Start](../features/alternate-start/README.md)
2. Product Spec
3. CK Implementation
4. Solo Design
5. [CK/STRE Bridge](../architecture/CK_STRE_BRIDGE.md)
6. Campaign State
7. ADRs 0003, 0006 et 0012.

Le rapport M7 est disponible sous `docs/features/alternate-start/history/` pour la traçabilité, mais n’est pas la source de vérité courante.

**Premier exercice :** modifier un record CK, passer les audits, compiler et fournir un test en jeu.

## Développeur UI

1. [Current Status](../project/STATUS.md)
2. Trading Product Spec
3. Item Preview Current API
4. Item Preview Platform
5. ADR-0011.

## Narrative Designer

1. Vision
2. Narrative Bible
3. Valen Character Bible
4. Alternate Start Product Spec
5. Dialogue Script.

## Character Artist

1. Valen Character Bible
2. Art Direction
3. Valen Art Brief
4. Asset Pipeline.

## Comédien / Audio

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
6. Feature-specific `TEST_PLAN.md` for the system under test.

**Premier exercice :** exécuter un scénario à deux PC avec SHA, configs et logs complets.
