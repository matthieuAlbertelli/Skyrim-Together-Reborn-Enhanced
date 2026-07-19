# Stratégie upstream

> **Statut : Proposition**

## Constat

Le dépôt est un fork de `tiltedphoques/TiltedEvolution`. `UPSTREAM.md` contient encore un placeholder alors que l’historique montre une branche custom au-dessus de l’upstream `dev`.

## Règles

- enregistrer le commit base exact à chaque release ;
- merge/rebase upstream à cadence régulière ;
- isoler les modifications STRE dans des services et dossiers dédiés ;
- éviter les changements de style massifs sur fichiers upstream ;
- maintenir un registre des conflits récurrents ;
- ajouter tests de non-régression avant chaque intégration.

## Classification des patches

- `isolated` : nouveau fichier/service ;
- `factory-registration` : opcode ou service registry ;
- `hook-sensitive` : reverse engineering/menu/native ;
- `ui-invasive` : composants Angular partagés ;
- `protocol-breaking` : factories/opcodes ;
- `build/release` : CI et packaging.

Les patches `hook-sensitive` et `protocol-breaking` exigent une revue dédiée lors d’un update upstream.
