# Missions ouvertes

> **Statut : Modèle d’appel à contribution mis à jour après M7**

Chaque mission doit être publiée avec un owner, un canal de contact, une licence de contribution et un jalon cible.

## Ingénieur·e STRE — Build persistence and Campaign State

**Point de départ :** Character Build v5 valide déjà race, classe, inventaire, sorts et hashes pendant une session.

**Mission :** rendre ce build restaurable puis construire l’état de campagne minimal.

**Travail attendu :**

- stockage versionné ;
- character binding ;
- migration de BuildVersion ;
- snapshot/reconnect idempotent ;
- roster et phase ;
- ready check ;
- tests de redémarrage et mismatch de plugin/catalogue.

**Compétences :** C++20, architecture distribuée, EnTT, sérialisation, persistance et tests réseau.

## Ingénieur·e C++ — Magic capability extensible

**Point de départ :** trois buffs STRE sont reconnus dans `MagicItem::IsBuffSpell()` par plugin + FormID local.

**Mission :** remplacer l’allowlist par une classification robuste compatible avec davantage de sorts coopératifs.

**Travail attendu :** keywords/capabilities, validation des cibles, stacking, friendly fire, logs et tests.

## Ingénieur·e C++ — Preview Platform

**Point de départ :** Trading et Character Creation utilisent déjà la preview 3D.

**Mission :** ajouter leases multi-consommateurs, owner tokens, priorité, arbitrage de surface et tests du solver/lifecycle.

## Moddeur·e Creation Kit — Alternate Start completion

**Point de départ :** cellule, quête, sièges, RaceMenu, tenues et sorts M7 existent.

**Mission :** terminer le vrai départ de campagne.

**Travail attendu :**

- interception du nouveau jeu ;
- skip Helgen et reprise vanilla ;
- Valen, aliases, scènes et dialogues ;
- porte de sortie ;
- navmesh et circulation ;
- logs Papyrus ;
- compilation PSC → PEX.

## Gameplay/CK Designer — Remaining starter kits

**Mission :** matérialiser Invocation, Illusion, Restauration, Enchantement et les kits utilitaires restants sans objets farfelus ni abus économiques.

**Livrables :** records CK, catalogue, UI, preview, tests combinatoires et plan d’équilibrage.

## Narrative Designer — Valen et introduction

**Mission :** écrire une introduction compatible avec 2 à 10 joueurs et l’ambiguïté sur le Dragonborn.

## Comédien ou comédienne — Voix de Valen

**Volume initial :** prototype après verrouillage du script et intégration de la scène.

## Character Artist — Valen

**Mission :** créer un personnage distinct mais cohérent avec Skyrim, avec provenance/licence explicites.

## UI/UX Designer

**Mission :** uniformiser Trading, Character Creation, futur lobby, classes et ready check ; navigation manette et 16:9/21:9.

## QA multijoueur

**Mission :** construire et exécuter des scénarios reproductibles à 2 puis 4 joueurs : build matrix, targeted buffs, déconnexion/reconnexion, logs et comparaison des hashes.
