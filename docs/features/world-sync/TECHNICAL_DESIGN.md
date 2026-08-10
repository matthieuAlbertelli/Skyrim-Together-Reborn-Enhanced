# World Sync — Technical design

> **Statut : Implémenté pour dropped items + lazy placed-reference adoption**

## Responsabilités

### Serveur

Le serveur possède :

- allocation/résolution de `WorldEntityId`;
- index `PlacedReferenceId -> WorldEntityId`;
- lifecycle;
- autorité de manipulation;
- état de settlement;
- snapshot de session.

### Client

Le client possède :

- binding `WorldEntityId <-> référence Skyrim locale`;
- matérialisation des drops;
- résolution d’une référence placée existante;
- observation des événements de grab/release;
- Havok local;
- sampling de stabilité;
- application des corrections ponctuelles.

## Identité

### Drop dynamique

Une référence temporaire créée par Skyrim n’a pas le même FormID chez tous les clients.

STRE lui attribue donc un `WorldEntityId` stable côté session.

### Référence placée

La clé de déduplication serveur est :

```text
PlacedReferenceId = GameId de la TESObjectREFR
```

Le serveur adopte la référence à la première interaction réseau pertinente. Chaque client résout ensuite sa propre référence locale correspondante.

## Drop et settlement

Le client autorité laisse Havok évoluer localement puis observe la stabilité.

Le settlement est borné :

- sampling périodique;
- durée minimale avant stabilité;
- durée maximale avec fallback sur le dernier transform connu;
- envoi d’un transform final;
- correction distante uniquement si la divergence dépasse la tolérance définie.

L’objectif est de converger sans appeler continuellement `MoveTo`/équivalent pendant la simulation.

## Manipulation

État logique :

```text
FREE
  -> Start
MANIPULATED(authorityPlayerId)
  -> Release
SETTLING(authorityPlayerId)
  -> final transform
FREE
```

Pendant `MANIPULATED` :

- Better Grabbing gère le déplacement local;
- le serveur reçoit seulement les informations nécessaires au lifecycle/heartbeat;
- les observateurs cachent leur représentation;
- aucun transform intermédiaire n’est streamé aux observateurs.

Au release :

- un drop dynamique reprend son chemin normal de matérialisation/Havok;
- une référence placée est repositionnée via le chemin `TESObjectREFR::MoveTo` déjà présent dans STR;
- l’appel moteur est exécuté via `RunnerService`/`OnUpdate`;
- le settlement final reprend ensuite.

## Engine safety

Ne pas réintroduire des wrappers `SetPosition`/`SetAngle` basés sur une signature d’adresse supposée.

Le crash observateur de référence placée a démontré que cette ABI était incorrecte. Le chemin validé est la primitive `MoveTo` STR existante, appelée sur le contexte de jeu approprié.

## Ownership

`Inventory::Entry` porte un owner server-space lorsqu’il peut être résolu.

Conséquences :

- instances identiques avec owners différents ne fusionnent pas;
- l’owner peut survivre aux chemins inventory/world supportés;
- `ExtraOwnership` est restauré lors d’une reconstruction appropriée;
- le grab d’une référence placée non autorisée déclenche `StealAlarm`.

Un simple booléen `IsStolen` n’est pas utilisé comme source de vérité.

## Dialogue / forced release

Le `Dialogue Menu` Skyrim n’est pas suffisant pour arrêter Better Grabbing à lui seul.

Quand ce menu s’ouvre pendant une manipulation locale, STRE demande une fin de grab native. Le `TESGrabReleaseEvent` existant poursuit ensuite le lifecycle réseau normal, y compris lorsqu’une adoption était encore en attente.

## Snapshot / late join

Un client rejoignant après création/adoption doit :

- recevoir l’état WorldEntity courant;
- matérialiser les drops nécessaires;
- lier les références placées existantes sans duplication;
- appliquer le transform canonique utile;
- ne jamais recréer une entité déjà consommée.

## Non-goals actuels

- simulation physique serveur;
- streaming frame-by-frame des rigid bodies;
- duplication des références vanilla placées;
- scan préalable de toutes les références mobiles;
- persistence disque complète;
- synchronisation de tous les ExtraData Skyrim sans politique explicite.
