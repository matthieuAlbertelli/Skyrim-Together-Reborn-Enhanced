# Trading — Technical Design

> **Statut : Implémenté**

## Architecture

```text
Trade domain (pur)
├─ Session
├─ Inventory planning
├─ Application
└─ Reconciliation

Protocol
├─ 7 client requests
└─ 6 server notifications

Server TradeService
├─ session registry
├─ player/session index
├─ application registry
├─ reconciliation baselines
└─ cleanup/expiry

Client
├─ TradeService
├─ TradeMenuService
├─ TradeItemPreviewService
└─ Angular Trade UI
```

## Automate

```text
PendingAcceptance
  ├─ accept → Negotiating
  └─ reject/expire/disconnect → Cancelled

Negotiating
  ├─ offer update → Negotiating + revision++ + confirmations reset
  ├─ both confirm current revision → Locked
  └─ cancel/disconnect → Cancelled

Locked
  ├─ valid inventory plan → Applying
  └─ validation failure → Failed

Applying
  ├─ both success + server commit → Completed
  ├─ uncertain/failure → reconciliation
  └─ timeout/disconnect → reconciliation or terminal failure
```

## Idempotence

- offre identique : succès sans changement ;
- acceptation répétée : succès sans changement ;
- confirmation répétée de la même révision : succès sans changement ;
- application client : résultat mémorisé par `ApplyId` ;
- réconciliation client : résultat mémorisé par `ReconcileId` ;
- snapshots d’offre complets plutôt que deltas UI.

## Validation d’inventaire

Le serveur construit un `InventorySnapshot` par joueur, valide les lignes, détecte ambiguïtés et incompatibilités, puis crée deux `PlayerMutationPlan`. Les offres réciproques du même item sont nettées de façon déterministe.

## Application et commit

Le serveur envoie à chaque participant uniquement son plan. Les clients appliquent les mutations à l’inventaire Skyrim local et renvoient un résultat structuré. Lorsque les deux réussissent, le serveur applique le plan à ses `InventoryComponent`.

## Réconciliation

Une baseline absolue est capturée au début de l’application. En cas d’incertitude, le serveur construit ou réutilise un `ReconciliationPlan` contenant les quantités cibles par item. Les clients convergent vers ces valeurs, plutôt que de rejouer un delta.

## Nettoyage

- session terminale retenue brièvement pour absorber les messages tardifs ;
- index joueur/session libéré ;
- applications et baselines supprimées ;
- expiration périodique.

## Points d’extension

- `TransferPolicy` pour sortir `IsMvpTransferable` des services ;
- `InventoryAdapter` pour rendre l’application testable sans Skyrim ;
- persistance optionnelle des sagas ;
- telemetry structurée ;
- capability `player-trade/1` du futur framework.
