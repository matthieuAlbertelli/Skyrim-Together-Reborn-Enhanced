# Alternate Start — Test Plan

> **Statut : Audits automatisés et smoke tests M7 exécutés ; couverture exhaustive à poursuivre**

## Contrôles déjà réalisés

- build Windows xmake réussi ;
- audit strict de 47 records CK conforme ;
- audit de 41 références catalogue/ESP conforme ;
- tests `Code/tests/character_build.cpp` compilés ;
- bootstrap Mage testé en jeu ;
- buffs ciblés testés entre deux PC ;
- fallback solo prévu dans le service et testé sur le flux de build.

Ces contrôles ne valent pas validation exhaustive de toutes les combinaisons.

## Audits statiques

```powershell
py -3 .\Tools\Scripts\audit_stre_plugin_records.py `
  .\GameFiles\Skyrim\STRE_AlternateStart.esp `
  --manifest .\docs\features\alternate-start\CK_RECORDS_M7_IMPLEMENTED.json `
  --output .\_audit\STRE_AlternateStart.records.m7.tsv `
  --strict `
  --reject-unexpected
```

```powershell
py -3 .\Tools\Scripts\audit_character_build_catalog.py `
  .\GameFiles\Skyrim\STRE_AlternateStart.esp `
  .\Code\common\CharacterCreation\CharacterBuildCatalog.cpp `
  --client-source .\Code\client\Services\Generic\CharacterCreationService.cpp
```

## Smoke test local

```text
resetquest STRE_QUEST_AlternateStart
startquest STRE_QUEST_AlternateStart
setstage STRE_QUEST_AlternateStart 10
```

Vérifier : RaceMenu, UI, preview, résumé, niveau 1, inventaire exact, équipement, sorts exacts, nettoyage et absence de rejet de hash.

## Matrice Mage

Tester les neuf combinaisons :

- Feu × Protection ;
- Feu × Exploration ;
- Feu × Matière ;
- Froid × Protection ;
- Froid × Exploration ;
- Froid × Matière ;
- Foudre × Protection ;
- Foudre × Exploration ;
- Foudre × Matière.

Chaque build doit produire exactement 7 sorts canoniques : 3 Destruction + 4 Altération.

## Tests multijoueur prioritaires

- mêmes versions client/serveur/plugin sur les deux PC ;
- créations indépendantes avec choix différents ;
- états Accepted puis Applied ;
- aucun `RejectedInventoryHash` ou `RejectedSpellHash` ;
- apparence et équipement distants ;
- Égide minérale : `DamageResist` augmente puis revient ;
- Souffle aquatique : effet actif puis expiration ;
- Allègement : `CarryWeight` augmente puis revient ;
- absence d’application sur mauvaise cible ;
- relance/cumul contrôlé.

## Tests de régression classes

- Warrior : équipement lourd, armes, forge, pendentif ;
- Thief : tenues, armes, 10 crochets ;
- Mage : tenue visuelle, 7 sorts ;
- changement de build avant accusé ;
- second build après état Applied rejeté.

## Tests encore bloqués par les fonctionnalités absentes

- nouveau jeu automatique et skip Helgen ;
- Valen/scène ;
- sortie et reprise vanilla ;
- save/load à chaque phase ;
- reconnexion et restauration du build ;
- ready check, late join et Campaign State ;
- 4 et 10 joueurs.

## Collecte de logs

Conserver pour chaque test : date, runtime, load order, BuildVersion, client/serveur, choix logiques et lignes `CharacterBuild`/`CharacterCreation`/`MagicService`. Les rapports `_audit`, TSV et logs restent locaux et ne sont pas versionnés.
