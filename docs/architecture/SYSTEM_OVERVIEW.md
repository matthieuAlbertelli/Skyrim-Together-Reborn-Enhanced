# Vue d’ensemble du système

> **Statut : architecture transversale courante**

Ce document décrit les frontières générales. Les détails d’une feature appartiennent à `docs/features/<feature>/`.

## Vue générale

```text
Skyrim / Creation Kit / SKSE plugins
        │ local events + projections
        ▼
STRE client adapters/services
        │ intents / results
        ▼
versioned network protocol
        │
        ▼
STRE server authority
        │ canonical notifications / snapshots
        ▼
STRE client projection
        │
        ▼
Skyrim / UI / local engine simulation
```

## Runtime

Les `World` client et serveur enregistrent leurs services dans le contexte EnTT. Le bus `entt::dispatcher` relie messages réseau, updates et événements de jeu.

Les messages sont des types statiques enregistrés dans les factories du protocole.

## Verticales first-party actuelles

### World Sync

World Sync ajoute une identité stable aux instances monde synchronisées sans tenter de transformer le serveur en moteur Havok.

```text
Skyrim object event
→ client WorldEntity lifecycle
→ server identity/authority
→ remote binding/materialization
→ local engine simulation
→ authoritative settlement
```

Voir [`features/world-sync/README.md`](../features/world-sync/README.md).

### Trading

```text
Angular action
→ client Trade services
→ server TradeService
→ Trade domain
→ canonical apply/reconcile messages
→ client inventory/UI projection
```

Voir [`features/trading/`](../features/trading/).

### Alternate Start / Character Build

```text
CK quest/RaceMenu
→ CharacterCreationService
→ Angular logical selections
→ CharacterBuildService or local fallback
→ canonical inventory/spells
→ local application + acknowledgement
```

Voir [`features/alternate-start/`](../features/alternate-start/).

### Item Preview

La preview est une ressource native interne partagée par Trading et Character Creation. Sa cible de lease/arbitration est documentée séparément.

Voir [`ITEM_PREVIEW_PLATFORM.md`](ITEM_PREVIEW_PLATFORM.md).

## Frontières

### Skyrim adapters

Responsables des appels engine-facing, résolution de formulaires, événements TES, matérialisation et application locale.

Ils ne doivent pas devenir la source canonique d’un état partagé uniquement parce qu’ils détiennent une référence native.

### STRE client services

Responsables de l’orchestration locale, de la traduction événement → intention et de l’application des résultats/snapshots.

Une mutation moteur déclenchée par le réseau doit être marshalled sur un contexte sûr lorsque nécessaire.

### Shared domain/protocol

Responsable des identités portables, structures sérialisées, règles métier partagées et bornes.

Aucun pointeur Skyrim natif ne traverse cette frontière.

### STRE server

Responsable de la validation et de l’autorité des états partagés explicitement confiés au serveur.

## Principes transverses

- autorité explicite;
- identité réseau distincte des FormID locaux lorsque nécessaire;
- KISS : utiliser le moteur local pour ce qu’il fait déjà correctement;
- DRY : une règle ou un état mutable ne possède qu’une source de vérité;
- fail closed si une opération ne sait pas préserver les métadonnées requises;
- snapshots pour les systèmes qui doivent reconstruire un état après join/reconnect;
- aucune API tierce annoncée stable avant validation first-party suffisante.

## Architecture future

Campaign State, persistence durable et Mod Integration Runtime générique étendront ces mêmes frontières sans remplacer les contrats first-party déjà éprouvés.
