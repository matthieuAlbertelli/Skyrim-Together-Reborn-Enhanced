# Current STRE Status

> **Status:** source of truth for implemented and validated state.
> **Last updated:** August 23, 2026.

This document describes **the repository's actual current state**. Product
direction and release gates belong in [`ROADMAP.md`](../../ROADMAP.md),
operational progress belongs in the GitHub Project governed by
[`docs/production/GITHUB_GOVERNANCE.md`](../production/GITHUB_GOVERNANCE.md),
and technical detail belongs in each feature's documentation.

## World Sync

### Implemented and validated in game

- dropped objects receive a stable network identity, `WorldEntityId`;
- each client retains its local Havok simulation;
- the player initiating an action retains settlement authority until the final
  transform;
- remote clients are corrected only when divergence is significant;
- snapshots support late materialization and binding of WorldEntities;
- movable references already present in the world are lazily adopted through
  their `PlacedReferenceId`;
- a placed reference is bound to the existing local Skyrim reference and is
  never duplicated;
- Better Grabbing is required by default in multiplayer through the generic
  native SKSE plugin policy;
- during a remote grab, observers hide the object instead of continuously
  streaming it;
- on release, placed references use STR's existing `MoveTo` path on the game
  thread through `RunnerService`;
- ownership and provenance are carried through supported paths;
- grabbing an owned object without being its owner triggers Skyrim's vanilla
  theft system;
- opening the `Dialogue Menu` cleanly ends a grab to avoid blocking guard or
  arrest dialogue.

### Known limitations

- custom names based on `ExtraTextDisplayData` are not synchronized yet;
- scripted references and quest objects still require a dedicated validation
  campaign;
- durable world persistence across server restart or save branches is not yet
  implemented;
- the WorldEntity model is not yet generalized to every type of world reference.

See [`docs/features/world-sync/`](../features/world-sync/).

## Trading

### Implemented

- dedicated session domain;
- authoritative server protocol;
- revisioned offers;
- deterministic mutation plans;
- idempotent client application;
- reconciliation to absolute quantities;
- Angular/CEF UI;
- native 3D preview.

### Limitations

- divisible stacks and gold are not supported yet;
- reconnecting an active trade needs further hardening;
- the MVP protocol does not carry all instance metadata;
- objects with ownership that cannot be represented are rejected instead of
  being transferred with data loss.

See [`docs/features/trading/`](../features/trading/).

## Item Preview

### Implemented

- native session;
- controller;
- host bridge and session;
- framing solver;
- raster measurement;
- Trading and Character Creation consumers.

### Structural limitation

The bridge still supports only one active consumer. An explicit lease and
ownership system remains necessary before declaring a stable third-party API.

See [`docs/features/item-preview/`](../features/item-preview/).

## Alternate Start / Character Build

### Implemented and smoke-tested

- versioned `STRE_AlternateStart.esp` with PSC/PEX files;
- inn, quest, aliases, and seats;
- RaceMenu and Angular Character Creation;
- shared Warrior/Mage/Thief catalog;
- canonical inventory and spells;
- hashes and application acknowledgement;
- local fallback without a server;
- Mage Destruction and Alteration;
- targeted cooperative buffs tested between two PCs;
- fresh New Game interception through `MQQuickstart = 5` and a dedicated MQ101 stage-0 STRE branch;
- direct world transition to the inn before starting the Alternate Start quest;
- same-process New Game re-entry through explicit Alternate Start quest lifecycle reset in `CharacterCreationService`;
- ordinary save loading verified not to retrigger the bootstrap;
- a fresh stage-20 handoff now opens a mandatory native/CEF campaign-bootstrap
  gate before RaceMenu; Solo releases the existing creation flow locally, while
  multiplayer releases only from a canonical sealed `CharacterCreation` snapshot
  with the complete roster `ACTIVE`; this addition is automated-tested and its
  Solo/two-player Create/Join happy path is validated in Skyrim;
