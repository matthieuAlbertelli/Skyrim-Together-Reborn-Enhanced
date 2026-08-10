# World Sync — Protocol reference

> **Statut : Implémenté pour le périmètre WorldEntity actuel**

Ce document décrit les contrats spécifiques World Sync. Les règles générales de protocole sont dans [`docs/architecture/NETWORK_PROTOCOL.md`](../../architecture/NETWORK_PROTOCOL.md).

## Types partagés

### `WorldEntityId`

Identifiant serveur stable d’une instance monde pendant la session.

### `WorldEntityTransform`

Regroupe position et rotation utilisées par les messages WorldEntity.

### `PlacedReferenceId`

`GameId` server-space de la **référence** Skyrim placée, distinct du `BaseForm` de l’objet.

Il sert à l’adoption lazy et à la déduplication serveur.

## Inventory/world lifecycle

Les messages d’inventaire existants portent les extensions World Sync nécessaires.

### `RequestInventoryChanges`

Champs World Sync pertinents :

- données d’item;
- indication drop/pickup selon le flux existant;
- transform de création/finalisation quand applicable;
- `PlacedReferenceId` pour l’adoption d’une référence non temporaire.

### `NotifyInventoryChanges`

Peut porter :

- `WorldEntityId`;
- `PlacedReferenceId`;
- lifecycle physique;
- `LifecycleOnly`.

`LifecycleOnly` permet de retirer/mettre à jour la représentation physique sans appliquer une seconde mutation d’inventaire lorsque le flux vanilla/activation possède déjà cette mutation.

## Manipulation

### `RequestWorldEntityManipulation`

Porte notamment :

- `WorldEntityId` (0 lors d’une première adoption placée);
- `PlacedReferenceId` si nécessaire;
- `Action`;
- `WorldEntityTransform`.

Actions actuelles :

```text
Start
Update
Release
Rejected
```

`Update` est utilisé comme heartbeat privé d’autorité; il n’implique pas un streaming visuel des poses aux observateurs.

### `NotifyWorldEntityManipulation`

Diffuse les transitions nécessaires aux observateurs et à l’autorité.

Pour une référence placée, `PlacedReferenceId` permet au client de lier le `WorldEntityId` à sa référence locale existante.

## Autorité serveur

```text
FREE
  -> Start accepted
MANIPULATED(authorityPlayerId)
  -> Release
SETTLING(authorityPlayerId)
  -> final TransformUpdate
FREE
```

Règles :

- un second `Start` concurrent est rejeté;
- heartbeat et release ne sont acceptés que de l’autorité;
- timeout/disconnect libère l’autorité;
- le transform final de settlement devient la référence de convergence.

## Snapshot

Le snapshot WorldEntity doit permettre de reconstruire :

- identité;
- item/instance metadata nécessaire;
- origine dynamique ou référence placée;
- transform canonique pertinent;
- lifecycle courant.

Un snapshot ne doit pas créer une seconde référence locale pour un `PlacedReferenceId` déjà présent.

## Ownership metadata

L’owner est sérialisé dans `Inventory::Entry` via un `GameId` stable quand disponible.

L’ownership fait partie des métadonnées d’instance à préserver; il ne doit pas être remplacé par un simple booléen “stolen”.

## Compatibilité

Toute modification de ces structures exige au minimum :

- round-trip encode/decode;
- test d’un client autorité et d’un observateur;
- test de lazy adoption;
- test de snapshot/late join si l’état sérialisé change;
- comportement fail-closed si une ancienne voie ne sait pas conserver une nouvelle métadonnée.
