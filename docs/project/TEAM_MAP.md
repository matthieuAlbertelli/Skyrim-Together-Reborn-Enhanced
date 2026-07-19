# Carte des équipes

> **Statut : Proposition organisationnelle**

| Équipe | Mission | Livrables principaux | Interfaces critiques |
|---|---|---|---|
| Direction produit | Vision, priorités, arbitrages | Vision, roadmap, critères d’acceptation | Toutes les équipes |
| STRE Core | Runtime client/serveur, réseau, persistance | Services, protocole, snapshots, logs | Integration Framework, QA |
| Mod Integration | SDK, contrats et runtime d’adaptation | Mod Adapter API, registry, bridge CK | STRE Core, moddeurs |
| Alternate Start CK | Contenu jouable solo et intégration Skyrim | ESP, quêtes, scènes, Papyrus, cellules | Narrative, Mod Integration |
| Narrative | Cohérence, scripts, personnages | Bible narrative, dialogues, direction de jeu | CK, Audio, Art |
| Character Art | Valen et assets personnages | concept, modèle, textures, intégration | Narrative, CK |
| Environment Art | Auberge, habillage et lisibilité | dressing, props, lumière, optimisation | CK, QA |
| Audio | Casting, voix, traitement et intégration | WAV, lip files, mix, nomenclature | Narrative, CK |
| UI/UX | Interfaces coopératives | flux, maquettes, Angular/CEF, accessibilité | STRE Client, QA |
| QA & Compatibility | Validation solo/réseau/load order | plans de test, matrice, rapports | Toutes les équipes |
| Build & Release | CI, packages, versions, distribution | artefacts, changelog, release notes | STRE Core, CK |
| Community & Docs | Onboarding et communication | README, guides, appels à contribution | Direction produit |

## Cellules de travail transverses

### Vertical Slice Trading

STRE Core + UI/UX + QA. Sert de référence pour les automates autoritaires, l’idempotence et la réconciliation.

### Vertical Slice Alternate Start

Alternate Start CK + Mod Integration + STRE Core + Narrative + Art + Audio + QA. Sert de référence pour l’adaptation d’un mod solo.

### Preview Platform

STRE Client + UI/UX + Mod Integration. Transforme la preview actuelle en ressource partagée, arbitrée et documentée.

## Responsables attendus

Chaque équipe doit avoir :

- un **Owner** qui priorise et accepte les livrables ;
- un **Technical/Creative Lead** qui définit les standards ;
- au moins un **Reviewer secondaire** pour réduire le bus factor.
