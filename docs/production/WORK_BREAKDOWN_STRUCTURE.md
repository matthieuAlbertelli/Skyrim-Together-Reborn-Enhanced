# Work Breakdown Structure

> **Status:** canonical structural decomposition; contains no progress state.

The WBS describes **the stable domains that make up STRE**. Product direction
and release gates belong in [`ROADMAP.md`](../../ROADMAP.md), implemented and
validated state in [`docs/project/STATUS.md`](../project/STATUS.md), and
operational progress in the GitHub Project governed by
[`GITHUB_GOVERNANCE.md`](GITHUB_GOVERNANCE.md).

## Product domain map

| Domain | Responsibility | Primary repository boundaries | GitHub labels |
|---|---|---|---|
| Multiplayer core | transport, protocol, sessions, party, network identity, client/server services | `Code/client`, `Code/server`, `Code/encoding`, `Code/common` | `area: networking` |
| World/entity synchronization | entity lifecycle, actors, dropped objects, placed references, grabs, mounts, transforms, snapshots | `Character`/`Object`/`Inventory` services, WorldEntity messages, `docs/features/world-sync` | `area: world-sync`, `area: actors`, `area: mounts` |
| Trading | trade session, authority, application, reconciliation, UX | `Code/common/Trade`, client/server services, Trading UI, `docs/features/trading` | `area: trading`, `area: ui` |
| Cooperative systems | shared combat, magic, party, and quest systems; accepted future cooperative mechanics | existing services and each feature's canonical directory | the concrete domain label; no artificial generic label |
| Alternate Start and creation | new game/Helgen, RaceMenu, creation, reset, canonical build, single-player, departure | Character Creation, Alternate Start UI, CK/Papyrus, `docs/features/alternate-start` | `area: alternate-start`, `area: ui` |
| Class program | canonical roster, kits, abilities/perks, personal quests, and validation for 21 classes | roster/spec, shared catalogs, UI, CK/Papyrus, class issues | `area: classes`, `area: quests` |
| Cooperative campaign | campaign/character identity, persistence, coordinated checkpoints, phases, sealed roster, readiness, disconnect recovery, collective restore | Campaign State architecture and future client/server implementations | `area: campaign`, `area: networking` |
| Valen and narrative | narrative contract, actor, voice, shared scene, phase projection | `docs/narrative`, `docs/art`, `docs/audio`, CK/Papyrus | `area: valen`, `area: quests` |
| Headquarters and housing | hub, ten rooms, room identity, ownership/assignment, restoration | CK content, campaign/persistence, `docs/features/alternate-start` | `area: housing` |
| UI and preview | Angular/CEF surfaces, typed commands, 3D preview, accessibility | `Code/skyrim_ui`, UI/preview services, `docs/features/item-preview` | `area: ui`; consumers add their domain |
| Build, test, and release | toolchain, CI, packaging, compatibility, evidence, publication | xmake, `.github/workflows`, `Tools`, `docs/development`, `docs/testing` | `area: build-ci` |
| Documentation and governance | sources of truth, ADRs, contribution, GitHub Project/Milestone, history | `docs`, root governance files, `.github` | `area: docs` |

**Ownership and authority** are cross-cutting rules, not a separate domain: every
shared state names its authority in architecture and in the issue that changes
it. **CK/Papyrus** is a content and adaptation boundary, not a second source of
truth; it projects the domain to which its record or script belongs.

Current labels cover actionable domains. `area: campaign` is necessary because
continuity, persistence, and phases form a durable product and are not reducible
to network transport or Alternate Start. Separate `core`, `authority`, `CK`,
`Papyrus`, `testing`, `release`, or `co-op` labels are not created: these are
cross-cutting layers already represented by the concrete domain,
`area: build-ci`, or `area: docs`.

## 1. Multiplayer core and networking

- 1.1 Client runtime and services
- 1.2 Server runtime and services
- 1.3 Transport, protocol factories, and bounds
- 1.4 Shared identities and authority contracts
- 1.5 Session, party, player, and authentication
- 1.6 Observability and diagnostics
- 1.7 Native plugin policy

## 2. World/entity synchronization

- 2.1 Actor and entity lifecycle
- 2.2 Dynamic dropped-item materialization
- 2.3 Placed-reference lazy adoption
- 2.4 Grab/manipulation authority and Better Grabbing
- 2.5 Local Havok settlement and reconciliation
- 2.6 Ownership, provenance, and interaction
- 2.7 Mount occupancy and presentation
- 2.8 Snapshot and late materialization
- 2.9 Durable world persistence and checkpoints
- 2.10 Validation of scripted/quest references and new types

## 3. Trading

- 3.1 Domain, session, and protocol
- 3.2 Server authority and validation
- 3.3 Client application, idempotence, and reconciliation
- 3.4 UI/UX and preview
- 3.5 Instance metadata
- 3.6 Gold and stack support
- 3.7 Reconnect, recovery, and validation

## 4. Cooperative systems and Item Preview

- 4.1 Existing combat, magic, party, and quest synchronization
- 4.2 Downed/recovery and other features only after product acceptance
- 4.3 Item Preview native session, controller, solver, and host
- 4.4 Multi-consumer leases and arbitration
- 4.5 Lifecycle and concurrency tests

## 5. Alternate Start and Character Creation

- 5.1 CK cell, quest, and aliases
- 5.2 New-game interception and Helgen continuity
- 5.3 RaceMenu, Angular flow, and preview
- 5.4 Anti-import reset
- 5.5 CharacterBuild catalog, inventory, and spells
- 5.6 Single-player fallback
- 5.7 Valen, readiness, and departure projection
- 5.8 Save, load, and collective checkpoint recovery

## 6. Twenty-one-class program

- 6.1 Roster and canonical identity
- 6.2 Exact kits and loadouts
- 6.3 Cooperative abilities and perks
- 6.4 One personal quest per class
- 6.5 Consistent catalogs, CK/Papyrus, UI, and versioning
- 6.6 Single-player/server equivalence
- 6.7 Automated and in-game validation

## 7. Cooperative campaign

- 7.1 Campaign/character identity and binding
- 7.2 Storage, migration, snapshots, and coordinated checkpoints
- 7.3 Sealed roster, readiness, and full-roster eligibility
- 7.4 Introduction, departure, and shared phases
- 7.5 Session Manager/Dragonborn policy
- 7.6 Disconnect recovery, checkpoint restore, and roster integrity
- 7.7 Journal, outbox, and recovery
- 7.8 Local CK/UI projection

## 8. Valen, headquarters, and housing

- 8.1 Narrative and scene contract
- 8.2 Actor, art, voice, and localization
- 8.3 CK scene and phase projection
- 8.4 Headquarters layout, navmesh, and performance
- 8.5 Ten usable room identities
- 8.6 Assignment, ownership, persistence, and restoration

## 9. CK/Papyrus, UI, and mod integration

- 9.1 CK/Papyrus ↔ STRE bridge
- 9.2 Idempotent projection and single-player operation
- 9.3 Angular/CEF surfaces and typed contracts
- 9.4 First-party adapters
- 9.5 Capability and version negotiation
- 9.6 Third-party SDK only after sufficient first-party validation

## 10. Project operations

- 10.1 Build, CI, and packaging
- 10.2 Tests, compatibility, and evidence bundles
- 10.3 Versioning, changelog, tags, and GitHub Releases
- 10.4 Documentation, ADRs, and history
- 10.5 GitHub governance and contribution
- 10.6 Licenses, provenance, and security
