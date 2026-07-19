# Trading — Product Spec

> **Statut : Fonctionnalité alpha implémentée, spécification consolidée**

## Problème

Skyrim Together Reborn ne fournit pas un flux d’échange joueur-à-joueur sécurisé et compréhensible. Déposer des objets au sol est peu fiable, peu immersif et ne donne aucune garantie de consentement mutuel.

## Objectif utilisateur

Deux joueurs proches peuvent ouvrir une transaction, composer leurs offres, inspecter les objets, confirmer la même révision puis recevoir un résultat clair : terminé, annulé ou échoué.

## Flux principal

1. Le joueur A active le joueur B et demande un échange.
2. B accepte ou refuse dans les 30 secondes.
3. Les deux joueurs ajoutent ou retirent des quantités.
4. Toute modification annule les confirmations précédentes.
5. Les deux joueurs confirment la même révision.
6. Le serveur valide les inventaires et verrouille la transaction.
7. Chaque client applique son plan local.
8. Le serveur confirme ou déclenche une réconciliation.
9. L’UI affiche l’état terminal.

## Règles alpha actuelles

Un objet n’est transférable que s’il :

- existe en quantité positive ;
- n’est pas un objet de quête ;
- n’est pas équipé ;
- ne porte pas de charge, enchantement custom, poison, âme ou santé/durabilité extra ;
- n’est pas ambigu dans l’inventaire logique ;
- peut être représenté par le couple canonique mod/base id.

Les offres et plans sont limités à 64 lignes.

## États UX

- invitation reçue ;
- invitation sortante ;
- synchronisation ;
- négociation ;
- verrouillé ;
- application ;
- terminé ;
- annulé ;
- échec.

## Non-objectifs de l’alpha

- échange d’or ;
- split avancé de stacks avec instances distinctes ;
- objets enchantés uniques ;
- persistance après restart serveur ;
- transactions multi-joueurs ;
- marketplace.

## Critères d’acceptation

- impossible de confirmer une offre périmée ;
- impossible d’échanger avec soi-même ou avec un joueur occupé ;
- retransmettre une commande identique ne double pas les objets ;
- un échec partiel mène à une réconciliation ou à un état explicite ;
- fermer/revenir sur l’UI ne modifie pas l’état serveur ;
- l’aperçu 3D ne bloque pas l’input après fermeture.
