# Architecture Decision Register

> **Status:** canonical ADR index.

ADRs are the source of truth for structural decisions. The
[ADR directory](../architecture/ADRs/README.md) defines their use and lifecycle;
this register allocates numbers and serves only as an index.

| ID | Decision | Status | ADR |
|---|---|---|---|
| ADR-0001 | Ports and Adapters for mod integrations | Accepted | [ADR-0001](../architecture/ADRs/ADR-0001-ports-and-adapters.md) |
| ADR-0002 | Server-authoritative campaign state | Accepted | [ADR-0002](../architecture/ADRs/ADR-0002-server-authoritative-campaign-state.md) |
| ADR-0003 | Alternate Start remains playable without STRE | Accepted | [ADR-0003](../architecture/ADRs/ADR-0003-alternate-start-standalone.md) |
| ADR-0004 | Full snapshot plus incremental events | Accepted | [ADR-0004](../architecture/ADRs/ADR-0004-snapshot-plus-events.md) |
| ADR-0005 | Session Manager and Dragonborn are separate roles | Accepted | [ADR-0005](../architecture/ADRs/ADR-0005-session-manager-not-dragonborn.md) |
| ADR-0006 | No hard-coded plugin FormIDs | Accepted | [ADR-0006](../architecture/ADRs/ADR-0006-no-hardcoded-formids.md) |
| ADR-0007 | Trading as a compensating saga | Implemented | [ADR-0007](../architecture/ADRs/ADR-0007-trading-saga-reconciliation.md) |
| ADR-0008 | Leased Item Preview runtime | Proposed | [ADR-0008](../architecture/ADRs/ADR-0008-preview-lease-manager.md) |
| ADR-0009 | First-party adapters before a third-party SDK | Accepted | [ADR-0009](../architecture/ADRs/ADR-0009-first-party-before-third-party-sdk.md) |
| ADR-0010 | Narrative secrets filtered by the server | Accepted | [ADR-0010](../architecture/ADRs/ADR-0010-server-side-secret-filtering.md) |
| ADR-0011 | Dedicated CEF channel for STRE features | Proposed | [ADR-0011](../architecture/ADRs/ADR-0011-dedicated-cef-command-channel.md) |
| ADR-0012 | CK scenes as projections of canonical state | Accepted | [ADR-0012](../architecture/ADRs/ADR-0012-ck-scenes-are-projections.md) |
| ADR-0013 | Preview refactored into dedicated components | Implemented | [ADR-0013](../architecture/ADRs/ADR-0013-preview-refactor.md) |
| ADR-0014 | Network identity independent of local FormIDs | Accepted | [ADR-0014](../architecture/ADRs/ADR-0014-world-entity-identity.md) |
| ADR-0015 | Host Skyrim save as canonical checkpoint | Superseded | [ADR-0015](../architecture/ADRs/ADR-0015-host-save-checkpoint.md) |
| ADR-0016 | Current state, journal, and transactional outbox | Accepted | [ADR-0016](../architecture/ADRs/ADR-0016-state-journal-outbox.md) |
| ADR-0017 | WorldEntity authority with local Havok | Accepted | [ADR-0017](../architecture/ADRs/ADR-0017-world-entity-authority-local-havok.md) |
| ADR-0018 | Fixed roster and coordinated Skyrim-save checkpoint recovery | Proposed | [ADR-0018](../architecture/ADRs/ADR-0018-fixed-roster-coordinated-checkpoint-recovery.md) |

`Implemented` is retained for a few historical ADRs. For new decisions, status
describes the decision (`Proposed`, `Accepted`, `Rejected`, or `Superseded`);
progress belongs in `STATUS.md` and GitHub issues.

## Open decisions

An open question is not yet an ADR. Create the ADR only when its decision forces
and options are sufficiently established.

- durable campaign and WorldEntity storage;
- post-v1 partial-roster progression, roster mutation, and catch-up;
- temporal synchronization for selected scenes and dialogue;
- officially supported Skyrim, CK, and SKSE versions;
- final form of the third-party Papyrus bridge;
- adapter migration and version negotiation.
