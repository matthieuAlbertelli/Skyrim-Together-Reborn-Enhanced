# ADR-0014 — Identité réseau indépendante des FormID locaux

- **Statut : Accepted**
- **Date : 2026-07-30**

## Contexte

Les références temporaires créées par Skyrim reçoivent des FormID locaux différents sur chaque client et peuvent changer après un chargement.

## Décision

Toute entité dynamique synchronisée reçoit un `WorldEntityId` attribué par le serveur. Les FormID locaux sont conservés uniquement dans un registre de liaison client versionné par génération.

## Conséquences

- les messages métier n’utilisent jamais un FormID temporaire comme identité durable ;
- chaque client maintient `WorldEntityId ↔ FormID local` ;
- les liaisons sont reconstruites après chargement ;
- les ambiguïtés de rattachement sont exposées comme conflits de réconciliation.
