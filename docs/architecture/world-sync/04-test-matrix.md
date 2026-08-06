# World Sync — Matrice de test

## Environnement de référence

```text
PC hôte
  serveur STRE
  client Skyrim/STRE
  sauvegarde canonique

PC distant
  client Skyrim/STRE
```

Déploiement : script externe `build-and-deploy-dev.ps1`, puis déploiement Vortex sur les deux ordinateurs.

## Scénario J0-A — Drop hôte vers distant

Préconditions :

- même cellule intérieure ;
- drops expérimentaux activés sur le serveur ;
- une épée de fer non modifiée, quantité 1.

Étapes :

1. l’hôte lâche l’épée ;
2. le joueur distant observe le drop ;
3. collecter les logs des trois processus.

Attendu :

```text
client hôte : request_send
serveur     : request_receive puis notify_broadcast
client dist.: notify_apply
```

## Scénario J0-B — Drop distant vers hôte

Même scénario en inversant les joueurs.

## Filtres de logs

```powershell
Select-String -Path <log> -Pattern "\[STRE\]\[WorldSync\]"
```

## Données à conserver avec chaque exécution

- SHA Git de la branche ;
- hashes des binaires déployés ;
- configuration serveur ;
- client hôte ;
- serveur ;
- client distant ;
- sens du test ;
- heure locale précise ;
- résultat observé en jeu.

## Garde-fous

Le jalon 0 ne valide pas encore :

- identité commune ;
- pickup autoritaire ;
- conservation après sortie de cellule ;
- sauvegarde/chargement ;
- ExtraData.
