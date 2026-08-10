# Registre des décisions d’architecture

> **Statut : index des ADR**

Les ADR sont la source de vérité des décisions structurelles. Ce registre sert uniquement d’index.

| Décision | ADR |
|---|---|
| Pattern global : Ports and Adapters | [ADR-0001](../architecture/ADRs/ADR-0001-ports-and-adapters.md) |
| Autorité campagne serveur | [ADR-0002](../architecture/ADRs/ADR-0002-server-authoritative-campaign-state.md) |
| Alternate Start autonome en solo | [ADR-0003](../architecture/ADRs/ADR-0003-alternate-start-standalone.md) |
| Reconnexion snapshot + events | [ADR-0004](../architecture/ADRs/ADR-0004-snapshot-plus-events.md) |
| Session Manager séparé du Dragonborn | [ADR-0005](../architecture/ADRs/ADR-0005-session-manager-not-dragonborn.md) |
| Aucun hardcode de FormID chargé | [ADR-0006](../architecture/ADRs/ADR-0006-no-hardcoded-formids.md) |
| Trading saga + réconciliation | [ADR-0007](../architecture/ADRs/ADR-0007-trading-saga-reconciliation.md) |
| Preview runtime à leases | [ADR-0008](../architecture/ADRs/ADR-0008-preview-lease-manager.md) |
| First-party avant SDK tiers | [ADR-0009](../architecture/ADRs/ADR-0009-first-party-before-third-party-sdk.md) |
| Secrets filtrés serveur | [ADR-0010](../architecture/ADRs/ADR-0010-server-side-secret-filtering.md) |
| Canal CEF dédié | [ADR-0011](../architecture/ADRs/ADR-0011-dedicated-cef-command-channel.md) |
| Scènes CK comme projections | [ADR-0012](../architecture/ADRs/ADR-0012-ck-scenes-are-projections.md) |
| Refactor Preview en composants | [ADR-0013](../architecture/ADRs/ADR-0013-preview-refactor.md) |
| WorldEntity authority + local Havok | [ADR-0014](../architecture/ADRs/ADR-0014-world-entity-authority-local-havok.md) |

## Décisions encore ouvertes

- stockage durable campagne/WorldEntity;
- politique Dragonborn déconnecté;
- late join après départ;
- synchronisation temporelle de certaines scènes/dialogues;
- versions Skyrim/CK/SKSE officiellement supportées;
- forme finale du bridge Papyrus tiers;
- migration/version negotiation des adapters.
