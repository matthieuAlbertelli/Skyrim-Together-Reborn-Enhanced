# ADR-0004 — Snapshot complet plus événements incrémentaux

- **Statut : Accepted**
- **Date : 2026-07-19**

## Contexte

Un client reconnecté ne peut pas dépendre d’événements qu’il a manqués.

## Décision

Chaque adapter expose un snapshot canonique versionné. Après application du snapshot, le client consomme les événements strictement postérieurs à cette version.

## Conséquences

- reprise déterministe ;
- coût de sérialisation et migration ;
- handlers d’événements idempotents ;
- tests de snapshot obligatoires.
