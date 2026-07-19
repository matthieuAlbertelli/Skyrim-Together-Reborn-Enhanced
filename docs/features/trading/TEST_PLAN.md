# Trading — Test Plan

> **Statut : Tests unitaires existants / intégration à compléter**

## Couverture existante

44 cas de test couvrent : automate, canonicalisation, révisions, validations inventory, mutation planning, application, réconciliation et sérialisation.

## Priorité suivante

- harness client/serveur complet ;
- paquets dupliqués/réordonnés ;
- timeout apply/reconcile ;
- disconnect initiator/recipient à chaque état ;
- commit serveur en échec ;
- journal client retransmis après plusieurs opérations ;
- test de charge d’updates d’offre ;
- UI Playwright en mode browser ;
- test in-game des types d’objets exclus.
