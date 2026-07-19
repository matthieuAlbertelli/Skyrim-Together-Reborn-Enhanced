# Stratégie de test

> **Statut : Proposition fondée sur les tests existants**

## Pyramide

### Tests purs

- automates de domaine ;
- validation et canonicalisation ;
- plans de mutation/réconciliation ;
- solver de preview ;
- transitions Campaign State ;
- policies d’autorité.

### Tests de sérialisation

- round-trip de tous messages ;
- tailles maximales ;
- payloads tronqués ;
- enums inconnus ;
- versions incompatibles.

### Tests de services

Avec doubles de transport/inventaire :

- invitations ;
- retransmissions ;
- timeouts ;
- déconnexion ;
- snapshot/replay ;
- commit failure.

### Tests client/serveur

Deux processus automatisés ou harness :

- ordre et perte de messages ;
- duplicate delivery ;
- reconnect ;
- version mismatch ;
- latence.

### Tests en jeu

- Skyrim réel ;
- CK plugin ;
- UI/preview ;
- sauvegarde/chargement ;
- 2, 4, 10 joueurs.

## Trading existant

44 tests couvrent session, application, inventory planning, protocole et réconciliation. Ajouter :

- intégration serveur/client ;
- commit failure ;
- disconnect à chaque étape ;
- stress de changements d’offre ;
- tests de journal capacity/eviction ;
- UI e2e.

## Alternate Start

Matrice par phase : nouvelle partie, save/load, disconnect/reconnect, late join, player absent, class conflict, scene completion, departure.

## Preview

- solver avec données synthétiques ;
- resizing ;
- changements rapides ;
- acquire/release ;
- perte de host ;
- edge clipping ;
- conflits de surface.

## Données de test

Les scénarios multi doivent produire : timestamp, player ids, campaign/session ids, versions, logs client/serveur et load order.
