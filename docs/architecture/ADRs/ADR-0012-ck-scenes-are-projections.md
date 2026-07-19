# ADR-0012 — Les scènes CK sont des projections de l’état canonique

- **Statut : Accepted**
- **Date : 2026-07-19**

## Contexte

Une scène peut être interrompue, non visible ou manquée par un client.

## Décision

La phase de campagne reste dans STRE. La scène CK rend localement cette phase et signale sa complétion ; elle n’est jamais l’unique source de vérité.

## Conséquences

- reprise après reconnexion ;
- nécessité de scènes idempotentes et skip/resume ;
- coordination explicite entre phase et stage local.
