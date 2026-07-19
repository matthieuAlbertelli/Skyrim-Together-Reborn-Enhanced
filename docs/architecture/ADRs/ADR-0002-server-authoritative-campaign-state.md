# ADR-0002 — État de campagne autoritaire côté serveur

- **Statut : Accepted**
- **Date : 2026-07-19**

## Contexte

Les phases, classes, ready states, bindings et secrets narratifs peuvent diverger si les clients décident indépendamment.

## Décision

Le serveur conserve l’état canonique de campagne. Le Session Manager possède des permissions administratives mais n’est pas la source technique finale.

## Conséquences

- reconnexion et arbitrage cohérents ;
- dépendance au serveur pour les transitions coopératives ;
- nécessité d’une persistance et d’un snapshot ;
- les scripts CK appliquent des conséquences, pas la vérité canonique.
