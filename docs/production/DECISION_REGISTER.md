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
| ADR-0018 | Fixed roster and coordinated Skyrim-save checkpoint recovery | Accepted | [ADR-0018](../architecture/ADRs/ADR-0018-fixed-roster-coordinated-checkpoint-recovery.md) |
| ADR-0019 | SQLite-backed durable server persistence | Accepted | [ADR-0019](../architecture/ADRs/ADR-0019-sqlite-durable-server-persistence.md) |
| ADR-0020 | Character Creation final local materialization | Accepted | [ADR-0020](../architecture/ADRs/ADR-0020-character-creation-final-local-materialization.md) |
| ADR-0021 | Natural-join final representation and standing Character Creation | Accepted | [ADR-0021](../architecture/ADRs/ADR-0021-natural-join-final-representation-standing-creation.md) |
| ADR-0022 | Posture-independent Character Creation and final rematerialization | Accepted | [ADR-0022](../architecture/ADRs/ADR-0022-posture-independent-character-creation.md) |
| ADR-0023 | Automatic final Character Creation rematerialization, including MASTER | Accepted | [ADR-0023](../architecture/ADRs/ADR-0023-automatic-final-character-creation-rematerialization.md) |
| ADR-0024 | Individual seating after completed Character Creation | Accepted | [ADR-0024](../architecture/ADRs/ADR-0024-individual-applied-creation-seating.md) |
| ADR-0025 | Existing CK markers as local seating approaches | Accepted | [ADR-0025](../architecture/ADRs/ADR-0025-existing-ck-markers-local-seat-approach.md) |
| ADR-0026 | Remote seating through the existing STR action stream | Accepted | [ADR-0026](../architecture/ADRs/ADR-0026-remote-seating-through-str-actions.md) |
| ADR-0027 | Animation replay continuity across a committed native binding | Accepted | [ADR-0027](../architecture/ADRs/ADR-0027-animation-replay-native-binding-continuity.md) |

`Implemented` is retained for a few historical ADRs. For new decisions, status
describes the decision (`Proposed`, `Accepted`, `Rejected`, or `Superseded`);
progress belongs in `STATUS.md` and GitHub issues.

## Open decisions

An open question is not yet an ADR. Create the ADR only when its decision forces
and options are sufficiently established.

- durable WorldEntity storage;
- post-v1 partial-roster progression, roster mutation, and catch-up;
- temporal synchronization for selected scenes and dialogue;
- officially supported Skyrim, CK, and SKSE versions;
- final form of the third-party Papyrus bridge;
- adapter migration and version negotiation.
