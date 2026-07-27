# Alternate Start

> **Statut : Vertical slice Character Build implémenté et smoke-testé ; campagne complète en cours**

## Résultat actuel

Le joueur peut être placé dans l’auberge, ouvrir RaceMenu, choisir une classe et des kits dans l’interface Angular, consulter le résumé puis sceller son build.

Le flux fonctionne :

- localement sans serveur ;
- en multijoueur avec validation serveur de l’inventaire et des sorts ;
- avec trois classes exposées : Warrior, Mage et Thief ;
- avec les sorts de Destruction et d’Altération du Mage ;
- avec des buffs ciblés fonctionnels sur un autre joueur.

Le catalogue courant est `BuildVersion = 5`.

## Ce qui n’est pas encore un Alternate Start complet

- interception automatique du nouveau jeu et skip Helgen exhaustif ;
- Valen et l’introduction narrative ;
- sortie collective vers Skyrim ;
- roster, ready check, Campaign State et Dragonborn secret ;
- persistance/reconnexion durable des builds ;
- écoles de magie et kits de compétences restants.

## Sources de vérité

Ordre de priorité pour le comportement implémenté :

1. `Code/common/CharacterCreation/CharacterBuildCatalog.*` — règles canoniques ;
2. `Code/skyrim_ui/src/app/data/character-loadouts.ts` — présentation et choix UI ;
3. `CK_RECORDS_M7_IMPLEMENTED.json` et `STRE_AlternateStart.esp` — records CK ;
4. `M7_CK_CODE_INTEGRATION.md` — architecture et validation du jalon ;
5. `KITS_EQUIPEMENT_PAR_COMPETENCE_V2.xlsx` — conception fonctionnelle à poursuivre ;
6. `SKILL_LOADOUTS_fr.md` — document historique V0.1, non canonique pour les valeurs implémentées.

## Documents

- `PRODUCT_SPEC.md` — cible produit et distinction actuel/futur ;
- `SOLO_DESIGN.md` — fonctionnement sans serveur ;
- `STRE_ADAPTER_SPEC.md` — sémantique coopérative actuelle et cible ;
- `CK_IMPLEMENTATION.md` — records et flux Creation Kit ;
- `STATE_MODEL.md` — build courant et futur état de campagne ;
- `TEST_PLAN.md` — validation automatisée, solo et multijoueur ;
- `OPEN_QUESTIONS.md` — décisions restantes ;
- `M7_CK_CODE_INTEGRATION.md` — intégration records/catalogue/protocole ;
- `CK_RECORDS_M7_IMPLEMENTED.json` — manifest strict des records et FormIDs locaux ;
- `KITS_EQUIPEMENT_PAR_COMPETENCE_V2.xlsx` — matrice de conception des kits ;
- `SKILL_LOADOUTS_fr.md` — archive de conception V0.1.

## Commandes de test locales

```text
resetquest STRE_QUEST_AlternateStart
startquest STRE_QUEST_AlternateStart
setstage STRE_QUEST_AlternateStart 10
```

## Dépendances actuelles

- `Skyrim.esm`, `Update.esm` et `Dragonborn.esm` pour les records CK utilisés ;
- client STRE pour l’UI native/Angular et le chemin multijoueur ;
- serveur STRE uniquement pour le chemin autoritaire ;
- aucun serveur requis pour le fallback local.

## Prochain jalon

Étendre et fiabiliser les kits restants, puis traiter persistance/reconnexion avant de construire le Campaign State complet.
