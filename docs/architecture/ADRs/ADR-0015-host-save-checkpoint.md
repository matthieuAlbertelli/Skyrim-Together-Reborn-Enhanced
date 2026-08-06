# ADR-0015 — Sauvegarde Skyrim de l’hôte comme checkpoint canonique

- **Statut : Accepted**
- **Date : 2026-07-30**

## Contexte

Le monde doit suivre les règles de persistance, reset et suppression du jeu solo tout en conservant les mutations multijoueurs validées.

## Décision

Le serveur STRE est autoritaire pendant la session. La sauvegarde `.ess` de l’hôte constitue le checkpoint externe canonique. Un journal STRE conserve les mutations non encore intégrées ou concernant des cellules non matérialisées chez l’hôte.

## Conséquences

- save/load doivent être coordonnés avec des checkpoints STRE ;
- les règles vanilla de l’hôte pilotent les suppressions ;
- un petit stockage STRE reste nécessaire pour la reprise après crash ;
- les sauvegardes invitées n’ont aucune autorité sur l’état partagé.
