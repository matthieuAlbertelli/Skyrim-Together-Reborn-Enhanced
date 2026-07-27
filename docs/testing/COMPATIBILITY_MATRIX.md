# Matrice de compatibilité

> **Statut : État connu au 27 juillet 2026 ; compléter à chaque campagne de test**

## Versions plateforme

| Composant | Version supportée / observée | Testée | Notes |
|---|---|---|---|
| Skyrim SE/AE runtime | `1.6.1170` | Oui | runtime principal du développement M7 |
| Address Library | entrée `441582`, fallback `21890` | Oui pour `441582` | utilisée par `Script::CompileAndRun` |
| Skyrim Together Reborn upstream | baseline `ca3f3234` | Partiel | voir `UPSTREAM.md` ; full SHA à enregistrer pour une release |
| STRE | `0.1.0-alpha.1` + changements Unreleased | Oui localement | version de package non incrémentée |
| Creation Kit | environnement compatible 1.6.1170 | Oui | version exacte à consigner |
| Angular | 16.x | Oui | Character Creation et Trading |
| xmake | 3.0.0 ou compatible | Oui localement | build Windows réussi |
| `STRE_AlternateStart.esp` | manifest M7, BuildVersion 5 | Oui | 47 records stricts, dépend de Dragonborn.esm |

## Fonctionnalités

| Configuration | Trading | Preview | Character Build solo | Character Build STRE | Campagne Alternate Start complète |
|---|---:|---:|---:|---:|---:|
| Vanilla + STRE | Alpha | Implémenté | Smoke testé | Smoke testé | Non implémentée |
| SkyUI | À retester | À retester | Utilisé dans l’environnement dev | Utilisé dans l’environnement dev | N/A |
| Anniversary content | À tester | À tester | Test runtime 1.6.1170 | Test runtime 1.6.1170 | N/A |
| 1 joueur hors ligne | N/A | Oui | Oui | N/A | Non |
| 2 joueurs | Cible principale | Oui | N/A | Buffs ciblés validés | Non |
| 4 joueurs | À tester | À tester | N/A | À tester | Cible future |
| 10 joueurs | Non prioritaire | Non prioritaire | N/A | Non testé | Cible hub future |

## Éléments M7 vérifiés

- BuildVersion et catalogue identiques client/serveur ;
- 47 records CK conformes ;
- 41 références code/ESP conformes ;
- inventaire et spell hashes acceptés ;
- ciblage `Target Actor` des trois buffs alliés ;
- même ESP déployé sur les deux PC.

## Fiche de test d’un mod

Pour chaque mod : nom, version, load order, adapter, résultat solo, résultat STRE, conflit, workaround, logs et date.
