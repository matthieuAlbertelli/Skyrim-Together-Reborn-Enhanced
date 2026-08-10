# Carte des équipes

> **Statut : proposition organisationnelle**

| Équipe | Mission | Livrables principaux | Interfaces critiques |
|---|---|---|---|
| Direction produit | Vision, priorités, arbitrages | Vision, roadmap, critères d’acceptation | Toutes les équipes |
| STRE Core | Runtime client/serveur, réseau, World Sync, persistance | Services, protocoles, snapshots, logs | QA, Mod Integration |
| Mod Integration | Contrats/runtime d’adaptation | Adapter API, registry, bridge CK | STRE Core, moddeurs |
| Alternate Start CK | Contenu jouable solo et intégration Skyrim | ESP, quêtes, scènes, Papyrus, cellules | Narrative, Mod Integration |
| Narrative | Cohérence, scripts, personnages | Bible narrative, dialogues | CK, Audio, Art |
| Character Art | Personnages | concepts, modèles, textures | Narrative, CK |
| Environment Art | Espaces/props | dressing, lumière, optimisation | CK, QA |
| Audio | Casting, voix, traitement | masters, lip files, nomenclature | Narrative, CK |
| UI/UX | Interfaces coopératives | flux, Angular/CEF, accessibilité | STRE Client, QA |
| QA & Compatibility | Validation solo/réseau/load order | plans, matrice, rapports | Toutes les équipes |
| Build & Release | CI, packages, versions | artefacts, changelog, releases | STRE Core, CK |
| Community & Docs | Onboarding et communication | README, guides, contribution | Direction produit |

## Cellules transverses

### World Sync

STRE Core + QA. Les intégrations SKSE externes restent des dépendances, pas des sous-modules du cœur.

### Trading

STRE Core + UI/UX + QA.

### Alternate Start

Alternate Start CK + Mod Integration + STRE Core + Narrative + Art + Audio + QA.

### Preview Platform

STRE Client + UI/UX + Mod Integration.

## Responsabilité minimale

Chaque domaine actif doit avoir :

- un owner fonctionnel;
- un responsable technique/créatif;
- un reviewer secondaire;
- une documentation canonique clairement localisée.
