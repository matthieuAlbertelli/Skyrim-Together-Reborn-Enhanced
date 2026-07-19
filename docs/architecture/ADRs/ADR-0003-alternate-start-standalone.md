# ADR-0003 — Alternate Start reste jouable sans STRE

- **Statut : Accepted**
- **Date : 2026-07-19**

## Contexte

Le plugin sert à sauter Helgen et doit également constituer une intégration de référence.

## Décision

Le contenu CK conserve un chemin solo complet. Le bridge STRE est optionnel et détecté à l’exécution. Aucun stage essentiel ne dépend d’une réponse réseau lorsque STRE est absent.

## Conséquences

- adoption plus large et meilleur découplage ;
- nécessité de tester deux modes ;
- certaines mécaniques purement coopératives ont une variante ou restent inactives en solo.
