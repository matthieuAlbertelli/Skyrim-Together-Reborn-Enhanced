# Alternate Start — Design solo

> **Statut : Spécification proposée**

## Principe

Le plugin CK est autonome. STRE est détecté comme une extension facultative. Le chemin solo ne doit appeler aucune API réseau obligatoire.

## Bootstrap

- intercepter le démarrage avant la séquence du convoi ;
- placer le joueur dans `STRE_CELL_AlternateStart` ;
- déclencher la création de personnage à la table ;
- initialiser la quête d’introduction ;
- neutraliser ou avancer proprement les états vanilla liés à Helgen ;
- conserver une route vers la quête principale.

## État local suggéré

- `STRE_QUEST_AlternateStart`
- `STRE_GLOBAL_AlternateStartPhase`
- alias joueur ;
- alias Valen ;
- classe choisie ;
- introduction terminée ;
- départ effectué.

## Classes

Chaque classe définit :

- équipement de départ ;
- statistiques ou compétences de départ ;
- don unique ;
- description ;
- éventuelle variante solo du don coopératif.

Pour respecter la vision, le contenu principal du don reste conçu pour la coopération. En solo, une variante minimale ou un effet inactif clairement expliqué est préférable à une refonte divergente.

Les paquetages de départ associés aux compétences sont définis dans [`SKILL_LOADOUTS.md`](SKILL_LOADOUTS.md).
## Sortie

La porte ne s’active qu’après validation locale des étapes. Elle mène vers un emplacement extérieur STRE sûr. La reprise vanilla doit être testée sur :

- quêtes principales ;
- dragons ;
- cris ;
- progression vers Blanche-Rive ;
- civil war/quests sensibles au passage de Helgen.

## Sauvegarde

Tous les états solo restent dans la sauvegarde Skyrim. Aucun blocage ne doit apparaître si STRE est installé puis absent lors d’un chargement ultérieur.
