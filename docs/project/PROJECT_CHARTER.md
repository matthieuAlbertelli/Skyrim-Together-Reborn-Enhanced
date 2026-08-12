# Charte du projet

> **Statut : charte active**

## Mission

Développer et maintenir un fork open source de Skyrim Together Reborn qui renforce la coopération immersive, fournit une base de campagne commune et construit des contrats d’adaptation maintenables pour du contenu Skyrim initialement solo.

## Périmètre

### Inclus

- client et serveur STRE;
- protocole réseau et états autoritaires;
- UI coopérative;
- synchronisation d’entités monde STRE;
- plugin Alternate Start;
- bridge CK/Papyrus ↔ STRE;
- contrats d’intégration pour mods;
- contenu narratif et artistique propre au projet;
- outils de test, logs, documentation et packaging.

### Hors périmètre initial

- monde persistant de type MMO;
- compatibilité universelle sans intervention du moddeur;
- réécriture complète de toutes les quêtes vanilla;
- support garanti de toutes les versions Skyrim et de tous les load orders;
- redistribution d’assets propriétaires sans autorisation.

## Gouvernance

### Décisions produit

La direction produit tranche la vision, le périmètre et les priorités après consultation des responsables concernés.

### Décisions d’architecture

Toute décision structurelle durable est consignée dans un ADR. Les détails d’une feature restent dans son répertoire canonique.

### Modifications de protocole

Toute modification significative d’opcode, de schéma sérialisé ou d’état persistant exige :

- bornes explicites;
- tests de round-trip;
- stratégie de compatibilité;
- revue serveur/client;
- mise à jour de la référence protocole de la feature.

## Règles de fonctionnement

- Une issue décrit un résultat observable.
- Une fonctionnalité n’est pas terminée sans tests, logs utiles et documentation.
- Une branche évite de mélanger refactor massif et changement fonctionnel sans justification.
- Toute donnée critique possède une source de vérité et une stratégie de reprise.
- Toute information documentaire mutable possède une seule source canonique.
- Les documents historiques sont archivés comme tels et ne servent pas à annoncer l’état courant.

## Principes d’ingénierie

- KISS : réutiliser les mécanismes fiables du moteur et de STR avant d’inventer une couche.
- DRY : une règle métier, un contrat protocole et un statut mutable ne doivent pas être maintenus en parallèle à plusieurs endroits.
- API-friendly : dépendre de contrats observables/stables plutôt que d’internals tiers lorsque possible.
- Fail closed : refuser une opération lorsque les métadonnées requises ne peuvent pas être préservées correctement.
- Authority explicit : chaque état partagé muté possède une autorité déclarée.
- Engine-safe : les mutations Skyrim déclenchées par le réseau sont exécutées sur un contexte moteur approprié.

## Critères de santé

- build reproductible;
- tests automatisés pertinents;
- upstream base identifiable;
- décisions architecturales enregistrées;
- roadmap liée à des résultats démontrables;
- documentation sans sources de vérité concurrentes;
- démos jouables à chaque jalon significatif.

Pour l’état implémenté/validé, voir [`docs/project/STATUS.md`](STATUS.md). La direction produit et les release gates appartiennent à [`ROADMAP.md`](../../ROADMAP.md); l’avancement opérationnel au GitHub Project défini par [`docs/production/GITHUB_GOVERNANCE.md`](../production/GITHUB_GOVERNANCE.md).
