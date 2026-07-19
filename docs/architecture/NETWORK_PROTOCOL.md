# Protocole réseau : existant et évolution

> **Statut : Implémenté pour Trading / Proposé pour Mod Integration**

## Trading actuel

### Requêtes client

1. `TradeInviteRequest`
2. `TradeInviteResponseRequest`
3. `TradeOfferUpdateRequest`
4. `TradeConfirmRequest`
5. `TradeCancelRequest`
6. `TradeApplyResultRequest`
7. `TradeReconcileResultRequest`

### Notifications serveur

1. `NotifyTradeInvite`
2. `NotifyTradeStarted`
3. `NotifyTradeState`
4. `NotifyTradeApply`
5. `NotifyTradeReconcile`
6. `NotifyTradeCancelled`

Les listes sont bornées à 64 entrées pour offres, mutations et targets.

## Sémantique

- `SessionId` identifie le trade.
- `Revision` change à chaque modification réelle d’offre.
- les confirmations portent sur une révision précise ;
- `ApplyId` identifie une tentative d’application ;
- `ReconcileId` identifie une correction absolue ;
- les clients journalisent les résultats pour répondre identiquement aux doublons.

## Modèle transactionnel réel

Le trading n’est pas une transaction ACID distribuée. C’est une **saga autoritaire** :

1. le serveur valide et produit un plan ;
2. chaque client applique sa part ;
3. le serveur collecte les résultats ;
4. il commit son état si tous réussissent ;
5. en cas d’incertitude, il envoie une cible absolue de réconciliation.

Cette formulation doit être utilisée dans la documentation publique afin de ne pas surpromettre une atomicité impossible entre deux processus de jeu.

## Évolution pour Mod Integration

Éviter un nouveau type statique pour chaque champ de chaque mod. Ajouter une enveloppe versionnée :

```cpp
struct ModCommandEnvelope
{
    AdapterId Adapter;
    CapabilityId Capability;
    SchemaVersion Schema;
    RequestId Request;
    StateVersion ExpectedVersion;
    BoundedByteBuffer Payload;
};
```

Notifications :

- `NotifyAdapterManifest`
- `NotifyAdapterSnapshot`
- `NotifyAdapterEvent`
- `NotifyAdapterCommandRejected`

## Compatibilité

- négociation des versions d’adapter à la connexion ;
- refus explicite si une capability obligatoire manque ;
- migration de snapshot côté serveur ;
- jamais de désérialisation non bornée ;
- opcodes génériques stables, schemas de payload versionnés.
