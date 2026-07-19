# ADR-0013 — Refactor de la preview en composants dédiés

> **Statut : Implemented**  
> **Origine :** migration et enrichissement de l’ancien `docs/decisions/ADR-0001-preview-refactor.md`

## Contexte

La première implémentation de la preview 3D était fortement couplée au système d’échange. Ce couplage rendait difficile le test des responsabilités, la stabilisation du cycle de vie natif et la réutilisation par de futurs écrans STRE.

## Décision

Extraire progressivement les responsabilités dans des composants dédiés :

- `ItemPreviewController` pour l’orchestration et l’état du fitting ;
- `ItemPreviewNativeSession` pour le cycle de vie d’`Inventory3DManager` ;
- `ItemPreviewHostSession` pour l’ouverture et la fermeture idempotentes du host menu ;
- `ItemPreviewHostBridge` pour le binding RAII vers le consommateur ;
- `ItemPreviewFitSolver` pour le calcul de transformation ;
- `ItemPreviewRasterMeasurer` pour la mesure D3D11 ;
- `TradePreviewHostMenu` comme host Scaleform natif non visible.

Le service de trading devient un adaptateur consommateur de ces composants plutôt que le propriétaire de toutes les responsabilités de rendu.

## Conséquences positives

- responsabilités plus faciles à auditer ;
- cycle de vie natif isolé ;
- base réelle pour un second consommateur ;
- préparation du futur runtime à leases ;
- réduction du couplage avec le domaine métier Trading.

## Limites restantes

- le bridge n’accepte encore qu’un seul consommateur ;
- le host et certains logs restent nommés autour du trading ;
- aucun contrat stable pour mods tiers n’est exposé ;
- les tests du solver et de l’arbitrage multi-consommateurs restent à ajouter.

## Relation avec les autres décisions

ADR-0013 décrit le refactor **déjà implémenté**. ADR-0008 décrit l’évolution **proposée** vers un runtime multi-consommateurs à leases.
