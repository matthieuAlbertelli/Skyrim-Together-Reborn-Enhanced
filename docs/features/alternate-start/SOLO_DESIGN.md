# Alternate Start — Design solo

> **Statut : Fallback de build local implémenté / bootstrap complet du nouveau jeu proposé**

## Principe

Le plugin CK et Character Creation doivent rester utilisables sans serveur STRE. Le même catalogue de classes, objets et sorts est appliqué localement.

## Implémenté

- quête et cellule Alternate Start ;
- déplacement vers la table ;
- RaceMenu ;
- UI de classe et loadouts ;
- nettoyage du personnage ;
- application locale de l’inventaire, des sorts et de l’équipement ;
- niveau 1 ;
- aucun appel serveur obligatoire quand STRE n’est pas connecté.

## À implémenter pour un vrai nouveau jeu solo

- intercepter le démarrage avant la séquence du convoi ;
- neutraliser ou avancer proprement les états vanilla liés à Helgen ;
- initialiser l’introduction ;
- ouvrir la porte de sortie après validation ;
- garantir la route vers la quête principale.

## État local

Éléments actuels :

- `STRE_QUEST_AlternateStart` ;
- aliases joueur/siège ;
- état local de Character Creation dans le service client.

Éléments futurs possibles :

- global de phase ;
- alias Valen ;
- introduction terminée ;
- départ effectué.

## Classes et paquetages

Le comportement réellement appliqué est défini par :

- `CharacterBuildCatalog.*` ;
- `character-loadouts.ts` ;
- `CK_RECORDS_M7_IMPLEMENTED.json`.

La conception élargie reste dans [`KITS_EQUIPEMENT_PAR_COMPETENCE_V2.xlsx`](KITS_EQUIPEMENT_PAR_COMPETENCE_V2.xlsx). [`SKILL_LOADOUTS_fr.md`](SKILL_LOADOUTS_fr.md) est une archive V0.1 et ne doit pas remplacer le catalogue courant.

## Sortie et reprise vanilla

La porte future ne devra s’activer qu’après validation locale du build. La reprise doit être testée sur :

- quête principale ;
- dragons et cris ;
- progression vers Blanche-Rive ;
- guerre civile et quêtes sensibles au passage de Helgen.

## Sauvegarde

Les états solo doivent rester dans la sauvegarde Skyrim. Aucun blocage ne doit apparaître si STRE est installé puis indisponible lors d’un chargement ultérieur.
