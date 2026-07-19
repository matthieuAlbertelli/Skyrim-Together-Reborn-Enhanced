# Trading — Référence protocole

> **Statut : Implémenté**

## Identifiants

| Champ | Type | Rôle |
|---|---|---|
| `SessionId` | uint64 | transaction logique |
| `Revision` | uint64 | version de l’offre globale |
| `ApplyId` | uint64 | tentative d’application |
| `ReconcileId` | uint64 | tentative de réconciliation |
| `PlayerId` | uint32 | participant |
| `ItemId` | uint64 | `ModId << 32 | BaseId` |

## Client → serveur

| Message | Données | Précondition |
|---|---|---|
| Invite | target player | aucun trade actif |
| InviteResponse | session, accepted | destinataire |
| OfferUpdate | session, expected revision, full offer | négociation |
| Confirm | session, revision | révision courante |
| Cancel | session | participant, non applying |
| ApplyResult | session, revision, apply id, result | application attendue |
| ReconcileResult | session, revision, apply/reconcile ids, result | réconciliation attendue |

## Serveur → client

| Message | Rôle |
|---|---|
| Invite | afficher l’invitation |
| Started | créer le contexte local |
| State | snapshot autoritaire de session |
| Apply | mutations signées locales |
| Reconcile | quantités cibles absolues |
| Cancelled | motif d’annulation |

## Bornes

- 64 lignes par offre ;
- 64 mutations par plan ;
- 64 cibles par réconciliation ;
- décodeur invalide la structure si la borne est dépassée.

## Compatibilité future

Tout changement de structure doit préciser :

- version de protocole ;
- clients minimum ;
- stratégie de downgrade ;
- comportement d’un serveur ancien ;
- tests de round-trip et malformed payload.
