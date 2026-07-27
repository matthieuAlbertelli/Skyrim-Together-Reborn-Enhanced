# Carte des dépendances

> **Statut : Mise à jour après Character Build M7**

## Chaîne implémentée Character Build

```text
Quête CK + RaceMenu
  → Character Creation Angular
  → sélections logiques
  → CharacterBuildCatalog v5
  → CharacterBuildService serveur ou fallback local
  → inventaire + sorts canoniques
  → application client + hashes
  → état Applied
```

Dépendances de cohérence :

```text
STRE_AlternateStart.esp
  ↔ CK_RECORDS_M7_IMPLEMENTED.json
  ↔ CharacterBuildCatalog.cpp
  ↔ character-loadouts.ts
  ↔ tests/audits
```

## Chaîne Alternate Start complète

```text
Character Build M7 (acquis)
  → persistance et character binding
  → Campaign State minimal
  → roster et ready check
  → Valen / introduction
  → départ et reprise vanilla
  → late join / reconnexion
```

## Chaîne Preview

```text
NativeSession + Controller + Solver + RasterMeasurer + Host
  → consommateurs Trading et Character Creation
  → futur Lease Manager
  → API interne stable
  → SDK tiers expérimental
```

## Chaîne Trading

```text
Trade Session Domain
  → Protocol Messages
  → Server TradeService
  → Client TradeService
  → Angular UI
  → Item Preview

Inventory snapshots
  → Mutation plan
  → Client apply journals
  → Server commit
  → Absolute reconciliation when uncertain
```

## Gates

- Persistance/character binding avant Campaign State complet.
- Skip Helgen testé avant d’annoncer Alternate Start comme remplacement du nouveau jeu.
- Script Valen verrouillé avant voix/lipsync.
- Lease manager avant SDK preview tiers.
- Au moins une intégration first-party supplémentaire avant de stabiliser le SDK d’adapters.
- Les nouveaux records CK doivent passer les audits manifest et catalogue avant intégration.
