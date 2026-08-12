# Team Map

> **Status:** organizational proposal.

| Team | Mission | Primary deliverables | Critical interfaces |
|---|---|---|---|
| Product Direction | Vision, priorities, and arbitration | Vision, roadmap, acceptance criteria | All teams |
| STRE Core | Client/server runtime, networking, World Sync, persistence | Services, protocols, snapshots, logs | QA, Mod Integration |
| Mod Integration | Adaptation contracts and runtime | Adapter API, registry, CK bridge | STRE Core, modders |
| Alternate Start CK | Playable single-player content and Skyrim integration | ESP, quests, scenes, Papyrus, cells | Narrative, Mod Integration |
| Narrative | Continuity, scripts, characters | Narrative bible, dialogue | CK, Audio, Art |
| Character Art | Characters | Concepts, models, textures | Narrative, CK |
| Environment Art | Spaces and props | Dressing, lighting, optimization | CK, QA |
| Audio | Casting, voice, processing | Masters, lip files, naming | Narrative, CK |
| UI/UX | Cooperative interfaces | Flows, Angular/CEF, accessibility | STRE Client, QA |
| QA & Compatibility | Single-player, network, and load-order validation | Plans, matrix, reports | All teams |
| Build & Release | CI, packages, versions | Artifacts, changelog, releases | STRE Core, CK |
| Community & Docs | Onboarding and communication | README, guides, contribution | Product Direction |

## Cross-functional groups

### World Sync

STRE Core and QA. External SKSE integrations remain dependencies, not core
submodules.

### Trading

STRE Core, UI/UX, and QA.

### Alternate Start

Alternate Start CK, Mod Integration, STRE Core, Narrative, Art, Audio, and QA.

### Preview Platform

STRE Client, UI/UX, and Mod Integration.

## Minimum ownership

Every active domain must have:

- a functional owner;
- a technical or creative lead;
- a secondary reviewer;
- clearly located canonical documentation.
