# ADR-0016 — État courant, journal et outbox transactionnelle

- **Statut : Accepted**
- **Date : 2026-07-30**

## Contexte

Un Event Sourcing intégral augmenterait fortement la complexité. Une écriture d’état suivie d’un envoi réseau non atomique créerait toutefois des fenêtres de perte ou de duplication.

## Décision

Le stockage futur combine :

- état courant normalisé ;
- journal append-only des mutations validées ;
- outbox de réplication écrite dans la même transaction.

Les commandes sont idempotentes et contrôlées par révision optimiste.

## Conséquences

- lecture directe de l’état courant ;
- reprise après crash sans reconstruire tout le monde ;
- retransmission réseau sûre ;
- nécessité de migrations de schéma et de tests d’injection de fautes.