- STRE-owned MQ101 continuity projection advances the required post-Helgen
  branches through stage 900, reaches MQ101 stage 1000, and leaves MQ102,
  MQ102A, and MQ102B untouched;
- `STRE_QUEST_HelgenNPCCleanup` removes the skipped Keep victims, moves Hadvar
  and Ralof to the post-escape objective, and removes the residual Imperial
  guard;
- `STRE_HelgenContinuityController` projects the validated destroyed-Helgen
  reference state and neutralizes the Keep collapse trigger while preserving
  the already-collapsed rubble presentation;
- the post-Helgen projection was runtime-smoke-tested after xEdit Quick Auto
  Clean, including Helgen exterior and `HelgenKeep01`;
- `STRE_QUEST_HelgenInvestigation` provides the local Helgen-investigation and
  standalone T+4 projection path, with a diagnostic stage-10 bootstrap and
  persistent investigation/survivor/world-phase/path state owned by
  `STRE_HelgenInvestigationController`;
- Hadvar and Ralof are independently projected to STRE-owned wounded positions
  in `HelgenKeep01` and use dedicated `SitTarget` packages with vanilla wounded
  furniture markers;
- the collapsed Keep passage has a bidirectional `Se faufiler` interaction using
  one reusable activator script, linked destination markers, a short fade, and
  local `MoveTo`, without changing the rubble collision or navmesh;
- a dead bandit and abandoned pickaxe provide environmental explanation for the
  opening through the rubble;
- `STRE_QUEST_HelgenInvestigation`, like the Alternate Start orchestration
  quest, is explicitly excluded from generic quest-stage synchronization so its
  CK stages cannot become shared campaign state;
- the standalone T+4 Helgen occupation fallback is implemented: four game days
  after investigation start it defers through `BanditOccupationPending` while
  the player remains in `HelgenLocation [00018A4A]`, then commits
  `BanditOccupied` after Helgen is clear;
