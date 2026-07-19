# ADR-0006 — Aucun FormID plugin codé en dur

- **Statut : Accepted**
- **Date : 2026-07-19**

## Contexte

Les FormIDs dépendent du load order et rendent les intégrations fragiles.

## Décision

Utiliser propriétés Papyrus, références configurées, Editor IDs contrôlés et résolution via mod/base id lorsque nécessaire.

## Conséquences

- installation plus robuste ;
- configuration initiale plus explicite ;
- besoin d’outils de validation des propriétés.
