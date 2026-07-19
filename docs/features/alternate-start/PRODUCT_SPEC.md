# Alternate Start — Product Spec

> **Statut : Décision produit / non présent dans l’archive STRE**

## Pitch

Commencer une nouvelle campagne directement en groupe dans une auberge, sans séquence de Helgen. Tous les joueurs créent un nouveau personnage lié à cette campagne, rencontrent Valen, choisissent leur classe et partent ensemble dans Skyrim.

## Objectifs

- sauter Helgen proprement ;
- empêcher l’arrivée de personnages externes non validés ;
- créer les personnages au même moment narratif ;
- offrir un hub lisible jusqu’à 10 joueurs ;
- fonctionner aussi en solo sans STRE ;
- servir de première implémentation du Mod Integration Framework.

## Expérience

1. Nouvelle partie.
2. Le joueur arrive à la table de l’auberge.
3. Il crée son personnage.
4. En STRE, le Session Manager crée/rejoint une campagne et invite le groupe.
5. Chaque joueur est placé sur un marqueur d’arrivée.
6. Valen révèle qu’il a convoqué plusieurs inconnus.
7. Il expose sa théorie : l’un d’eux pourrait être le Dovahkiin.
8. Les joueurs choisissent leur classe et reçoivent leur équipement.
9. Ready check.
10. Départ collectif vers Skyrim.

## Solo

- même auberge et même introduction ;
- pas de roster ni ready check obligatoire ;
- choix de classe local ;
- Dragonborn géré selon la logique solo choisie ;
- aucune dépendance binaire obligatoire à STRE.

## STRE

- campagne canonique ;
- roster et personnages liés ;
- phases partagées ;
- Dragonborn assigné secrètement ;
- départ autorisé par le serveur ;
- snapshot après reconnexion.

## Hors périmètre MVP

- attribution dynamique du Dragonborn selon les exploits ;
- late join après départ ;
- import d’un personnage existant ;
- grande campagne entièrement réécrite ;
- classes infiniment configurables.
