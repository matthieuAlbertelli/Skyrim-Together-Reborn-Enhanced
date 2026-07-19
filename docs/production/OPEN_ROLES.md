# Missions ouvertes

> **Statut : Modèle d’appel à contribution**

Chaque mission doit être publiée avec un owner, un canal de contact, une licence de contribution et un jalon cible.

## Ingénieur·e STRE — Mod Integration Framework

**Mission :** concevoir et implémenter le runtime qui permet à un mod de décrire sa sémantique multijoueur.

**Travail attendu :**

- interfaces d’adaptateur ;
- registry et cycle de vie ;
- intents, commandes, événements et snapshots ;
- politiques d’autorité ;
- bridge CK/Papyrus ;
- premier adaptateur Alternate Start ;
- tests de reconnexion, doublons et versionnement.

**Compétences :** C++20, architecture distribuée, protocoles, EnTT, sérialisation, idéalement SKSE/Creation Kit.

**Livrable d’essai :** capability `PlayerReady` synchronisée entre deux clients avec snapshot après reconnexion.

## Ingénieur·e C++ — Preview Platform

**Mission :** transformer la preview 3D actuelle en ressource générique stable.

**Point de départ :** `ItemPreviewController`, `ItemPreviewNativeSession`, `ItemPreviewHostBridge`, `ItemPreviewFitSolver` et `ItemPreviewRasterMeasurer` existent déjà.

**Travail attendu :** leases multi-consommateurs, API typée, arbitrage de surface, tests du solver, exemple non lié au trading.

## Moddeur·e Creation Kit — Alternate Start

**Mission :** produire un plugin solo robuste qui saute Helgen et sert de référence d’intégration STRE.

**Travail attendu :** cellule auberge, navmesh, quêtes, aliases, scènes, table de création, classes, sortie vers Skyrim, logs Papyrus, propriétés configurables.

**Livrable d’essai :** nouvelle partie → création du personnage dans l’auberge → première ligne de Valen → progression de quête.

## Narrative Designer — Valen et introduction

**Mission :** écrire une introduction compatible avec 2 à 10 joueurs et l’ambiguïté sur le Dragonborn.

**Travail attendu :** bible narrative, structure de scène, lignes, variantes, réactions aux absences/retards, résumé pour late joiners.

## Comédien ou comédienne — Voix de Valen

**Profil :** voix posée, érudite, persuasive, capable d’une légère gêne sociale et d’une conviction presque obsessionnelle sans caricature.

**Volume initial :** prototype de 20 à 40 lignes, puis lot principal après validation narrative.

**Contraintes :** enregistrement propre, fichiers mono WAV, prises séparées, disponibilité pour retakes, autorisation explicite d’intégration et de redistribution dans le mod.

## Character Artist — Valen

**Mission :** créer un personnage distinct mais cohérent avec Skyrim.

**Travail attendu :** concept, visage, cheveux/barbe, tenue et accessoires, textures, LOD/optimisation si nécessaire, intégration CK et captures en jeu.

**Critère principal :** Valen doit être identifiable en silhouette et en gros plan sans sembler importé d’un autre univers visuel.

## UI/UX Designer

**Mission :** uniformiser trading, lobby de campagne, classes et ready check.

**Travail attendu :** user flows, maquettes, états d’erreur, navigation manette, résolutions 16:9/21:9, guidelines de surfaces CEF.

## QA multijoueur

**Mission :** construire et exécuter des scénarios reproductibles à 2, 4 et 10 joueurs.

**Travail attendu :** matrice de versions, procédures de collecte de logs, tests de déconnexion/reconnexion, rapports avec timeline et identifiants de session.
