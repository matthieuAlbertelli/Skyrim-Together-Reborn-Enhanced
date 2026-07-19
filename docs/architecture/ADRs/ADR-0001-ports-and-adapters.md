# ADR-0001 — Ports and Adapters pour les intégrations de mods

- **Statut : Accepted (direction proposée à ratifier)**
- **Date : 2026-07-19**

## Contexte

STRE veut permettre à un mod solo de décrire sa sémantique multijoueur sans intégrer chaque détail de ce mod dans le cœur réseau.

## Décision

Adopter une Plugin Architecture/Microkernel dans laquelle chaque intégration est un `STRE Mod Adapter`. Le cœur expose des ports : capabilities, intents, état canonique, événements, snapshots et policies. Les adapters traduisent les concepts du mod.

## Conséquences

- séparation claire et testabilité ;
- fonctionnement solo préservé ;
- coût initial de conception du runtime ;
- besoin de versionner les contrats ;
- aucune promesse de compatibilité automatique.
