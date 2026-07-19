# Registre des décisions à ratifier

> **Statut : Proposition**

| Décision | Recommandation | Document |
|---|---|---|
| Pattern global | Microkernel + Ports and Adapters | [ADR-0001](../architecture/ADRs/ADR-0001-ports-and-adapters.md) |
| Autorité campagne | serveur | [ADR-0002](../architecture/ADRs/ADR-0002-server-authoritative-campaign-state.md) |
| Mode solo Alternate Start | obligatoire | [ADR-0003](../architecture/ADRs/ADR-0003-alternate-start-standalone.md) |
| Reconnexion | snapshot + events | [ADR-0004](../architecture/ADRs/ADR-0004-snapshot-plus-events.md) |
| Session Manager / Dragonborn | séparés | [ADR-0005](../architecture/ADRs/ADR-0005-session-manager-not-dragonborn.md) |
| FormIDs | aucun hardcode plugin | [ADR-0006](../architecture/ADRs/ADR-0006-no-hardcoded-formids.md) |
| Trading | saga + réconciliation | [ADR-0007](../architecture/ADRs/ADR-0007-trading-saga-reconciliation.md) |
| Preview | runtime à leases | [ADR-0008](../architecture/ADRs/ADR-0008-preview-lease-manager.md) |
| SDK tiers | après adapters first-party | [ADR-0009](../architecture/ADRs/ADR-0009-first-party-before-third-party-sdk.md) |
| Secrets | filtrage serveur | [ADR-0010](../architecture/ADRs/ADR-0010-server-side-secret-filtering.md) |
| CEF | canal dédié | [ADR-0011](../architecture/ADRs/ADR-0011-dedicated-cef-command-channel.md) |
| Scènes CK | projection locale | [ADR-0012](../architecture/ADRs/ADR-0012-ck-scenes-are-projections.md) |
| Refactor preview en composants | implémenté | [ADR-0013](../architecture/ADRs/ADR-0013-preview-refactor.md) |

## Décisions encore ouvertes

- stockage de campagne ;
- politique Dragonborn déconnecté ;
- late join après départ ;
- synchronisation temporelle des dialogues ;
- doublons de classes ;
- versions Skyrim/CK/SKSE supportées ;
- langue canonique publique ;
- forme exacte du bridge Papyrus ;
- migration de protocole adapters.
