# ADR-0009 — Adapters first-party avant SDK tiers

- **Statut : Accepted**
- **Date : 2026-07-19**

## Contexte

Une ABI ou un protocole public figé trop tôt serait coûteux à maintenir.

## Décision

Implémenter Alternate Start comme adapter compilé dans STRE. Stabiliser les concepts avec au moins deux integrations avant de publier un SDK tiers expérimental.

## Conséquences

- apprentissage rapide et liberté de refactor ;
- ouverture externe retardée ;
- les interfaces internes doivent néanmoins être documentées et testées.
