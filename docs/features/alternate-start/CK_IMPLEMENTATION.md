# Alternate Start — Implémentation Creation Kit

> **Statut : Bootstrap et records M7 implémentés ; introduction/skip Helgen à poursuivre**

## Fichiers versionnés

```text
GameFiles/Skyrim/STRE_AlternateStart.esp
GameFiles/Skyrim/Source/Scripts/QF_STRE_QUEST_AlternateStart_02001AF9.psc
GameFiles/Skyrim/Scripts/QF_STRE_QUEST_AlternateStart_02001AF9.pex
GameFiles/STRE_AlternateStart.manifest.txt
```

Les PSC seuls ne sont pas exécutés par Skyrim : le PEX compilé doit être récupéré et déployé.

## Records principaux confirmés

- `STRE_CELL_AlternateStart`
- `STRE_CELL_DevSandbox`
- `STRE_QUEST_AlternateStart`
- `STRE_FURN_PlayerSeat01`
- `STRE_FURN_PlayerSeat02`

Aliases utilisés :

- `Alias_Player`
- `Alias_PlayerSeat01`

Étapes de quête :

- `0` — initialisation ;
- `10` — déplacement/assise du joueur ;
- `20` — déclenchement Character Creation.

La quête est volontairement exclue de la synchronisation générique des quêtes.

## Flux actuel

```text
setstage 10
→ MoveTo vers le siège via alias
→ attente de l’état assis
→ passage au stage 20
→ TESQuestStageEvent reçu par CharacterCreationService
→ RaceMenu puis UI Angular
```

Ne jamais coder en dur un FormID chargé dépendant du load order. Les références CK utilisent aliases/propriétés ; le catalogue natif utilise nom de plugin + FormID local.

## Records M7

Le manifest `CK_RECORDS_M7_IMPLEMENTED.json` couvre 47 records attendus :

- cellules, quête et références de sièges ;
- tenues et bottes ;
- enchantements faibles ;
- sorts de Destruction et Altération ;
- effets magiques ciblables pour les buffs alliés.

Les trois buffs alliés doivent conserver un couple compatible dans le `SPEL` et le `MGEF` :

```text
Casting Type : Fire and Forget
Delivery     : Target Actor
```

`Contact` ne convient pas à ces sorts lancés à la main.

## Navmesh

La cellule contient plusieurs fragments de navmesh. Éviter de dépendre d’un pathfinding PNJ complexe tant que la cellule n’a pas reçu un audit CK complet. Toute modification de mobilier ou porte doit être suivie d’un test de circulation.

## Restant à implémenter

- interception propre du nouveau jeu ;
- skip Helgen et états vanilla associés ;
- Valen, scènes, dialogues et aliases ;
- porte de sortie et reprise de la quête principale ;
- marqueurs/placements pour davantage de joueurs ;
- scripts de campagne et bridge générique ;
- compilation Papyrus automatisée.

## Audits

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

Les rapports `_audit/*.tsv` et logs sont générés localement et ne doivent pas être commités.

## Test local

```text
resetquest STRE_QUEST_AlternateStart
startquest STRE_QUEST_AlternateStart
setstage STRE_QUEST_AlternateStart 10
```
