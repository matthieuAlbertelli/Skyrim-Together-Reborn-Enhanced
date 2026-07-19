# ADR-0011 — Canal CEF dédié aux fonctionnalités STRE

- **Statut : Proposed**
- **Date : 2026-07-19**

## Contexte

Le trading multiplexe actuellement ses actions sur `toggleDebugUI('__trade__', ...)`.

## Décision

Créer un canal natif typé et versionné, par exemple `streCommand(namespace, action, payload)` et des événements correspondants.

## Conséquences

- contrat lisible et extensible ;
- validation centralisée ;
- migration du frontend ;
- compatibilité temporaire à prévoir.
