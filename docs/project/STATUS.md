# Current STRE Status

> **Status:** source of truth for implemented and validated state.
> **Last updated:** August 27, 2026.

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
  [000F8240]`, retires the major post-attack fire/smoke FX and the temporary STRE
  squeeze traversal, preserves the collapsed bridge/debris projection, and moves
  survivors still in `WoundedInCave` to independent locked `CapturedInKeep`
  jail projections;
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
  both survivor jail projections; revalidation on 24 August confirmed the final
  FX/encounters/rubble/bridge invariant in standalone and in a multiplayer
  campaign;
- the strict CK manifest now covers 67 expected STRE-owned records and rejects
  unexpected master overrides, including exact allowlisting of the two captured
  jail-door bindings; CK packaging passes with 19 managed files, the client/server
  builds are green, and TPTests pass 2199 assertions in 158 test cases.

The current catalog uses `BuildVersion = 5`.

### Limitations

- the New Game bootstrap and MQ101/post-Helgen world-state projection are
  implemented, but the neutral MQ102/MQ103 vanilla main-quest handoff remains
  unfinished;
- the Helgen investigation is still entered through a diagnostic quest
  bootstrap; Valen does not yet start it;
- the multiplayer T+4 vertical slice and its final occupied projection are
  runtime-validated in a multiplayer campaign, but the complete permutation
  matrix (both exit orders, both already outside at T+4, interior/exterior,
  disconnect, mixed survivor states, save/load, and cell reset) remains pending;
- the diagnostic stage-10 starts are aligned only after every active sealed
  roster member reaches `BeginInvestigation()`; Valen remains the missing
  narrative trigger;
- coordinated checkpoint creation is implemented and automated/build-tested;
  its nominal sealed-roster Candidate/ACK/commit path is runtime-validated with
  two real Skyrim clients. Failure, disconnect, ACK-replay, and commit-boundary
  resilience remain tracked by [#72](https://github.com/matthieuAlbertelli/Skyrim-Together-Reborn-Enhanced/issues/72).
  `RECOVERY_LOCK` restore is implemented, automated/build-tested, and live
  validated by #56 with two clients across nominal, successive, and
  restart-rehydrated recovery. Helgen
  deliberately adds no parallel reconnect/persistence mechanism and fences its
  local progression after a campaign disconnect;
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
admission protocol described below now use it, and the coordinated native-save
flow described below drives its Candidate/ACK/commit primitives. Collective
reconnect recovery is implemented, automated/build-tested, and live validated
with two clients, including a successive checkpoint/recovery cycle and durable
incomplete-attempt rehydration after restart without a second restore.
`CharacterBuildService`
continues to use session state; durable binding to the admitted campaign slot
and character identity remains unimplemented. The nominal #55 two-PC checkpoint
path is runtime-validated; its remaining live resilience matrix is tracked by
[#72](https://github.com/matthieuAlbertelli/Skyrim-Together-Reborn-Enhanced/issues/72).
Disconnect recovery lock plus collective restore/reload is implemented by #56.
Native `.ess` payloads remain local and are not uploaded to server persistence;
durable WorldEntity persistence remains separate future work rather than part of
#55 or #56.

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

The production cold-session **Load Campaign / Resume campaign** surface is
implemented and automated/build-tested. Its marked-save backend path has been
exercised after a full Skyrim restart through exact #56 restore and completion.
The exact-target correction was also exercised live. That rerun exposed a final
presentation lifecycle defect after completion; the successful disconnect-flow
rehydration rerun now also live validates its terminal-close correction. A
later same-process Quit-to-Main-Menu rerun confirmed that volatile admission is
cleared, the durable binding is retained, and the real transport is closed. It
also exposed a distinct client-only presentation defect: the intentional
disconnect still projected a gameplay recovery gate over the Main Menu. The
semantic local-gate correction and the disconnect incident UX are now live
validated: the Main Menu remains responsive, the gate releases only at
`MainMenuEntered`, the durable binding remains available, and Continue/Resume
re-enters the existing #56 rehydration path:

- `CampaignIdentityStore` exposes bounded read-only access to its existing
  non-canonical binding cache. Ordinary F2 Resume may enumerate zero, one, or
  multiple candidates and never selects one implicitly. ResumeRequired instead
  loads only the binding named by the exact save marker and requires campaign,
  slot-hint, and character-binding equality, yielding zero or one opaque target.
  It never enumerates or projects unrelated campaigns. Malformed/missing data
  fails closed, and the existing successful Leave path removes the candidate;
- the normal connected STR menu exposes `Resume campaign`. Create/Join and
  Resume now share the same campaign shell and roster primitives rather than
  maintaining a parallel resume popup. Angular receives only ephemeral 128-bit
  local tokens, ordinal/presence roster labels, and a local-slot flag, never
  campaign, slot, player, or character-binding identities. Duplicate selection
  is suppressed in the UI and remains idempotent at the native state boundary;
- a thin `CampaignResumeService` resolves the selected token locally and invokes
  the existing `CampaignService::ResumeCampaign()` request. It creates no local
  admission and is completely separate from `CampaignBootstrapState`, so resume
  cannot emit Character Creation authorization for an existing save;
- only the existing authoritative successful server response can establish
  admission. Binding/identity mismatch, deleted campaign, invalid cache,
  unavailable session, send failure, and local admission failure are projected
  as bounded errors without host privilege, fallback, or synthesized state;
- every successfully completed #55 checkpoint save now receives a bounded
  `stre-campaign-save-v1` sidecar in the existing local identity directory. It
  records only campaign, slot hint, character binding, checkpoint, and exact
  native logical identity; no secret, snapshot, path, or authority is stored.
  Failure to write it fails the checkpoint ACK. During a campaign, ordinary
  Manual/Quick attempts are now routed into the collective #55 flow while
  autosaves and unknown save families are blocked; only the eventual managed
  save is marked. Outside campaigns Skyrim's save behavior remains vanilla;
- loading an identity with an exact valid local campaign marker arms the existing
  `CampaignRuntimeGate` before `Load_Impl`. A `stre-*` name alone grants no
  authority; missing/corrupt metadata blocks the native load before the gate,
  and failure to arm also blocks. After `TESLoadGameEvent`, the
  guard menu pauses gameplay while CEF/F2 and networking remain usable. The UI
  opens once at the first in-game boundary, states that resume is mandatory,
  renders no campaign list and at most one exact opaque action, embeds the existing
  connection form when disconnected, and offers no close or solo fallback. F2
  may hide and reopen the unchanged view without releasing the native gate;
- the shared surface distinguishes connection/admission, authoritative sealed-
  roster waiting with per-slot presence, native checkpoint restoration, and
  restored-snapshot synchronization through a persistent six-step progress
  projection. A local recovery failure stays fenced and
  can retry only by replaying the selected campaign through the existing
  idempotent Resume request. The surface disappears only after the correlated
  recovery completion has released the gate; no Angular action can produce
  `ACTIVE`;
- ResumeRequired completion is terminal. After authoritative `ACTIVE` and the
  correlated gate release, native state clears the exact token, candidates,
  marker, roster, and error, projects idle/unavailable, and closes only the STR
  surface. Angular closes the mandatory view on the same terminal projection.
  It never falls through into OrdinaryResume; an ordinary candidate enumeration
  occurs only after a later explicit `Resume campaign` action. Failures and
  retries remain visible and fenced;
- `MainMenuOpened` had previously been trace-only, so quitting an admitted
  loaded game left the transport connected, the client admission present, and
  the server roster projected as present. The event now ends only an existing
  admitted runtime: it clears volatile admission/projections and the automatic
  reconnect candidate while preserving the durable binding, then closes the
  actual transport. The unchanged server disconnect path marks the slot absent
  and opens/retains #56 recovery without `LeaveCampaign`, roster mutation, new
  protocol, or N=1 branch. A boot Main Menu is distinguished solely by the
  absence of admission, not by time or frame heuristics;
- the exact transport close initiated by that semantic runtime departure now
  carries a bounded local context until the next connection into
  `CampaignRecoveryService`. The server
  still observes the disconnect and retains its authoritative recovery
  semantics, but the client logs `LOCAL_GATE_SKIPPED` and does not create a
  provisional recovery lock or open `STRECampaignGateMenu` while no gameplay
  world is present. Ordinary transport loss in an admitted gameplay world still
  locks fail-closed. The context is consumed once and cannot make later
  recovery globally ungated: a subsequent marked native load arms the existing
  ResumeRequired gate and hands it to #56 until authoritative completion;
- an ACTIVE-to-recovery snapshot now opens a presentation-only disconnect
  incident over the already-locked gameplay gate. It derives one/multiple/
  restored missing-member state from the current authoritative sealed roster;
  because the public snapshot has no durable display name, Angular receives
  ordinal/presence fallback data rather than transient player IDs. Local
  transport loss uses distinct connection-lost wording. `StayAndRecover` sends
  no protocol and invokes no load: it only selects the existing ResumeRequired/
  recovery projection, which remains driven by the real #56 load request and
  completion. The N=2 `StayAndRecover` path is live validated through exact
  checkpoint load, both #56 barriers, authoritative completion, UI close, and
  gate release. The alternative Main Menu action is also live validated as
  described below;
- the incident's Main Menu action requests only Skyrim's top-level `Main::resetGame`
  boundary, then waits for the existing semantic `CampaignMainMenuEnteredEvent`
  emitted when the Main Menu opens
  before clearing the UI-only incident and releasing only the local gameplay
  presentation. The established runtime-departure lifecycle clears volatile
  admission/projections, closes transport, retains the durable binding, and
  emits no `LeaveCampaign`; the server's durable recovery remains unchanged.
  Its first live click crashed before any previously available native action
  diagnostic. The audited CEF callback already marshals into the existing
  Skyrim update runner; the native implementation was nevertheless forcing the
  separate `fullReset` content-reset flag in addition to `resetGame`. The
  corrected path records the action, returns from its initiating handler,
  dispatches exactly once during the service update, and leaves `fullReset`
  untouched. Bounded Angular, bridge, dispatch, engine-request, transport, menu,
  UI-close, and gate diagnostics now identify the last completed boundary. The
  corrected rerun reaches a responsive Main Menu without CTD or zombie gameplay
  gate, releases the gate only at `MainMenuEntered`, retains the durable binding,
  and then re-enters the existing #56 rehydration through Continue/Resume to
  resume the campaign successfully. The disconnect incident UX is therefore
  live validated on both branches. No console command, fake New Game, arbitrary
  load, Papyrus workaround, protocol, or persistence mutation was added. The
  existing protocol also does not project the server-only
  `NO_COMMITTED_CHECKPOINT` diagnostic to CEF, so that case remains locked with
  its technical reason in server logs;
- the Main Menu cannot host this UI today: the overlay and `UiSurfaceService`
  require a real `PlayerCharacter` plus NiNode, and Angular is mounted only for
  the in-game projection. No fake player, alternate New Game flow, or new engine
  hook was added. The production entry is therefore Skyrim's native load of a
  marked checkpoint followed by the earliest engine-safe post-load CEF surface;
- the existing `CampaignResumeRequest` gained only a
  `RestoreCommittedCheckpoint` intent bit. The server retains a transient
  per-campaign intent while the sealed roster is incomplete. Exact admission of
  the final member first yields authoritative `ACTIVE`, then opens the existing
  durable #56 recovery from `ACTIVE`; it never creates `BeginRecovery` from
  `WAITING_FOR_ROSTER`. An already-open recovery and its `RestoreAttemptId` are
  reused;
- the resume-required gate hands off to the correlated recovery without opening.
  Every client then reloads its own authoritative `LastCommittedCheckpoint`
  artifact through #56, applies the canonical restored snapshot, crosses both
  barriers, and releases only on matching `CampaignRecoveryComplete`. Generic
  `ACTIVE` cannot bypass this lock;
- no schema migration, new canonical persistence, new checkpoint/recovery state
  machine, host authority, console command, or `/stre-campaign-resume` trigger
  was added. The complete campaign Playwright slice covers the shared
  Create/Join/Resume shell, explicit multi-candidate and duplicate selection,
  exact marked-save targeting, mandatory/no-solo presentation, connection,
  waiting roster, restore/synchronization, errors/retry, F2 state retention, and
  absence of local `ACTIVE`. The focused Resume Playwright slice passes all 14
  cases, including terminal close, no automatic OrdinaryResume fallback, and
  later explicit ordinary reopening. Native tests cover metadata restart/corruption,
  exact candidate matching, idempotent selected-campaign retry, gate handoff,
  protocol intent, and full-roster entry into the existing recovery. See
  [`CAMPAIGN_LOAD_CAMPAIGN.md`](../development/CAMPAIGN_LOAD_CAMPAIGN.md).

The client-side player-load fence is implemented, automated-tested, and live
validated for cold marked Manual load and Main Menu Continue. Its common
`Load_Impl` policy and null-target `LoadMostRecentSaveGame` transport remain the
final safety boundaries:

- one pure policy allows only the exact active #56 native-load correlation,
  blocks every player load while authoritative admission or the campaign gate
  exists, routes a cold valid-marker target to ResumeRequired, preserves
  ordinary vanilla loads outside campaign, and blocks reserved-but-unproven
  targets without trusting their filename;
- actionable F9 is consumed at the proven QuickLoad handler when that same
  policy sees a campaign-sensitive runtime, preventing Skyrim's misleading
  corrupt-save dialog. Cold F9 continues to the common load boundary;
- the candidate `LoadMostRecentSaveGame` adapter (AE ID `35766`) observes the
  public CommonLib `saveGameList` front entry and scopes an owned target only
  across its call. Audit of a reproducible Show All Saves crash found no STRE
  write, retained entry/string pointer, relocation overlap, or load-policy
  frame. The AE 1.6.1170 dump faults in Scaleform `ObjectAddRef` (ID `82269`):
  the best-resolved native `CharacterSelected` callback (ID `52919`) receives
  one argument from the active 2017 SkyUI `quest_journal.swf`, then reads and
  copies a nonexistent fourth argument required by the expanded AE Journal
  contract. `SkyUI_SE.esp` is active and its BSA overrides the materially newer
  vanilla 1.6.1170 movie. The crash is therefore an installed Journal UI
  compatibility failure, not checkpoint naming/metadata or the ID `35766`
  detour. The adapter remains enabled. No device ID, timeout, UI label, or
  prefix supplies authority;
- subsequent AE 1.6.1170 live runs proved that Main Menu `Continue` bypasses
  the Manual `UISaveLoadManager.LoadGameCallback`, the candidate
  `LoadMostRecentSaveGame` adapter, and the currently hooked `Load_Impl` path:
  the semantic `FxDelegate::Callback("ContinueLastSavedGame")` had one raw GFx
  argument but no readable save list at callback time, then enqueued native
  operations `0xD0000100`, `0xD0000010`, and `0xD0002000` before closing Main
  Menu. CommonLib's public callback contract proves that raw `args[0]` is the
  response ID and user payload starts at `args[1]`; `argumentCount=1` therefore
  carries no target/index payload. Runtime disassembly of AE ID `35772` and its
  exact request RTTI now proves that `0xD0000100` is a base 0x18-byte `Request`
  which invokes the manager callback at `+0x240`, not a save-identity carrier.
  Its distinct `0xD0000010` branch consumes a 0x28-byte `LoadRequest`; the
  derived payload at `+0x18` points to the native source whose filename at
  `+0xBB0` is read by exact consumer ID `442580` before native load work. A
  bounded trace tags only a successful exact `0xD0000100` pointer pushed
  inside the `ContinueLastSavedGame` call stack, propagates that root only to
  exact requests pushed during its ID `35772` dispatch, distinguishes deferred
  requeues, identifies request classes by public vtables, and records the
  canonical `LoadRequest` target plus relevant manager mutations before/after
  dispatch. The focused rerun live-proved one exact root pointer producing one
  direct typed `LoadRequest` child with the same lineage and canonical target
  `stre-checkpoint-<id>.ess` both when pushed and immediately before dispatch.
  Native Continue target resolution and functional interception are therefore
  **LIVE VALIDATED** at exact consumer ID `442580`: only the first direct typed
  child is claimed, its `.ess`
  filename is normalized once to the existing extensionless
  `NativeSaveIdentity`, and the common player `CampaignLoadPolicy` runs exactly
  once. Ordinary targets call the original consumer once; marked targets create
  logical ResumeRequired pending ownership with the exact save identity before
  calling that same native consumer, but do not arm the runtime gate, input
  lock, or guard menu while Main Menu remains open. A successful native return
  makes the pending transition eligible, and only the semantic
  `UI.MenuOpenCloseEvent MainMenuClosed` boundary commits the existing gate
  directly to its post-load ResumeRequired state exactly once. A rejected
  native request, replacement attempt, or failed commit clears pending state.
  Blocked or unproved targets skip the consumer and reuse the public Main Menu
  rebuild. Request correlation is cleared before native dispatch so child
  requests and requeues cannot duplicate ResumeRequired. Uncorrelated requests—including
  exact #56 internal loads—bypass this seam. No device ID, time window, list
  endpoint, filesystem ordering, opcode-only authority, special Continue
  policy, protocol, persistence, or server state was added;
- bounded `[STRE][CampaignLoadTrace]` records cover QuickLoad, Main/Journal
  context, `LoadMostRecentSaveGame`, exact `Load_Impl` arguments and policy,
  native return, and `TESLoadGameEvent` owner routing. A live Manual-load run
  proved that final `Load_Impl` rejection occurs after Skyrim has already
  committed its Journal fade, leaving a black screen. Runtime disassembly then
  proved the earlier semantic seam: AE 1.6.1170
  `UISaveLoadManager::Accept` registers literal `LoadGame` on adapter ID
  `52914`, adjacent to the already proven `SaveGame` ID `52915`. The callback
  carries the selected save-list index and creates the native operation only
  when forwarded. STRE now resolves an owned target copy, evaluates the same
  policy, and consumes blocked player decisions before that operation exists.
  The first rerun proved no fade, `Load_Impl`, `TESLoadGameEvent`, or campaign
  gate lock, but also proved that a bare return leaves the Journal
  non-interactive. Audit of the SkyUI `SystemPage` contract explains why: the
  SWF sets `disableSelection` and `bMenuClosing` before calling the native
  `void` callback, which has no supported cancel/failure response. The blocked
  projection now queues the normal `Journal Menu` `UIMessage::kHide` message
  and emits a localized notification; it does not mutate private SWF state,
  repair a fade, use a timer, or partially call the original. It keeps no
  callback provenance or selection state. Cold marked loads and vanilla
  outside-campaign loads still forward to the final boundary; `Load_Impl`
  remains the safety enforcement point for bypasses and the owner of
  ResumeRequired arming. The second live rerun confirmed
  `Consumed -> JournalCloseRequested -> JournalClosed`, no fade, no
  `Load_Impl`, no `TESLoadGameEvent`, no campaign-gate lock, and the localized
  in-game notification. CampaignLoadPolicy was unchanged;
- the shared `LoadGame` callback now projects blocked UX according to its public
  owning menu. Journal keeps the live-validated `kHide` behavior. A defensive
  Main Menu block queues normal `kHide` then `kShow` messages for `Main Menu`,
  replacing the disabled/busy SWF instance rather than attempting to close a
  nonexistent Journal. There are no private SWF flags, offsets, timers, click
  simulation, fade repair, or partial native callback. Nominal same-process
  marked load now sees no stale admission and evaluates to
  `BeginResumeRequired`; the fallback exists only for stale/edge state. That
  defensive fallback remains outside the nominal Continue validation and still
  awaits a dedicated live usability rerun;
- the current full suite passes `TPTests` at 3,786 assertions in 261 test cases
  plus `SkyrimTogetherClient` and its Angular production pre-build, including
  the Main Menu Continue target-resolution trace and functional interception.
  The client and Angular production builds pass. Live cold marked Manual load
  and Main Menu Continue both enter ResumeRequired, and Continue/Resume has
  completed the existing #56 rehydration path. Show All Saves must still be
  rerun with an AE 1.6.1170-compatible `quest_journal.swf`; ordinary vanilla and
  cold F9 remain in the unvalidated live matrix. The exact #56 internal recovery
  path is live validated with two clients, including successive recovery and
  durable restart rehydration.

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
this focused slice does not complete it. `CHECKPOINTING` is now active for the
#55 Candidate lifecycle and fences unrelated durable mutations per campaign.
`RECOVERY_LOCK` and `RESTORING_CHECKPOINT` are active for #56 sealed-roster
disconnect and two-barrier collective checkpoint restore. This path is
automated/build-tested and live validated with two clients across nominal,
successive, and restart-rehydrated recovery.

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
successful test. That historical spike itself added no production campaign
save detection. Production now uses the retained gate for marked checkpoint
loads: #55 owns the save plus local non-authoritative marker, the Load Campaign
surface owns the resume-required lock, and #56 owns authoritative collective
restore and release.

### Campaign native-save completion spike

The bounded #55 native-save spikes have progressed through three evidence
levels:

- one canonical `CheckpointId -> stre-<CheckpointId>` transformation reuses the
  existing bounded campaign ID validator and rejects path syntax;
- human validation rejected v1: the direct game-update
  `BGSSaveLoadManager::Save_Impl(2, 0, name)` call froze Skyrim, left a
  zero-byte `.ess.tmp`, produced no final `.ess`, and never reached its
  post-call log;
- v2 accepts one owned save intent on STRE's game-update path, returns without
  calling `Save_Impl`, and consumes the intent after the original Skyrim
  save/load process function at Address Library ID `35772`;
- the v2 separation matches the audited SKSE request/process architecture
  without treating SKSE's `RequestSave` abstraction as Bethesda-native;
- v2 is human-validated on AE 1.6.1170 with SKSE 2.2.6: request and processing
  ran on different threads, Skyrim remained responsive, `Save_Impl` returned
  true, `.ess` plus `.skse` were produced, and the `.ess` reloaded successfully;
- v3 is implemented, automated/build-tested, and human runtime-validated through
  the production #55 two-PC flow on 24 August 2026: Skyrim's ID `109278` resolves
  the profile-aware local save path, and a bounded off-thread observer declares
  completion only after a fresh `.ess`/`.skse` bundle has no `.ess.tmp`, both
  members are simultaneously open without write/delete sharing, and all bytes
  have been SHA-256 hashed while those handles remain open;
- the deterministic, path-independent metadata codec v1 records the logical
  identity plus canonical `ess`/`skse` roles, sizes, and per-member hashes; the
  bundle fingerprint is SHA-256 of those exact metadata bytes and fits the
  existing checkpoint persistence fields without a schema change.

The production run kept Skyrim responsive, produced both required files, matched
STRE's per-member hashes against independent PowerShell `Get-FileHash` results,
and successfully loaded the generated save in Skyrim. `SAVE_CALL_RETURN` itself
remains explicitly untrusted as completion proof. Live failure, disconnect, and
exact replay resilience are tracked by
[#72](https://github.com/matthieuAlbertelli/Skyrim-Together-Reborn-Enhanced/issues/72).
Recovery is implemented separately by #56; retention, cleanup, and upload remain
unimplemented. See
[`CAMPAIGN_NATIVE_SAVE_SPIKE.md`](../development/CAMPAIGN_NATIVE_SAVE_SPIKE.md).

### Programmatic campaign native-load Slice 0

The #56 Slice 0 native-load primitive is implemented, Windows build-tested,
and human runtime-validated on 25 August 2026. It retains a production-capable
client primitive from an exact cached `stre-<CheckpointId>` artifact through the
existing `CampaignNativeSave` reopen/hash proof, `RunnerService` game-update
execution, the existing `BGSSaveLoadManager::Load_Impl` hook,
`TESLoadGameEvent`, the #60
runtime gate/menu, and a connected transport update. Success requires every
milestone independently; the native Boolean is not sufficient. One request is
single-flight until explicit terminal release, and ordinary unarmed loads are
not managed.

TPTests pass 2277 assertions in 168 test cases; `TPProcess` and
`SkyrimTogetherClient`, including the production Angular UI, build. After human
validation, the temporary `/stre-campaign-resume`, `/stre-native-load`, and
`/stre-native-load-release` commands and all corresponding Angular, CEF V8, and
OverlayClient wiring were removed. They are not production-facing UX, and no
replacement debug or console command was added. The retained native-load
service has no player-accessible trigger; #56 now invokes it only from the
production recovery protocol. Recovery authority remains on the persistent
server.

The validated cold-session run used campaign
`campaign-367760f49cba23fd72a5ad5013a75e1b`, checkpoint
`checkpoint-4a33f050b434778db8b09094658831d5`, and native identity
`stre-checkpoint-4a33f050b434778db8b09094658831d5`. Before its removal, the
temporary harness sent the existing Resume request and admission was accepted
only by the authoritative server response at revision 7 (`operation=2`); no
local admission was synthesized. The exact load then passed artifact
validation, entered the existing `Load_Impl` hook, returned true, observed
`TESLoadGameEvent`, locked
the campaign gate, kept transport connected, displayed the guard menu with
`UI::GameIsPaused() == true`, and reached `COMPLETED`. The expected checkpoint
visibly loaded while gameplay froze and F2/CEF remained responsive.

A duplicate request while terminal and locked was rejected as
`request-not-idle` without another invocation. Explicit release destroyed the
guard with `gateRemainsLocked=false` and immediately restored gameplay. Before
and after evidence was identical: `.ess` length 2,600,863, timestamp
`2026-08-24 18:01:39`, SHA-256
`8AC74662C3AC18F599C36690253907465326AD721B5BCE5D357176F0F83E6123`;
`.skse` length 2,789, the same timestamp, SHA-256
`3FC8EA1291BE750871F23094E93723BC964EDF3E7C7CFE20B45D4D51033403CF`;
no matching `.tmp` existed before or after. A subsequent vanilla/manual load
remained unmanaged and acquired no STRE gate.

This proves the deterministic local primitive consumed by issue #56. The
production `CampaignRecoveryService`, `RestoreAttemptId` protocol, full-roster
rollback, `LoadedAndLocked` and `SnapshotApplied` barriers, canonical monotonic
server restore, restart reconstruction, and no-checkpoint diagnostics are now
implemented and automated/build-tested. The first two-client recovery run
reached the native-load boundary on both clients but did not complete its first
barrier because the already-open guard menu emitted no second `PostDisplay`.
That proof now uses observable menu-open plus paused state. A later cold
one-member marked-save run crossed #56 and released at restore revision 7. The
first two-client rerun after canonical recipient preparation then completed
both barriers and returned the campaign to `ACTIVE`. A new checkpoint used that
restore revision as its source, but the immediately following recovery failed
closed after durable restore with `reason=snapshot-unavailable`. Checkpoint
creation and restore now both rebase the canonical core payload to their exact
revision; deterministic consecutive N=1/N=2 and persistence-reload regressions
pass. A restart of the previously incomplete r14 attempt then live-proved that
fix through two-recipient `RESTORE_SNAPSHOT_SENT`, but the fresh client correctly
rejected the direct replay because no authoritative load request had rebuilt its
attempt/checkpoint correlation. Recovery rehydration now replays that exact
native-load barrier first. The fresh two-client rerun live-validated the same
durable attempt and restore revision end-to-end without a second durable
restore. See
[`CAMPAIGN_NATIVE_LOAD_SPIKE.md`](../development/CAMPAIGN_NATIVE_LOAD_SPIKE.md).

### Collective campaign recovery

The production issue #56 implementation has addressed the reviewed
crash/reconnect cases, is automated-tested plus Windows client/server
build-tested, and is live validated with two clients for nominal recovery, a
successive checkpoint B/recovery B cycle, and durable incomplete-recovery
rehydration across restart. The restart replay reused the exact persisted
attempt/checkpoint and existing restore revision without a second durable
`RestoreCheckpointSnapshot`:

- a sealed-roster disconnect durably appends `BeginRecovery` only from `ACTIVE`
  or `CHECKPOINTING`, abandons only an in-flight Candidate, projects
  `RECOVERY_LOCK`, and fences checkpoint creation plus unrelated durable
  mutations for that campaign. `WAITING_FOR_ROSTER` never starts a new attempt,
  while disconnects during an open recovery still replay its barrier;
- exact admission of the complete immutable roster resumes one deterministic
  `(CampaignId, RestoreAttemptId)` and selects only
  `LastCommittedCheckpoint`; an absent committed checkpoint reports
  `NO_COMMITTED_CHECKPOINT` and remains locked;
- every connection receives only its canonical slot/binding and exact local
  `.ess`/`.skse` artifact proof. The client locks Skyrim before load, reuses the
  validated native-load primitive, and cannot release on failure, timeout,
  reconnect, UI state, or a stale message;
- after `TESLoadGameEvent`, the first world update accepts safety proof only when
  `STRECampaignGateMenu` is observably open and `UI::GameIsPaused()` is true.
  This covers a guard menu that survives the load without another `PostDisplay`;
  an absent menu or unpaused game still fails closed;
- the first full-roster barrier requires every exact native load and artifact
  proof. Only then does SQLite restore the immutable shared snapshot at one new
  monotonic revision with durable source provenance;
- checkpoint creation re-encodes the authoritative runtime core at the
  checkpoint's exact `SourceRevision`, and restore re-encodes that exact state at
  its new `RestoreRevision` before updating current state and the transactional
  outbox. Restore-generated revisions are therefore normal canonical sources
  for the next checkpoint. A bounded journal-lineage reader exists only for
  checkpoints already written by the prior implementation and never selects
  unrelated or current snapshot data;
- restore-snapshot dispatch is now prepared all-or-nothing from the durable
  checkpoint roster. Every exact slot/player/binding must resolve to one current
  admitted connection and live server player before any member is sent the
  snapshot. Reconnect-generated transient IDs are accepted only through that
  canonical mapping; missing, duplicate, unexpected, or stale recipients leave
  recovery fenced and replayable with an explicit required/resolved diagnostic;
- the second full-roster barrier requires every client to apply that exact
  correlated snapshot. `CompleteRecovery` is a durable accepted no-op marker;
  only its matching server completion message releases client gates and returns
  the campaign to `ACTIVE`;
- the same generic barriers cover a sealed one-member roster without a special
  solo branch: its sole exact Loaded ACK completes the first barrier and its
  sole exact Applied ACK completes the second;
- stable restore/completion mutation IDs and journal reconstruction resume the
  correct barrier after server restart without a duplicate restore revision;
  duplicate packets are idempotent and mismatched identity, checkpoint,
  attempt, revision, roster, or artifact evidence fails closed;
- exact per-slot Loaded and Applied receipts are stable accepted no-op journal
  mutations. They preserve partial barrier/idempotency evidence across restart
  without advancing canonical state, changing schema, or adding protocol;
- after server restart, an incomplete attempt always replays its authoritative
  native-load request before any restored snapshot, including when the restore
  is already durable. Fresh clients thereby acquire exact correlation and
  re-prove the local load; survivors resend. Both volatile full-roster barriers
  must still be rebuilt before snapshot/completion, and the existing restore
  revision is reused rather than applied again;
- a client lost during snapshot application must replay its native checkpoint
  load before receiving that snapshot. Survivors resend their load proof, and
  the same attempt plus existing durable restore revision are reused;
- if completion is durable before a server crash, a still-correlated client
  replays `SnapshotApplied` and receives the idempotent completion message. An
  authoritative `ACTIVE` snapshot releases only an uncorrelated provisional
  transport lock and cannot bypass a correlated recovery;
- serialized checkpoint/disconnect ordering is explicit: a commit that wins
  first may become the rollback point; a disconnect that wins first leaves the
  Candidate uncommitted and late ACKs cannot replace the prior committed point;
- no schema migration, host authority, partial-roster continuation, late join,
  player replacement, quest-stage reconstruction, native-save upload, cleanup,
  retention, or new player/debug trigger was added.

Automated coverage includes runtime recovery and restart behavior, both
barriers, mutation fencing, no-checkpoint failure, duplicate/stale messages,
strict protocol validation, client correlation, fail-closed local gating,
checkpoint/disconnect ordering, the Load Campaign entry, and multiplayer save
policy. It now includes a sealed one-member recovery from `ACTIVE` through
disconnect, reconnect, `RESTORING_CHECKPOINT`, Loaded, restore, Applied, and
back to `ACTIVE`; immediate N=1 barriers, duplicate/stale ACKs, reconnect during
an attempt, and no release before authoritative completion. It additionally
covers the two-member Loaded 1/2 then 2/2 barrier, exact two-recipient snapshot
preparation after both members reconnect under new transient IDs,
missing-recipient fail-closed behavior, restore replay without a second durable
revision, and completion only at Applied 2/2. It now also executes three
consecutive checkpoint/recovery cycles for N=1 and N=2, a second cycle across
persistence reload, corrupt-snapshot fail-closed behavior, exact two-recipient
preparation on the second restore-generated checkpoint, and idempotency
conflict for an altered restore payload. It now also covers N=2 restart while
Loaded is 1/2, restart while Applied is 1/2, durable per-slot receipt
reconstruction, current-session barrier reproval, fresh-client correlation
establishment before snapshot acceptance, and one restore revision throughout.
`TPTests` passes 3,786 assertions in 261 test cases;
`SkyrimTogetherClient` (including Angular production) and
`SkyrimTogetherServer` build in the Windows
development environment. The first real two-client run locked both clients,
restored both native checkpoints, observed `TESLoadGameEvent`,
`LockedAfterLoad`, `GameIsPaused=true`, and connected transport, but timed out
because the already-open guard menu emitted no second `PostDisplay`. The proof
now consumes the observable first-world-update state instead. The cold
one-member ResumeRequired flow subsequently completed authoritatively and
released the gate. The recipient correction then live-proved a complete fresh
N=2 recovery (`restoreRevision=9`, dispatch 2/2, Applied 2/2, durable
completion). After gameplay created a checkpoint with `sourceRevision=9`, the
next N=2 recovery reached `restoreRevision=15` and resolved 2/2 recipients, but
failed with `reason=snapshot-unavailable`. After the canonical payload fix and
a server/client restart, that same durable r14 attempt resolved both recipients
and emitted `RESTORE_SNAPSHOT_SENT`; the fresh client then rejected it with
empty expected attempt/checkpoint correlation because no load request had been
replayed first.

Audit isolated this second blocker to revision ownership, not dispatch: the
first restore copied the checkpoint's opaque core payload still encoded at its
older source revision, then labeled current state as revision 9. The next
checkpoint recorded source revision 9 around that stale payload; after the
second restore, `LoadCampaign` rejected the payload/revision mismatch, so
`BuildSnapshot` returned unavailable. The corrected canonical path normalizes
the core payload when creating an immutable checkpoint snapshot and again when
materializing a restore revision. Diagnostics retain recipient counts and now
distinguish checkpoint source revision, restore revision, checkpoint payload
presence, and runtime canonical snapshot presence. No schema migration,
alternate snapshot authority, arbitrary revision fallback, or protocol was
added. The recovery rehydration correction now journals exact slot receipts as
accepted no-ops, reconstructs partial phases, and replays the native-load
barrier before any snapshot after restart. Diagnostics report persisted phase,
replay action, exact correlation/revision, durable receipt sets, volatile
barrier counts, and whether restore is already durable. The repeated
two-client live rerun completed both barriers and authoritative completion
using that exact replay order. Issue #56's collective recovery contract is
therefore live validated; the disconnect incident UI described below is a
separate presentation improvement and does not reopen its protocol or
persistence status. See
[`CAMPAIGN_COLLECTIVE_RECOVERY.md`](../development/CAMPAIGN_COLLECTIVE_RECOVERY.md).

### Coordinated campaign checkpoints

The production issue #55 checkpoint protocol and managed native-save path are
automated-tested and build-tested. The player-facing Manual NewSlot and Quick
origin transports are implemented from live AE 1.6.1170 evidence and are now
live validated end-to-end. Manual ExistingSlot overwrite and Auto provenance
remain explicitly unproved and fail closed:

- the server owns `CheckpointId`, derives `stre-<CheckpointId>`, creates the
  exact SQLite Candidate snapshot/source revision, and publishes transient
  per-campaign `CHECKPOINTING` only for a sealed, fully admitted roster;
- the production `Save_Impl` boundary has the exact audited CommonLibSSE-NG
  order `(self, int32 deviceId, uint32 outputStats, const char* fileName)` across
  its original pointer, detour, trampoline, and #55 internal caller. An
  automated sentinel test guards against the different `Load_Impl` order;
- readable `Save*`, `QuickSave*`, and `AutoSave*` families still map to the
  intended Manual/Quick/Auto rules, and null/unreadable/empty/other names remain
  Unknown fail-closed. No `Unknown -> Manual` fallback exists;
- the first live CampaignSavePolicy run reached that correctly typed hook with
  `fileName == nullptr` for both Manual Save and QuickSave. Consequently those
  attempts classified Unknown and were blocked; the apparently blocked
  autosave is not evidence of Auto classification either. Production-safe
  diagnostics now include `deviceId`, `outputStats`, pointer present/null, the
  bounded name only when safely readable, and the resulting classification;
- the ordered observation run proved that broad `Quicksave` input dispatch is
  repeated and cross-thread, while the exact Quick handler accepts only the
  first non-zero/non-held button event. That path creates a 24-byte native
  request through ID `35769`, queues operation `0xF0000200`, and ID `35772`
  consumes it before `Save_Impl(4, 0, nullptr)`. The request pointer is the only
  bounded native correlation; it is not a stable ID. Production tags only the
  exact request created under the actionable handler, carries that pointer
  through successful push/pop/requeue, arms Quick only on its final correlated
  pop, and consumes it once in `Save_Impl`. Failed pushes, mismatched requests,
  dropped/coalesced requests, and process-boundary exit without `Save_Impl`
  clear the proof. No device ID, delay, or input-event count is consulted;
- the same run invalidated `MenuControls::NewSave` for Manual Save. Journal
  open/close remains context only. The exact Scaleform `SaveGame` callback at
  IDs `52915`/`52923` is the confirmed operation seam: new-slot selection calls
  `Save_Impl(2, 0, nullptr)` directly and synchronously; overwrite resolves the
  selected save entry first. Only live-proven `NewSlot` now carries a scoped,
  one-shot thread-local Manual provenance around that synchronous callback.
  `ExistingSlot` remains instrumented and fail-closed pending live proof;
- Auto is still unproved. Several static producers enqueue native operation
  `0xF0000040`, and its process branch calls `Save_Impl(3, 0, nullptr)`, but
  neither that code nor `deviceId=3` is accepted as an Auto mapping. A
  deterministic Save-on-Wait live trace is still required. Auto is never
  inferred by exclusion;
- the server derives campaign/member authority from the admitted connection,
  accepts a new player intent only with the authoritative full roster in
  `ACTIVE`, and owns all checkpoint IDs/revisions. Simultaneous intent reuses
  the same open activity and creates exactly one Candidate;
- the #55 internal native call uses scoped thread-local provenance through the
  same hook. The `stre-` filename itself grants no bypass, preventing recursive
  checkpoint requests without trusting a caller-controlled name;
- one runtime mutation fence rejects unrelated durable mutations for that
  campaign while leaving other campaigns independent;
- one bounded server-to-client save request and one bounded client-to-server
  result carry only campaign/checkpoint/native identity and the canonical
  artifact. Player, slot, binding, paths, expected revision, and client
  `MutationId` are absent;
- admission derives the exact slot/player/binding tuple from the connection,
  validates the fixed SHA-256/codec-v1 artifact, records the ACK through the
  existing store primitive, and commits only after every Candidate slot is
  complete;
- stable server mutation IDs cover Candidate creation, each canonical slot ACK,
  and commit. ACK replay recovers its original expected revision from the
  durable journal rather than duplicating a revision ledger in memory;
- the small client checkpoint service persists the completed bundle artifact
  atomically before ACK. An exact replay reopens and hashes the existing
  `.ess`/`.skse` against that cache without invoking Skyrim Save or overwriting
  files; malformed or conflicting evidence fails closed;
- localized requested/committed/failed and blocked/unavailable outcomes use the
  existing system-message surface. While in campaign, STR Settings projects
  Skyrim's four autosave preference families (rest, wait, travel, character
  menu) as disabled/unchecked secondary information; this never modifies the
  player's persisted Skyrim preferences. The real Skyrim AE 1.6.1170 Gameplay
  rows remain visually and interactively vanilla. Audit of `Journal Menu`,
  `quest_journal.swf`, `SystemPage`, `OptionsList`, and `SettingsOptionItem`
  found no per-row disabled/help contract and no typed STRE GFx seam; supporting
  it would require a replacement SWF or fragile private ActionScript/native
  hooks. This is a known UX limitation, while the independent native save hook
  remains fail-closed authority;
- explicit server console commands `stre_checkpoint <CampaignId>` and
  `stre_checkpoint_resend <CampaignId>` provide deterministic validation and
  logical lost-ACK replay. There is no scheduler or checkpoint cadence;
- client failure abandons only the transient activity. A disconnect also opens
  #56 `RECOVERY_LOCK` for a sealed campaign. The Candidate and all saves remain
  and the previous committed checkpoint remains selected;
- crash-before-commit does not resume unfinished Candidates; crash-after-commit
  resolves the new `LastCommittedCheckpoint`. No save/Candidate cleanup,
  retention, pruning, deletion, upload, or recovery was added.

Automated coverage includes strict intent/outcome wire failures, the expected
native filename families, exact native boundary order, exact Quick pointer
correlation across threads/requeues, non-actionable/failed/dropped cleanup,
one-shot Manual NewSlot scope, vanilla non-campaign behavior, every campaign
runtime state, Auto/Unknown fail-closed handling, untrusted `stre-*` filenames,
explicit internal recursion provenance, one-Candidate duplicate intent,
conditional UI settings projection,
client cache restart/conflict, authority spoof rejection, duplicate/reversed
ACKs, mutation fencing, independent campaigns, failure/disconnect preservation,
and exact 2/4/10-slot commits. TPTests passes 2,906 assertions in 218 test
cases; the complete Campaign Playwright slice passes 18 tests, including the 2
save-policy cases; and the Windows `SkyrimTogetherClient` (including Angular production) and
`SkyrimTogetherServer` builds pass. These UI tests cover only the secondary STR
Settings projection, not disabled rows in Skyrim's native Gameplay menu. The
upstream Quick and Manual/NewSlot seams and their causal properties were
exercised live, and the resulting production provenance transports were then
validated end-to-end in Skyrim for Quick and Manual NewSlot. Auto and Manual
ExistingSlot overwrite remain explicitly unproved and fail-closed.

On 24 August 2026, the nominal #55 path was validated end-to-end with two real
Skyrim clients in sealed campaign
`campaign-367760f49cba23fd72a5ad5013a75e1b`. Checkpoint
`checkpoint-4a33f050b434778db8b09094658831d5` captured source revision 3 as a
Candidate at revision 4. Slot 1 was accepted at revision 5 with
`committed=false`; slot 2 was accepted at revision 7 with `committed=true`, then
the server emitted `CHECKPOINT_COMMITTED` at revision 7. Both clients produced
their own `.ess`/`.skse` bundle under the shared logical identity, and each
server-accepted bundle fingerprint matched its originating client. This proves
the nominal full-roster commit barrier in live multiplayer without granting host
save authority. The two fingerprints differ by design because the native
payloads are per-player.

Live client failure/disconnect, lost-ACK exact replay and no-overwrite behavior,
and server interruption around the commit boundary remain tracked by
[#72](https://github.com/matthieuAlbertelli/Skyrim-Together-Reborn-Enhanced/issues/72);
they are not claimed as completed runtime validation here. See
[`CAMPAIGN_COORDINATED_CHECKPOINTS.md`](../development/CAMPAIGN_COORDINATED_CHECKPOINTS.md).

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
