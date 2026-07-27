# Protocole réseau : état actuel et évolution

> **Statut : Implémenté pour Trading et Character Build / Proposé pour Mod Integration générique**

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

### Modèle transactionnel

Le trading est une **saga autoritaire** : validation serveur, application locale, collecte des résultats puis commit ou réconciliation absolue.

## Character Build actuel

### Messages

Client vers serveur :

1. `CharacterBuildRequest`
2. `CharacterBuildAppliedRequest`

Serveur vers client :

1. `CharacterBuildResponse`
2. `NotifyCharacterBuildState`

### Requête logique

Le client transmet :

- `BuildVersion` ;
- `RaceId` ;
- `ClassId` ;
- une liste bornée de couples `GroupId` / `OptionId`.

Il ne transmet ni l’inventaire final ni une liste arbitraire de sorts.

### Snapshot canonique

`CharacterBuildSnapshotData` contient :

```text
BuildVersion
RaceId
ClassId
Selections
CanonicalInventory
InventoryHash
CanonicalSpells
SpellHash
```

Le catalogue courant utilise `BuildVersion = 5`.

### Validation et accusé

Le serveur :

1. valide la version, la race, la classe et les sélections ;
2. résout les plugins et FormIDs locaux ;
3. construit, trie et déduplique les récompenses ;
4. calcule les hashes d’inventaire et de sorts ;
5. remplace l’inventaire serveur par le snapshot canonique ;
6. envoie l’état `Accepted`.

Le client nettoie puis applique le snapshot. Il envoie ensuite :

```text
Revision
InventoryHash
SpellHash
```

Le serveur refuse notamment :

- `RejectedVersion` ;
- `RejectedInvalidRace` ;
- `RejectedInvalidBuild` ;
- `RejectedMissingPlugin` ;
- `RejectedRevision` ;
- `RejectedInventoryHash` ;
- `RejectedSpellHash`.

Un accusé valide place le build à l’état `Applied` et le niveau serveur à 1.

### Limites

- état en mémoire pendant la session ;
- pas de persistance/reconnexion durable ;
- protocole spécifique Character Build, pas encore une enveloppe générique d’adapter ;
- compatibilité réseau requiert la même `BuildVersion` et le même catalogue/plugin sur les clients.

## Évolution pour Mod Integration

Éviter un type statique par champ de chaque mod. Une future enveloppe versionnée peut prendre la forme :

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

Notifications proposées :

- `NotifyAdapterManifest`
- `NotifyAdapterSnapshot`
- `NotifyAdapterEvent`
- `NotifyAdapterCommandRejected`

## Compatibilité future

- négociation des versions d’adapter à la connexion ;
- refus explicite si une capability obligatoire manque ;
- migration de snapshot côté serveur ;
- jamais de désérialisation non bornée ;
- opcodes génériques stables et schémas de payload versionnés.
