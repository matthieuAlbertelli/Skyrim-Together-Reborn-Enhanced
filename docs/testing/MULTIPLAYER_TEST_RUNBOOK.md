# Runbook de test multijoueur

> **Statut : Procédure active pour les smoke tests à deux PC ; automatisation future**

## Préparation

- mêmes commits client/serveur ;
- même `BuildVersion` ;
- même `STRE_AlternateStart.esp` vérifié par SHA-256 ;
- même load order et masters ;
- serveur redémarré ;
- logs vidés/archivés ;
- horloges système proches ;
- identifiants joueurs notés ;
- scénario et résultat attendu partagés.

## Test Character Build

Sur chaque PC :

```text
resetquest STRE_QUEST_AlternateStart
startquest STRE_QUEST_AlternateStart
setstage STRE_QUEST_AlternateStart 10
```

Choisir des builds différents et noter : classe, options Destruction/Altération, revision, inventoryHash et spellHash.

Résultat attendu :

```text
Build accepted
Canonical spells applied
Applied acknowledgement sent
Build applied
```

Aucune ligne :

```text
RejectedInventoryHash
RejectedSpellHash
spell resolution failed
build rejected
```

## Test des buffs ciblés

- conserver `Fire and Forget` + `Target Actor` dans SPEL/MGEF ;
- viser l’autre joueur à courte portée ;
- lancer Égide, Souffle aquatique ou Allègement ;
- comparer `DamageResist`, état WaterBreathing ou `CarryWeight` avant/après ;
- vérifier l’expiration ;
- collecter les événements MagicService/add target.

## Pendant le test

Marquer : T0 connexion, T1 création, T2 scellement, T3 buff, T4 anomalie, T5 fin. Conserver player IDs, server IDs, revisions et timestamps.

## Collecte

- `tp_client.log` de chaque joueur ;
- log serveur ;
- vidéo/capture si UI ou rendu ;
- save concernée ;
- liste des mods/load order ;
- SHA-256 de l’ESP ;
- étapes exactes ;
- résultat reproductible ou non.

## Filtre PowerShell utile

```powershell
Select-String `
  -Path $log.FullName `
  -Pattern "CharacterBuild|CharacterCreation|MagicService|Spell grant|spellHash|inventoryHash|rejected|failed" |
Select-Object -Last 400 |
ForEach-Object { $_.Line }
```

## Scénarios de panne futurs

- tuer un client pendant Pending ;
- couper le réseau avant l’accusé ;
- reconnecter ;
- redémarrer serveur ;
- mismatch de BuildVersion ;
- ESP différent ;
- envoyer un doublon ;
- plugin/master absent.

Les scénarios de reconnexion/restauration ne peuvent être déclarés réussis tant que la persistance des builds n’est pas implémentée.
