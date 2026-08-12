# Alternate Start

> **Statut : Vertical slice Character Build implémenté et smoke-testé ; campagne complète en cours**

## Résultat actuel

Le joueur peut être placé dans l’auberge, ouvrir RaceMenu, choisir une classe et des kits dans l’interface Angular, consulter le résumé puis sceller son build.

Le flux fonctionne :

- localement sans serveur;
- en multijoueur avec validation serveur de l’inventaire et des sorts;
- avec Warrior, Mage et Thief;
- avec le vertical slice Destruction/Altération du Mage;
- avec des buffs ciblés fonctionnels sur un autre joueur.

Le catalogue courant est `BuildVersion = 5`.

Pour l’état courant du projet et les limites globales, voir [`docs/project/STATUS.md`](../../project/STATUS.md).

## Sources de vérité de la feature

Ordre de priorité pour le comportement implémenté :

1. `Code/common/CharacterCreation/CharacterBuildCatalog.*` — règles canoniques;
2. `Code/skyrim_ui/src/app/data/character-loadouts.ts` — présentation/choix UI;
3. `CK_RECORDS_M7_IMPLEMENTED.json` et `STRE_AlternateStart.esp` — records CK;
4. `CK_IMPLEMENTATION.md` — comportement CK maintenu;
5. `CLASS_ROSTER_V1.md` — roster produit canonique des 21 classes v1 et affectations majeures/mineures;
6. `PRODUCT_SPEC.md` / `STATE_MODEL.md` — cible fonctionnelle;
7. `KITS_EQUIPEMENT_PAR_COMPETENCE_V2.xlsx` — matrice de conception à poursuivre.

Les documents sous `history/` sont des preuves/jalons datés et ne remplacent pas ces sources.

## Documents courants

- `PRODUCT_SPEC.md` — cible produit;
- `CLASS_ROSTER_V1.md` — roster canonique des 21 classes v1;
- `SOLO_DESIGN.md` — fonctionnement sans serveur;
- `STRE_ADAPTER_SPEC.md` — sémantique coopérative actuelle/cible;
- `CK_IMPLEMENTATION.md` — records et flux CK;
- `STATE_MODEL.md` — état build/campagne;
- `TEST_PLAN.md` — validation;
- `OPEN_QUESTIONS.md` — décisions restantes;
- `CK_RECORDS_M7_IMPLEMENTED.json` — manifest strict;
- `KITS_EQUIPEMENT_PAR_COMPETENCE_V2.xlsx` — conception des kits.

## Historique

- `history/M7_CK_CODE_INTEGRATION_20260727.md` — validation du jalon M7;
- `history/SKILL_LOADOUTS_v0.1_fr.md` — ancienne conception V0.1.

## Commandes de test locales

```text
resetquest STRE_QUEST_AlternateStart
startquest STRE_QUEST_AlternateStart
setstage STRE_QUEST_AlternateStart 10
```

## Dépendances

- `Skyrim.esm`, `Update.esm`, `Dragonborn.esm` selon les records;
- client STRE pour l’UI native/Angular et le chemin multijoueur;
- serveur STRE pour le chemin autoritaire;
- aucun serveur requis pour le fallback local.
