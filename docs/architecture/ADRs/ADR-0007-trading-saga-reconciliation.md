# ADR-0007 — Le trading est une saga compensée

- **Statut : Implemented**
- **Date : 2026-07-19**

## Contexte

Deux inventaires Skyrim locaux et un état serveur ne peuvent pas être modifiés par une transaction ACID unique.

## Décision

Le serveur valide un plan, les clients appliquent leur part et journalisent le résultat, puis le serveur commit. En cas d’incertitude, une réconciliation par quantités absolues restaure la convergence.

## Conséquences

- robustesse aux retransmissions ;
- complexité de protocoles `ApplyId/ReconcileId` ;
- l’atomicité doit être communiquée comme logique, non ACID ;
- tests de panne indispensables.
