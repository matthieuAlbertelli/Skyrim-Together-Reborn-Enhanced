# Carte des dépendances

> **Statut : Proposition**

## Chaîne Alternate Start

```text
Vision produit
  └─ Narrative Bible
      ├─ Valen Character Bible
      │   ├─ Valen Art Brief → modèle/texture → intégration CK
      │   └─ Valen Voice Brief → casting → enregistrement → lipsync
      └─ Alternate Start Product Spec
          ├─ Solo Design → CK Implementation
          └─ STRE Adapter Spec
              ├─ Mod Integration Framework
              ├─ CK/STRE Bridge
              └─ Campaign State
```

## Chaîne Preview

```text
ItemPreviewNativeSession
  + ItemPreviewController
  + ItemPreviewFitSolver
  + ItemPreviewRasterMeasurer
  + Host Menu
        ↓
Preview Runtime / Lease Manager
        ↓
Trade consumer + futurs consommateurs
        ↓
SDK tiers expérimental
```

## Chaîne Trading

```text
Trade Session Domain
  → Protocol Messages
  → Server TradeService
  → Client TradeService
  → TradeMenuService / Angular UI
  → Item Preview consumer

Inventory snapshots
  → Mutation plan
  → Client apply journals
  → Server commit
  → Absolute reconciliation when uncertain
```

## Gates de démarrage

- Le modèle de Valen ne commence pas avant validation de la bible personnage et du brief art.
- L’enregistrement principal ne commence pas avant verrouillage du script et test de prononciation.
- L’adaptateur Alternate Start ne commence pas avant définition du Campaign State minimal et du bridge.
- Le SDK tiers ne commence pas avant validation du runtime first-party avec Alternate Start.