- the occupied projection reuses Bethesda's `PostHelgenEncountersMarker
  [000F8240]`, removes the pre-occupation bridge/debris state and the temporary
  STRE squeeze traversal, and moves survivors still in `WoundedInCave` to
  independent locked `CapturedInKeep` jail projections;
- connected campaigns now use an ephemeral full-roster investigation-start
  barrier plus a server-evaluated `NONE inside Helgen` predicate; clients cache
  the reliable notification without blocking Papyrus, retain the local T+4
  timer, and apply the existing CK projection only when the cached predicate is
  known and true;
- the Helgen footprint is the exact `Skyrim.esm` membership of
  `HelgenLocation [00018A4A]`: eight exterior cells and three interiors,
  resolved by plugin name plus local FormID; missing roster members, unknown
  positions, a non-`ACTIVE` campaign, or an unresolved footprint all fail
  closed;
- no Helgen-specific persistent server state, event history, quest-stage sync,
  or C++ duplication of the physical projection was introduced; campaign saves
  retain the local state for future collective checkpoint recovery;
- the player-present-at-deadline -> pending -> leave-Helgen -> occupied flow was
  runtime-validated on 23 August 2026, including vanilla bandit occupation and
  both survivor jail projections;
- the strict CK manifest now covers 67 expected STRE-owned records and rejects
  unexpected master overrides; CK packaging passes with 19 managed files, the
  client/server builds are green, and TPTests pass 1794 assertions in 130 test
  cases.

The current catalog uses `BuildVersion = 5`.

### Limitations

- the New Game bootstrap and MQ101/post-Helgen world-state projection are
  implemented, but the neutral MQ102/MQ103 vanilla main-quest handoff remains
  unfinished;
- the Helgen investigation is still entered through a diagnostic quest
  bootstrap; Valen does not yet start it;
- the multiplayer T+4 vertical slice is implemented and automated-tested, but
  its two-client in-game matrix (A exits while B remains, inverse exit order,
  both already outside at T+4, and a member in `HelgenKeep01`) is not
  runtime-validated yet. The gameplay-facing campaign bootstrap from #71 is
  now available and is the supported path for the next two-PC validation
  phase. No multiplayer Helgen runtime result is claimed here;
- the diagnostic stage-10 starts are aligned only after every active sealed
  roster member reaches `BeginInvestigation()`; Valen remains the missing
  narrative trigger;
- coordinated checkpoint creation and `RECOVERY_LOCK` restore are still future
  campaign work. Helgen deliberately adds no parallel reconnect/persistence
  mechanism and fences its local progression after a campaign disconnect;
- rescue/liberation and physical `Freed`/`Departed` projections remain
  unimplemented; mixed-state and save/load/cell-reset regressions for the new
  occupation flow are still required;
- Valen and the narrative departure are not finalized;
- the live Character Build service is not yet bound to durable campaign identity
  or reconnect restoration;
- several schools and kits remain to be materialized;
- skill, perk, and attribute-history reset remains incomplete.

### Durable campaign persistence foundation

- a dedicated campaign persistence port and SQLite adapter are implemented;
- the locked server setting defaults to `state/stre-server.sqlite3`;
- the server opens, migrates, and integrity-checks the store before constructing
  its `World`, and persistence startup failure fails closed;
- schema version 2 stores multiple campaign identities, roster slots,
  `PlayerId`/`CharacterBinding` records, versioned Character Build state,
  audience-tagged adapter state, immutable snapshots, Candidate/Committed
  checkpoint metadata, per-slot native-save metadata, an append-only journal,
  and a transactional outbox;
- optimistic revisions and `MutationId` idempotency protect atomic current-state
  + journal + outbox mutations;
- accepted semantic no-ops durably reserve their `MutationId` in the same
  append-only journal without advancing canonical state or producing redundant
  outbox work; existing schema-v1 databases migrate transactionally to this
  representation;
- checkpoint restore materializes the exact immutable snapshot at a new
  monotonic revision and supersedes obsolete pending outbox work;
- file-backed automated tests cover restart, migration, partial-write rollback,
  multiple campaigns, identity mismatch, checkpoint lifecycle, exact restore,
  malformed persisted data, audience filtering, and prepared data statements;
- runtime validation created and reopened a real schema-v1 database across a
  clean server stop/restart, accepted a real Skyrim client connection, and
  confirmed fail-closed startup for an intentionally incompatible schema
  version before normal startup resumed with schema version 1. That validation
  occurred while schema v1 was current; the repository now uses schema v2.

The durable server campaign/checkpoint persistence substrate is implemented,
automated-tested, and runtime-validated. The fixed-roster/runtime core and live
admission protocol described below now use it, while coordinated native saves
and collective reconnect recovery remain unimplemented. `CharacterBuildService`
continues to use session state; durable binding to the admitted campaign slot
and character identity remains unimplemented. Coordinated
native-save/checkpoint work remains #55, and disconnect recovery lock plus
collective restore/reload remains #56. No native `.ess` payload, save/load
engine call, recovery UI, or WorldEntity persistence is part of this
foundation; durable WorldEntity persistence remains separate future work rather
than part of #55.

### Campaign roster/runtime and live protocol

The first production increment of the server-authoritative campaign runtime is
implemented and automated-tested:

- `CampaignPhase` models the canonical Lobby-through-OpenWorld sequence, while
  `CampaignRuntimeState` separately models roster eligibility and the future
  checkpoint/recovery states;
- a mutable Lobby roster uses durable `CampaignSlotId`, `PlayerId`, and
  `CharacterBindingId` values, enforces unique non-empty identities and the v1
  ten-slot limit, and is stored in deterministic slot order;
- `CommitCampaignStart` is server-authoritative at the domain boundary and
  atomically validates and seals the exact roster, establishes an explicitly
  selected roster-member Session Manager, advances
  `Lobby -> CharacterCreation`, increments the state version once, journals the
  mutation, and writes a canonical snapshot intent to the transactional outbox;
- the future session/network caller remains responsible for proving host-role
  administration before issuing that server-authorized start; roster membership
  alone grants no seal authority and transient session authority is not persisted;
- post-seal roster ownership is immutable, including across Session Manager
  transfer;
- one exact full-roster predicate distinguishes transport connectivity from
  campaign admission and rejects missing, extra, replacement, wrong-campaign,
  wrong-slot, wrong-binding, and duplicate active identities;
- exact per-slot readiness is durable, supports withdrawal and idempotent
  duplicates, and can be changed only by the matching sealed member;
- optimistic revisions and journal-backed `MutationId` replay prevent stale,
  delayed, duplicate, or out-of-order commands from regressing canonical state,
  including accepted readiness, self-transfer, and identical-roster no-ops;
- the transition-policy boundary records source, target, actor authority,
  shared preconditions, and resulting intent for every canonical phase edge.

`GameServer` owns this core over the existing `ICampaignStore`; no second
persistence layer was introduced. The existing SQLite schema was minimally
revised to v2 so accepted semantic no-op commands can share a resulting state
revision while retaining unique `MutationId` values per campaign.
Connection/admission presence is deliberately transient: a sealed exact roster
derives `ACTIVE`; any mismatch derives `WAITING_FOR_ROSTER`, and future
narrative transitions are gated by that same predicate.

The second production increment is implemented and automated-tested at the
transport/service boundary:

- each client installation has one opaque high-entropy STRE `PlayerId`, stored
  atomically in the user-local configuration directory and transported as
  identity metadata in the existing authenticated handshake; it is neither the
  server password nor a credential, username, platform identity, connection ID,
  or transient STR `Player::GetId()`;
- a non-canonical local cache retains only accepted
  `CampaignId`/`CampaignSlotId`/`CharacterBindingId` assignments for reconnect;
  malformed existing identity/cache files fail closed rather than silently
  replacing campaign identity;
- explicit typed messages on the existing STR transport cover campaign create,
  pre-seal join/leave, exact pre-seal or sealed resume admission,
  host-authorized start/seal, readiness, bounded command results, and public
  canonical snapshots; an appended join-by-code request and bounded transient
  lobby projection support the gameplay bootstrap without exposing durable
  identifiers to Angular;
- a focused server admission service keeps connection, party, and admission
  presence transient, uses `PartyService::IsPlayerLeader()` only as current
  administrative proof, and routes every durable roster/readiness/phase mutation
  through `CampaignRuntimeService`;
- campaign, slot, and character-binding identities are allocated canonically by
  the server; ready/start actors are derived from the admitted connection rather
  than trusted from packet fields;
- campaign-create retries resolve their original server assignment from the
  atomic SQLite creation journal even after a full server restart, without a
  second receipt store or duplicate campaign; the historical assignment is
  admitted only after the exact tuple is verified against the current mutable
  Lobby roster, so removed or rebound ownership is never restored;
- an existing Lobby member reconnects through exact `PlayerId`/binding resume
  without changing roster or version; a genuinely new join mutation for that
  member is rejected explicitly as resume-required;
- exact sealed-roster admission derives public `WAITING_FOR_ROSTER`/`ACTIVE`
  snapshots; disconnect removes only transient presence, and exact
  `PlayerId`/binding resume restores the same canonical slot without changing
  the durable roster;
- focused tests cover message factories and malformed packets, durable local
  identity/cache behavior, authority and spoof rejection, idempotent/stale
  mutations, 2/4/10-slot flows, disconnect/resume, and readiness no-ops.

The gameplay-facing #28 lobby slice is also implemented and automated-tested:

- a server-owned ephemeral directory maps exact four-character codes from
  `ABCDEFGHJKLMNPQRSTUVWXYZ23456789` to canonical `CampaignId` values; codes are
  collision-safe, bounded, non-persistent, and invalidated at seal;
- campaign creation marks or creates an exclusive transient PartyService party,
  and join-by-code deterministically aligns a player to that party before reusing
  the existing canonical admission mutation; failed admission rolls back only
  alignment introduced by the request, and `bAutoPartyJoin` does not admit
  players to campaign-managed parties;
- every creator/joiner supplies a trimmed, control-free pseudo bounded to 24
  Unicode code points/96 UTF-8 bytes; it is stored only in the ephemeral lobby
  directory keyed internally by `PlayerId`, projected without durable IDs, and
  invalidated at seal. It is not a Skyrim character name, identity,
  authorization input, ownership proof, binding, save, or checkpoint field;
  malformed pseudos are rejected before connection/party/campaign mutation;
  authorization remains derived server-side as `canStart`;
- the Angular surface provides only Solo, Create, Join, connection fields when
  required, code, member names/presence, Start, Back, and concise errors;
- seven focused Playwright cases pass, including required Unicode pseudo
  validation and reuse of the regular STR `last_connected_address` value
  without campaign-specific or password storage; pure/native tests cover code
  allocation, malformed wire/pseudo data, opcode stability, the five-argument
  CEF contract, and exact one-shot gate release.
- the first Skyrim validation confirmed that the stage-20 gate renders, then
  exposed a missing `campaignBootstrapAction` registration in the real CEF
  renderer; that registration is now implemented, native-tested, rebuilt, and
  deployed locally. Revalidation confirmed Solo, Create, second-PC Join by the
  four-character code, shared transient pseudos, reuse of the persisted last
  server address, and both players progressing through Character Creation into
  the STRE inn;
- runtime validation also established that `Alternate Start - Live Another Life`
  must not be active with `STRE_AlternateStart.esp`. Compatibility work is not
  part of this slice;
- the remaining campaign-bootstrap negative/runtime matrix is still pending and
  is not implied by this happy-path evidence.

The only production narrative transition currently executed by the live
campaign runtime is `Lobby -> CharacterCreation`. The fixed-roster/readiness/
phase-policy, live admission foundation, and focused New Game lobby projection
developed in the #28 workstream are implemented, but durable Character Build
binding, CK/Valen projections, feature-owned later narrative phase execution,
and Departure validation remain unimplemented. GitHub issue #28 remains open;
this focused slice does not complete it. `CHECKPOINTING`, `RECOVERY_LOCK`, and
`RESTORING_CHECKPOINT` are represented but have no #55/#56 behavior here;
native-save/checkpoint identity and fingerprinting remain #55, while disconnect
to recovery lock and collective checkpoint restore remain #56.

### Campaign save-load runtime gate spike

The isolated #60 client spike is automated-tested and human runtime-validated:

- STRE can observe the native save-load boundary and the existing post-load
  event without pretending to be a conventional SKSE messaging plugin;
- a local `CampaignRuntimeGate` plus a modal native guard menu freezes Skyrim
  gameplay and vanilla menus after a managed load;
- CEF remains available and STRE networking continues while Skyrim is paused;
- explicit release removes the guard and Skyrim resumes normally;
- a small number of STRE updates were observed before `GameIsPaused=true`, but
  no free gameplay progression was observed, so no deeper engine hook is
  justified by the evidence.

The validation-only F8/F10 controls and raw-input probes were removed after the
successful test. No production campaign save detection, automatic gate
arming/release, checkpoint coordination, or recovery state is implemented by
this spike. Production integration remains future work; #55 owns coordinated
checkpoint saves and #56 owns recovery lock plus collective restore.

See [`docs/features/alternate-start/`](../features/alternate-start/).

## Important fixed regressions

- a swimming regression introduced during STRE work;
- an observer crash while repositioning a placed reference;
- a stuck grab state during guard or arrest dialogue;
- a Google Fonts dependency that blocked offline Angular builds.

## Communication rule

Do not infer project state from an old milestone report or dated audit.

- **Current state:** this document.
- **Product direction and release gates:** [`ROADMAP.md`](../../ROADMAP.md).
- **Operational progress:** the GitHub Project governed by
  [`docs/production/GITHUB_GOVERNANCE.md`](../production/GITHUB_GOVERNANCE.md).
- **History:** [`CHANGELOG.md`](../../CHANGELOG.md) and `docs/audit/`.
