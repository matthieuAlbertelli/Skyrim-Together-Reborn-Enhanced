# Stratégie de test

> **Statut : Mise à jour avec les tests Character Build M7**

## Pyramide

### Tests purs

- automates de domaine ;
- validation et canonicalisation ;
- catalogues de récompenses ;
- plans de mutation/réconciliation ;
- hashes déterministes ;
- solver de preview ;
- futures transitions Campaign State.

### Tests de sérialisation

- round-trip de tous messages ;
- tailles maximales ;
- payloads tronqués ;
- enums inconnus ;
- versions incompatibles ;
- snapshot Character Build avec inventaire et sorts ;
- accusé avec inventory/spell hashes.

### Tests de services

- validation des requêtes ;
- plugins/FormIDs locaux manquants ;
- build Pending remplacé avant application ;
- build Applied non remplaçable ;
- mismatch de révision/hash ;
- retransmissions ;
- déconnexion et future restauration.

### Tests client/serveur

Deux processus automatisés ou harness :

- ordre et perte de messages ;
- duplicate delivery ;
- version mismatch ;
- catalogues/plugin différents ;
- reconnect ;
- latence.

### Tests en jeu

- Skyrim 1.6.1170 ;
- plugin CK ;
- UI/preview ;
- magie ciblée ;
- sauvegarde/chargement ;
- 1 puis 2 joueurs, ensuite 4/10.

## Trading

Les tests existants couvrent session, application, inventory planning, protocole et réconciliation. Restent : intégration serveur/client, commit failure, disconnect à chaque étape, stress et UI e2e.

## Character Build

`Code/tests/character_build.cpp` couvre :

- validation des classes/options ;
- neuf combinaisons Mage ;
- quantité/unicité des sorts ;
- kit simplifié du Thief et 10 crochets ;
- hash de sorts normalisé ;
- sérialisation du snapshot et de l’accusé.

Scripts statiques :

- `audit_stre_plugin_records.py` ;
- `audit_character_build_catalog.py`.

À ajouter :

- tests de service serveur dédiés aux rejets ;
- harness client/serveur pour les hashes ;
- tests de persistance/reconnexion ;
- test extensible de classification des buffs ;
- matrice en jeu de toutes les classes/options.

## Alternate Start complet

Matrice future : nouveau jeu, skip Helgen, save/load, Valen, disconnect/reconnect, late join, player absent, class conflict, scene completion et departure.

## Preview

- solver avec données synthétiques ;
- resizing ;
- changements rapides ;
- acquire/release ;
- perte de host ;
- edge clipping ;
- conflits de surface ;
- concurrence Trading/Character Creation.

## Données de test

Chaque scénario multi doit produire : timestamp, player IDs, build/campaign/session IDs, revisions, BuildVersion, hashes, logs client/serveur, load order et SHA de l’ESP déployé.
