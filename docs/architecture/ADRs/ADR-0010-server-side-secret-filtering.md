# ADR-0010 — Les secrets narratifs sont filtrés côté serveur

- **Statut : Accepted**
- **Date : 2026-07-19**

## Contexte

Masquer l’identité du Dragonborn uniquement dans l’UI ne protège pas la donnée contre les logs ou l’inspection mémoire.

## Décision

Le serveur produit des vues de snapshot par audience. Un client non autorisé ne reçoit jamais la donnée secrète.

## Conséquences

- meilleure intégrité narrative ;
- schemas publics/privés ou champs filtrables ;
- tests d’audience ;
- logs serveur protégés.
