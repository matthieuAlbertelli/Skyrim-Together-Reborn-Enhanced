# Runbook de test multijoueur

> **Statut : Proposition**

## Préparation

- mêmes builds et checksums ;
- même load order ;
- logs vidés/archivés ;
- horloges système proches ;
- identifiants joueurs notés ;
- scénario et résultat attendu partagés.

## Pendant le test

Marquer verbalement ou dans un fichier : T0 connexion, T1 action, T2 anomalie, T3 fin. Conserver campaign/session/version ids.

## Collecte

- logs client de chaque joueur ;
- log serveur ;
- vidéo/capture si UI ou rendu ;
- save concernée si possible ;
- liste des mods ;
- étapes exactes ;
- résultat reproductible ou non.

## Scénarios de panne

- tuer un client ;
- couper le réseau ;
- retarder un message via harness ;
- reconnecter ;
- redémarrer serveur lorsque supporté ;
- envoyer un doublon ;
- charger une version d’adapter incompatible.
