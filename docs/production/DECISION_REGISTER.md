# Registre des décisions d’architecture

> **Statut : index canonique des ADR**

Les ADR sont la source de vérité des décisions structurelles. Le
[répertoire ADR](../architecture/ADRs/README.md) définit leur usage et leur
cycle de vie; ce registre attribue les numéros et sert uniquement d’index.

| ID | Décision | Statut | ADR |
|---|---|---|---|
| ADR-0001 | Ports and Adapters pour les intégrations de mods | Accepted | [ADR-0001](../architecture/ADRs/ADR-0001-ports-and-adapters.md) |
| ADR-0002 | État de campagne autoritaire côté serveur | Accepted | [ADR-0002](../architecture/ADRs/ADR-0002-server-authoritative-campaign-state.md) |
| ADR-0003 | Alternate Start reste jouable sans STRE | Accepted | [ADR-0003](../architecture/ADRs/ADR-0003-alternate-start-standalone.md) |
| ADR-0004 | Snapshot complet plus événements incrémentaux | Accepted | [ADR-0004](../architecture/ADRs/ADR-0004-snapshot-plus-events.md) |
| ADR-0005 | Session Manager et Dragonborn sont deux rôles distincts | Accepted | [ADR-0005](../architecture/ADRs/ADR-0005-session-manager-not-dragonborn.md) |
| ADR-0006 | Aucun FormID plugin codé en dur | Accepted | [ADR-0006](../architecture/ADRs/ADR-0006-no-hardcoded-formids.md) |
| ADR-0007 | Trading comme saga compensée | Implemented | [ADR-0007](../architecture/ADRs/ADR-0007-trading-saga-reconciliation.md) |
| ADR-0008 | Runtime Item Preview à leases | Proposed | [ADR-0008](../architecture/ADRs/ADR-0008-preview-lease-manager.md) |
| ADR-0009 | Adapters first-party avant SDK tiers | Accepted | [ADR-0009](../architecture/ADRs/ADR-0009-first-party-before-third-party-sdk.md) |
| ADR-0010 | Secrets narratifs filtrés côté serveur | Accepted | [ADR-0010](../architecture/ADRs/ADR-0010-server-side-secret-filtering.md) |
| ADR-0011 | Canal CEF dédié aux fonctionnalités STRE | Proposed | [ADR-0011](../architecture/ADRs/ADR-0011-dedicated-cef-command-channel.md) |
| ADR-0012 | Scènes CK comme projections de l’état canonique | Accepted | [ADR-0012](../architecture/ADRs/ADR-0012-ck-scenes-are-projections.md) |
| ADR-0013 | Refactor Preview en composants dédiés | Implemented | [ADR-0013](../architecture/ADRs/ADR-0013-preview-refactor.md) |
| ADR-0014 | Identité réseau indépendante des FormID locaux | Accepted | [ADR-0014](../architecture/ADRs/ADR-0014-world-entity-identity.md) |
| ADR-0015 | Sauvegarde Skyrim de l’hôte comme checkpoint canonique | Accepted | [ADR-0015](../architecture/ADRs/ADR-0015-host-save-checkpoint.md) |
| ADR-0016 | État courant, journal et outbox transactionnelle | Accepted | [ADR-0016](../architecture/ADRs/ADR-0016-state-journal-outbox.md) |
| ADR-0017 | WorldEntity authority with local Havok | Accepted | [ADR-0017](../architecture/ADRs/ADR-0017-world-entity-authority-local-havok.md) |

`Implemented` est conservé sur quelques ADR historiques. Pour les nouvelles
décisions, le statut décrit la décision (`Proposed`, `Accepted`, `Rejected` ou
`Superseded`); l’avancement appartient à `STATUS.md` et aux issues.

## Décisions encore ouvertes

Une question ouverte n’est pas encore un ADR. Créer l’ADR seulement lorsque les
forces de décision et les options sont suffisamment établies.

- stockage durable campagne/WorldEntity;
- politique Dragonborn déconnecté;
- late join après départ;
- synchronisation temporelle de certaines scènes/dialogues;
- versions Skyrim/CK/SKSE officiellement supportées;
- forme finale du bridge Papyrus tiers;
- migration/version negotiation des adapters.
