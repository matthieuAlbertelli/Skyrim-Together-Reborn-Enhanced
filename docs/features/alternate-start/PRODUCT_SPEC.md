# Alternate Start — Product Spec

> **Statut : Cible produit acceptée / bootstrap de personnage partiellement implémenté**

## Pitch

Commencer une campagne dans une auberge, sans importer un personnage déjà avancé. Chaque joueur crée son apparence, choisit sa classe et ses kits, puis reçoit un build propre et cohérent avec la campagne.

## Implémenté aujourd’hui

1. Téléportation vers un siège de l’auberge via la quête CK.
2. Ouverture de RaceMenu.
3. Sélection Warrior, Mage ou Thief dans l’UI Angular.
4. Sélection des kits disponibles.
5. Preview 3D et écran de récapitulatif.
6. Soumission finale « Sceller la destinée ».
7. Nettoyage anti-import, niveau 1, inventaire et sorts canoniques.
8. Chemin local hors ligne ou validation serveur en multijoueur.
9. Buffs ciblés Altération fonctionnels sur un compagnon distant.

## Objectifs produit complets

- sauter Helgen proprement ;
- empêcher l’arrivée de personnages externes non validés ;
- créer les personnages au même moment narratif ;
- fonctionner en solo sans serveur STRE ;
- offrir un hub lisible à un petit groupe ;
- former la première intégration first-party de référence ;
- restaurer l’état après reconnexion ;
- quitter l’auberge avec une progression vanilla cohérente.

## Expérience cible

1. Nouvelle partie redirigée vers l’auberge.
2. Création d’apparence à la table.
3. Classe et kits.
4. En STRE, création/jonction de campagne et binding du personnage.
5. Validation de tous les builds.
6. Introduction de Valen.
7. Ready check.
8. Départ collectif vers Skyrim.

Les étapes 2 à 5 sont partiellement présentes ; les étapes 1, 6, 7 et 8 restent à construire complètement.

## Solo

- même création dans l’auberge ;
- catalogue local identique ;
- pas de roster ni ready check obligatoire ;
- aucune dépendance obligatoire au serveur ;
- état conservé dans la sauvegarde Skyrim.

## STRE actuel

- validation serveur de race/classe/sélections ;
- inventaire et sorts canoniques ;
- hashes et accusé d’application ;
- diffusion Pending/Applied ;
- build non persistant au-delà de la session.

## STRE cible

- campagne canonique ;
- roster et personnages liés ;
- phases partagées ;
- Dragonborn assigné secrètement ;
- départ autorisé par le serveur ;
- snapshot et restauration après reconnexion.

## Hors périmètre immédiat

- attribution dynamique du Dragonborn selon les exploits ;
- late join après départ ;
- import volontaire d’un personnage existant ;
- réécriture complète de la campagne vanilla ;
- SDK tiers stable avant plusieurs intégrations first-party.
