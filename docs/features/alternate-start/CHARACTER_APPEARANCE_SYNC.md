# Character appearance synchronization

Implementation and validation truth is in [STATUS.md](../../project/STATUS.md).
This contract concerns initial Character Creation on Steam Skyrim 1.6.1170, with
SKSE64 2.2.6 as the native reference. It does not change classes, loadouts,
camera or campaign recovery authority. ADR-0022 makes entry and final
rematerialization independent of posture; collective seating is a future phase.
ADR-0023 makes the official final flow automatic in every build, including MASTER.

## Runtime checkpoint and remaining validation

The 2026-09-19 maintainer checkpoint accepts final appearance in both directions,
individual local MarkerXX -> SeatXX seating and visible remote seating after
binding replay, in both roster-2 completion orders. Weapon 4/5 remains unsafe:
the bounded pre-reservation wait proceeds only after all admission guards pass.
The log-backed A-first run shows weapon 4 -> 0, one candidate-commit and refined
IdleChairEnterInstant on the new Actor/token. STATUS owns timestamps, the human
attestation for the other order, binary identity and the limits of this evidence.
No remote Activate/MoveTo or all-Applied barrier participates in this contract.

Earlier checkpoints:

The maintainer also confirms automatic final rematerialization without Ctrl+F11
on the tested multiplayer creation run (2026-09-18 commit handoff). This is bounded
human acceptance of default activation, not full matrix or native lifetime proof.

Character Creation live appearance sync is superseded; final local natural-join
rematerialization is implemented. The maintainer has human-validated observer A
seeing B's final appearance in one successful 2026-09-18 transaction. Exact tested
identities, trace sequence and the validation boundary belong in
[STATUS](../../project/STATUS.md#first-final-rematerialization-runtime-checkpoint-2026-09-18).
This does not validate every race/sex, repeated cycles, recovery/reconnect,
native retirement completion, production readiness or future collective seating.
Registry presence at +30 seconds proves neither a leak nor complete destruction.
Race-agnostic implementation remains a contract, not a completed test matrix.

## Accepted Character Creation strategy - natural-join correction, 2026-09-18

[ADR-0020](../../architecture/ADRs/ADR-0020-character-creation-final-local-materialization.md)
supersedes the hot-appearance strategy below for initial STRE Character Creation.
[ADR-0021](../../architecture/ADRs/ADR-0021-natural-join-final-representation-standing-creation.md)
adds race-agnostic natural-join projection.
[ADR-0022](../../architecture/ADRs/ADR-0022-posture-independent-character-creation.md)
supersedes its standing eligibility requirement without changing the materializer.
[ADR-0023](../../architecture/ADRs/ADR-0023-automatic-final-character-creation-rematerialization.md)
supersedes the debug activation gate and non-MASTER functional restriction.

Final Character Creation rematerialization is race-agnostic. It recreates the
remote as a natural join representation. No live appearance synchronization
occurs during Character Creation. Sitting, standing and furniture state neither
block entry nor final rematerialization. Nord, Orc, Khajiit and Argonian use exactly the same
materializer/projection. No humanoid allowlist, beast exclusion or transition
classifier participates in LAB eligibility. Invalid/unresolvable canonical
records and unsafe lifecycle states still reject.

- RaceMenu in progress and intermediate closes: NO live remote appearance sync.
- After authoritative build sealing: ONE final canonical
  AppearanceBuffer/ChangeFlags/FaceTints snapshot for the entire creation.
- Observers: ONE automatic local natural-join rematerialization, including unchanged appearance,
  cosmetics only, same race/sex, sex only, race only and combined changes.
- Post-Character-Creation editing, including later manual showracemenu: OUT OF SCOPE.

The maintainer explicitly selected build sealing as the final boundary. ModifyRace
remains available beforehand. Only the matching local pending Applied notification
publishes; the new final is qualified by FinalBuildRevision. The server checks
owner, Applied revision and race, freezes the first canonical final, and relays
identical retries. Schema 2 requires matching client/server builds. Race/sex/weight
descriptors are validation metadata; the materializer receives only canonical
native appearance bytes/flags/tints, never a sequence of visual deltas.

This is the normal initial Character Creation behavior in MASTER and non-MASTER.
No user/debug activation is required. Ctrl+F11 and the Final Character Creation
local respawn LAB menu item no longer control or enable it; that toggle is removed.
Service startup logs `phase=final-rematerialization-enabled
source=official-character-creation-default`. The internal RemoteRespawnLab name
and log prefix remain for trace continuity. F11/Shift+F11 still control only the
separate non-MASTER passive probes. Rejection never falls back to legacy hot apply.
Solo sends no snapshot and creates no remote representation.

Only the observer's native Actor/private TESNPC pair changes. serverId, PlayerId,
ownership, sealed roster and versioned ECS entity remain unchanged. The shared
Slice 1 materializer creates the candidate at the current remote's position,
rotation/cell/worldspace from the initial native Spawn. RemoteActorProjection is
shared with both natural creation callers and WaitingFor3D completion: native
remote/player flags, actor values, inventory/equipment, factions, animation
variables, and Actor/root readiness. Existing network interpolation/action
baselines stay attached to the same ECS entity, now targeting its new native
binding. They are not reset as for a newly arriving logical player. The candidate
uses the canonical sealed inventory and latest received animation variables.
Descriptor race/sex validate the native result; existing headpart entries must
resolve, but neither headpart count nor face/head presence defines a race domain.
Slice 2A's pure lifecycle fences it by
session/entity/server/generation. Source-time Discovery routing quarantines staging
and retirement before any legacy subscriber. Fresh binding evidence, not the
clearable RemotePlayerAppearanceBaseComponent, supplies durable transaction identity.
Candidate-local FaceGen is published only at commit. Geometry and canonical live
inventory counts/equipment must pass; old retirement uses the existing Actor Delete.
No SwitchRace, Reset3D, live Actor Deserialize, manual TESNPC free or server respawn.

Transaction records are bounded to ten server identities and retained as tombstones through
disconnect. An attempted final cannot be reset by a debug control. Restart client processes
between runs. Applied matching and pre-reservation admission share one ten-second
monotonic budget from the first valid final snapshot. Candidate readiness retains
its separate ten-second budget after reservation; abort
keeps a still-valid old binding. Recovery defers candidate cleanup while locked.
Additional non-MASTER retirement diagnostics observe thirty seconds; registry persistence is not leak
proof. Native rendering is human-validated only for the recorded scenario;
full state-projection/race/sex/lifetime coverage remains pending (see STATUS).

### Animation continuity at final binding publication

[ADR-0027](../../architecture/ADRs/ADR-0027-animation-replay-native-binding-continuity.md)
adds one value-only notification from final candidate-commit to AnimationSystem.
The animation component retains up to 32 consumed actions using the shared STR
server replay cache. Its refined history is inserted before the existing pending
queue on the new native binding, once per session/generation. Pending actions are
not duplicated; newer exits invalidate older persistent history. The existing
AnimationSystem executes the resulting ActionReplayChain, including shared instant
counterparts, after its normal tick/graph checks. Recovery/disconnect invalidate
this transient history. No appearance admission, weapon policy, transaction,
native materializer, FaceGen, retirement or network behavior is changed. Human
pose acceptance remains separate from a successful ForceAction return.

Both CK entry fragments use the existing non-furniture marker and validate the
player, cell and position before stage 20, without GetSitState. After lobby authorization,
the native service places each player behind one of the ten existing CK anchors,
selected temporarily by sorted stable PlayerIds in the sealed roster (Solo uses
index zero). SlotId validity/uniqueness does not participate in this placement;
campaign SlotId semantics elsewhere remain unchanged. Missing/duplicate PlayerIds
and a local durable player absent from the sealed roster reject.
Placement does not activate furniture, wait for WantToStand, force an exit or
write ActorState. It validates current cell and finite position within 32 units of
the target. The player-specific native MoveTo is deferred: one call followed by
real cell/position polling with a five-second monotonic timeout, no fixed sleep.
Target rotation is applied after arrival. Detailed standing-move-observation logs
separate pending movement, arrival and exact rejection reasons.
The historical standing-position log label is retained, with no
posture implication. Distinct coordinates are source-tested; physical clearance
still needs human validation. No seating phase is implemented.
The LAB ignores all sit/sleep field values and furniture Interaction extra data
at capture and pre/post-commit validation. Alive/enabled, combat/mount and other
independent actor-state safety, binding, assignment, cell and lifecycle guards
remain. No seat state is repaired or restored; post-creation collective seating
is a separate future phase.

LAB current-binding rejection is diagnostic, not a fallback or relaxed gate.
Capture logs one exact detail for every failed predicate, with versioned entity,
component IDs/presence, Actor/Base IDs/tokens, WaitingFor3D/Local/Assignment,
recovery state and raw/safe ActorState results from the tested observation.
Native fields use -1 when unavailable. Both existing GetParentCell and loaded
parentCell IDs are logged without changing the LAB's cell guard. A missing
Player/Remote may be located for logging only; it cannot be selected for a respawn.

### Bounded admission before reservation (2026-09-19)

The observer distinguishes Ready, Pending and Rejected on the official path,
including MASTER. WantToSheathe (weapon=4) and Sheathing (weapon=5) may remain
Pending only when every other binding, runtime, canonical-data and ActorState
guard passes. Neither state satisfies WeaponSheathed or authorizes replacement.
All ActorState predicates, including the post-reservation and precommit gates,
remain unchanged. Sit/sleep/furniture never participates.

Every eligible UpdateEvent resolves and validates the current Actor/private base
again, including guards that previously followed the ActorState veto: provenance
when present, alias collisions, cell/worldspace, root/components and canonical
race resolution. A failing independent guard rejects immediately. Pending pins
only session, versioned entity, server/player identities, FormIDs and allocation
tokens; a changed binding rejects. No borrowed native pointer or partial gameplay
capture survives between updates. Only Ready saves a fresh complete capture and
enters the existing single transaction. Before that point there is no reservation,
candidate, native mutation, retirement or remote seat activation. Remote seating
continues to require CommittedActor for the matching final revision.

Freeze the first canonical final and its FinalBuildRevision. Identical deliveries
cannot restart the deadline or a terminal job; a conflicting final or matching
Applied payload terminates admission. Applied matching and weapon settling use
the same ten seconds, not consecutive budgets. Disconnect/recovery cancels pending
admission without native cleanup; existing post-reservation cleanup rules remain.
The local MarkerXX -> SeatXX prototype and its non-MASTER scope are unchanged.

Trace `admission-wait-begin`, `admission-weapon-changed`, then `admission-ready`
or `admission-rejected` / `admission-expired`. Logs include identity/revision,
elapsed milliseconds from the first final, budget, binding guard results and
decoded state. At most 32 pending transition lines are emitted per job; terminal
evidence is unconditional and reports suppressed transitions. Ready followed by
gate-accepted, transaction-reserved and candidate-create-enter proves admission,
not commit or visual correctness. Successful 4 -> 0 or 4 -> 5 -> 0 can resume;
persistent 4/5 ends with `weapon-sheathing-timeout`, leaves the existing binding
and cannot be reported as a fixed placeholder. The upstream state producer and
resolution remain unknown until correlated runtime evidence establishes them.
No forced ActorState, animation, sheathing, protocol change or hot apply is used.

### LAB ActorState policy (1.6.1170 only)

`FinalRespawnActorStateSafe` now uses a read-only decoder and named predicates.
The field layout follows [CommonLibSSE-NG ActorState](https://raw.githubusercontent.com/CharmedBaryon/CommonLibSSE-NG/main/include/RE/A/ActorState.h);
life 9 is established separately by exact-build evidence below.
The supplied `01200041 / 00001008` decodes to life=9, knock=0, attack=0,
fly=0, weapon=0, recoil=0, stagger=false, sprint=false, swim=false.
Its only former veto was the life field; moving-back/walking, head tracking and
allow-flying bits are not active knock/attack/flight states and do not veto.

Life 9 is **DontMove**, established in the exact installed 1.6.1170 image,
not guessed from an incomplete enum:

- Registration at RVA 9EFCD8/9EFCE6 binds callback 9EA930 to the strings
  Actor (17C784C) and SetDontMove (191A480).
- Callback 9EA930 calls 6750D0 (address-library ID 37489).
- The true branch passes 9 to 680740 (ID 37612); the false branch clears
  life only when its masked value equals 9.
- 6808BF..6808F2 extracts/updates exactly bits 21:4 at Actor+C8. This agrees
  with the repository's ActorState offset assertions. The image's SHA256 is
  c434208894f07f604b852f29b8edc3a58c4de63de783373733e72b2b73f33be9.
- CharacterCreationService::SetPlayerActorLock already invokes SetDontMove(true).
  This identifies the state's meaning and normal use during creation; it does
  not prove which call last wrote that particular remote's word.

Only this veto is removed: life=0 or 9 is permitted, still subject to every
other guard. No setter, state normalization, or new native call is introduced.
Dead, disabled, combat and mounted checks remain separate native checks; the
life predicate explicitly rejects other lifecycle states instead of bundling
life, flight, knock and attack into one opaque mask.

| Predicate | Policy and justification |
| --- | --- |
| LifeAllowsReplacement | Alive or DontMove. Dying/dead, unconscious, reanimation/recycling, restrained, essential-down/bleedout and unknown values stay outside the supported lifecycle. IsDead alone cannot describe all these transitions. |
| KnockIdle, AttackIdle | No knock/recovery/ragdoll-related transition or active attack; native transition/action state is not reconstructed by the LAB. |
| RecoilIdle, NotStaggered | No active hit-reaction transition to lose while replacing the native actor. |
| FlyIdle, WeaponSheathed | Existing conservative LAB limits, including steady flight/drawn weapon. These values are not all proven deletion hazards; no further admission is inferred from this run. |
| NotSprinting, NotSwimming | Existing conservative movement limits. Ordinary movement is not equated with death or a native lifetime hazard. This mission changes only DontMove admission. |

No sit/sleep/furniture field participates. Unknown life values 10..15 remain
fail-closed. `stateVeto` names the first failed predicate, and individual booleans
show all failing predicates even if several fields are nonzero. Numeric fields
are lifeState, knockState, attackState, flyState, weaponState and recoilState;
lifeName supplies the semantic label (including dont-move or unknown).
Staggered, sprinting and swimming are logged because the policy reads them.

At capture, the same saved words drive both the gate and its diagnostic. A
failure keeps detail=unsafe-actor-state and adds stateVeto, decoded fields and
predicate results to that same line. Unresolved actors use stateDecode=unavailable.
Accepted capture logs actor-state-accepted with the same evidence before the
existing gate-accepted / transaction-reserved / candidate-create-enter sequence.
Later old-binding, precommit and post-placement state failures emit
actor-state-rejected with their boundary and decoded reason before normal abort.
Neither logging nor admission changes the materializer or native retirement.

Implementation/validation truth is in STATUS; runtime procedure and corrective T1-T14 mapping
are in TEST_PLAN. The sections below describe LEGACY / BYPASSED FOR CHARACTER
CREATION hot paths. Their presence in the repository does not make them reachable
from Character Creation. Cleanup is a separate future task.

## Legacy hot-apply vanilla humanoid domain (bypassed for Character Creation)

The final-only applier on runtime 1.6.1170.0 admits the eight base Skyrim.esm
playable races: Breton (0x13741), DarkElf (0x13742), HighElf (0x13743), Imperial
(0x13744), Nord (0x13746), Orc (0x13747), Redguard (0x13748), WoodElf (0x13749).
These are master-relative identities, resolved with ModManager and compared to the
actual TESRace pointer. Loaded FormID masking is not an eligibility check.
Argonian/Khajiit, custom records, vampire variants, transformations, unresolved and
nonplayable races are outside this phase. Their exclusion does not imply a different
native algorithm. Winning runtime forms must retain the playable flag and nonempty
male and female model paths of at most 512 bytes. Paths need not match across
sexes or races. The exact-runtime check precedes the audited read-only flags prefix.

`ApplyAppearanceSnapshots` is the single hot-update dispatch. Pure
`ClassifyAppearanceTransition` compares race identity and sex only; the descriptor
wrapper additionally checks validity and coherent runtime/base race. Domain eligibility
is separate and applies before any native mutation. No concrete race selects a strategy.

| Classification | Native order |
|---|---|
| SameRaceSameSex | Existing 4E Deserialize, one rebuild, historical face/head readiness, FaceGen |
| RaceChangeSameSex | SwitchRace(false), race/identity/source-sex check, Deserialize, target check, one reset, strict readiness, FaceGen |
| SameRaceSexChange | Deserialize, target-sex/unchanged-race/identity check, one reset, strict readiness, FaceGen |
| RaceAndSexChange | SwitchRace, verify race/source sex, reset1, race geometry ready, return to service, later-update final Deserialize, verify race/target sex, reset2, final geometry ready, FaceGen |
| Invalid | Refuse before mutation |

Race-only reserves each SwitchRace/Deserialize/reset once. Combined reserves one
SwitchRace, one final Deserialize and exactly two distinct reset phases at most;
sex-only retains one reset and never calls SwitchRace or writes sex bits. Strict shared geometry requires nonnull root,
face and head, a new root, and a changed face or head relative to pre-reset values;
ongoing race/sex/identity checks remain separate mandatory prerequisites. Events alone
are insufficient. The existing 120-tick budget includes FaceGen, and Generated must
belong to stable verified geometry. 4E's historical readiness remains distinct.

One immutable Active and one Latest pending snapshot prevent parallel mutations.
After a cycle completes, the next snapshot is classified against the actual current
actor state. Thus the authorized classification domain is closed under intermediate
race/sex edits, including race-first then sex-only on a non-Nord race. This is not a
claim that every engine invocation succeeds: mismatch, loss of provenance, reset failure
or timeout stops without rollback, replacement, respawn or retries of the active snapshot.

Canonical native bytes/flags/tints, derived descriptor, spawn capture/MarkChanged policy,
NPC behavior, message wire formats, owner validation and final-only publication remain
unchanged. No live fingerprint sampling or 2 Hz publisher is enabled in this phase.
Native position/death and inventory/equipment remain observed; no position, inventory,
ownership or identity rewrite is introduced. Aggregate checks cannot prove per-item
identity or rendered correctness. Weight remains diagnostic and nonfatal.

The phase sections below retain historical experiments and their original narrower
gates; the current domain and dispatch above supersede those eligibility restrictions.
Validation results belong only to STATUS; runtime acceptance is in TEST_PLAN.

## Combined native lifecycle experiment

`RaceAndSexChange` uses an internal `CombinedAppearanceCycle` under the same
`ApplyAppearanceSnapshots` entry, domain and immutable Active/Latest component.
The previous atomic combined order is superseded by two sequential native phases.
Race-only, sex-only and historical4E native paths retain their existing behavior.
No transport, spawn, capture or live publisher changes accompany this experiment.

Phase1 calls SwitchRace(target,false), reacquires and checks target runtime/base race,
SOURCE sex and all identity/provenance fields, then requests reset ordinal1 without
Deserialize or final tints. Record pre-reset1 geometry. Wait for nonnull root/face/head,
new root AND changed face or head; check the source-sex invariants every tick. At120
unsuccessful ticks stop with `combined-race-stage-head-timeout`. Do not consume the
final bytes, issue reset2, or call FaceGen after this failure. There is no fallback.

Observing race/source-sex readiness sets `RaceStageReady`, logs
`phase-boundary=return-to-service` with serverId/serviceUpdate and immediately returns.
A dedicated branch before the waiting-state filter resumes on a later CharacterService
update. The existing update ordinal is retained as `RaceReadyUpdate`; equality forbids
same-update reentry, with no elapsed-time budget or arbitrary frame delay.
Before any final write, reacquire ECS/Actor/Base and recheck immutable Active presence,
server/form/pointer/private provenance and remote-player markers, cached identity,
target runtime/base race with SOURCE sex, inventory/equipped totals, WaitingFor3D=false,
and strict race geometry relative to pre-reset1. Failure stops without final bytes/reset2.
Log `combined-final-stage resume-from-race-ready` with serverId, serviceUpdate,
race/sex/root/face/head. Only then consume the one immutable Active final payload.
Reacquire and verify target race and TARGET sex plus private identity before reset ordinal2.
Deserialize and reset2 remain in that same later invocation, without a second barrier. Record
fresh pre-reset2 geometry, which may include a null head. Final readiness uses those
new reference tokens and the same strict geometry predicate, alongside target race/sex
checks. Phase2 gets its own120-tick budget including FaceGen completion. No elapsed
RaceMenu delay, completion event alone, or arbitrary sleep authorizes either phase.

One final FaceGen Setup consumes incoming tints after final readiness. Existing Update
may poll across ticks for material/Generated in that single bounded tint cycle; it is
never called in phase1. A changed geometry target invalidates Generated, and post-Update
identity/phase/geometry checks precede Applied. No second Setup/tint cycle or third reset.
A native-call-in-progress guard prevents combined update reentry during engine calls.
Reservation states also cannot advance while the call is outstanding.

Combined diagnostics identify race/final stage and reset ordinal, before/after native
headparts/hash, hair/bodyColor, overlay, models, geometry, actor/base and network IDs,
position/death and inventory/equipped totals. The packet has no separate headpart hash:
comparison with A is observational via both logs, not a new wire field or fabricated
remote comparison. Weight remains nonfatal. Native failure stops without rollback.
Whether this service boundary restores the final rendered head remains a runtime
question. Current supplied phase1 evidence and final-stage failure are recorded in
STATUS; pure tests do not establish native rendering success.

## Existing capture and spawn pipeline

`CharacterService::RequestServerAssignment` captures the local player's
`TESNPC::Serialize` bytes, `GetChangeFlags()` and `PlayerCharacter::GetTints()`.
Assignment marks the player NPC changed with `0x2000800` before capture.
`TESNPC::Serialize` uses a 32 KiB native save buffer and `ScopedSaveLoadOverride`.
Tint entries carry texture name, alpha, color and type. The diagnostic sampler
does not mark the NPC changed and does not send network messages.

Server `CharacterService::CreateCharacter` stores assignment data in
`CharacterComponent::{SaveBuffer,ChangeFlags,FaceTints}`. Its `Serialize` method
copies all three into the ordinary `CharacterSpawnRequest`.

On the remote client, `OnCharacterSpawn` and `CreateCharacterForEntity` create
the custom NPC with `TESNPC::Create`, which initializes it and deserializes the
native bytes, or deserialize an existing base when a BaseId is supplied.
The custom-player path calls `FaceGenSystem::Setup` and creates the remote
Actor. `WaitingFor3D` delays the other spawn setup. `RunRemoteUpdates` calls
`FaceGenSystem::Update` until it finds head geometry and produces its texture.

`OnBeastFormChange` sends `RequestRespawn` with bytes and flags but no tints.
The server updates those fields and relays `NotifyRespawn`. The observer uses
`CancelServerAssignment`, which can delete the temporary actor and remove its
network components. This is unsuitable for slider updates.

## Generic snapshot protocol

`RequestCharacterAppearanceUpdate` and `NotifyCharacterAppearanceUpdate` use
one shared payload and are appended to their opcode enums (74 in each
direction), preserving existing opcode values. Both factories register them.

Wire order:

1. ActorId: existing network entity ID, uint32 varint, not a Skyrim FormID.
2. ChangeFlags: 32 bits.
3. AppearanceBuffer: existing string wire format, including embedded zero bytes.
4. FaceTints: existing `Tints::Serialize` format. Count is 8 bits; each entry has
   type varint, color 32 bits, CachedString name and alpha float bits.
5. Final-creation descriptor: schema byte 2, Race GameId (BaseId then ModId varints),
   Sex byte (0/1), Weight float bits (finite, 0..100).
6. FinalBuildRevision: 64 bits, identifying the authoritative Applied build.

This is the current wire contract, including the cumulative changes since `main`.
The decoder rejects schema 1 and every unknown schema; there is no schema-1
migration or negotiation. A zero final revision is decodable but is rejected by
the final-build service gates. Matching client/server builds are required.

Limits are 32 KiB of nonempty native bytes, 255 tints, 1023 bytes per texture
name, finite alpha in [0,1], no embedded NUL in texture names, and a conservative
aggregate bound of 64 KiB including packet prefix/opcode. The aggregate bound
does not depend on the current StringCache contents. Oversized local snapshots
encode a rejected length rather than silently truncating.

The new decoder checks every read and bounds varints and strings before
allocation. It decodes the existing tint format into `Tints`, rejects unknown
cached IDs, and commits the complete payload only after validation. There is no
second tint model and no change to existing assignment/spawn encodings.

Server `OnCharacterAppearanceUpdate` resolves an entity with OwnerComponent and
CharacterComponent. It also requires an Applied CharacterBuildComponent, a
nonzero matching FinalBuildRevision and the build's canonical Race GameId.
`MatchesAppliedCharacterBuild` rejects any final that differs from an already
accepted FinalAppearance. `ApplyCharacterAppearanceUpdate` then rejects absent
senders, mismatched owners and invalid payloads before mutation. The first valid
final replaces all three canonical fields, including an empty tint collection,
and is retained in FinalAppearance. Exact duplicate finals may relay again;
conflicting updates cannot replace that final. `SendToPlayersInRange` excludes
the sender. No animation cache, inventory, ownership or entity is replaced.

The store is the existing in-memory character component, not new durable
campaign checkpoint persistence. Future ordinary materialization reads these
fields through the existing spawn serializer. This creates no admission or
late-join permission: ADR-0018 still requires sealed-roster collective recovery.
Matched client/server builds are required before activating these new messages;
older factories do not know the appended opcodes.

### Ordering and update execution audit

The client and server opcode enums have separate factories and extractor
tables; equal opcode numbers across directions are intentional. TransportService
calls `TiltedConnect::Client::Send` with its default `kReliable` argument;
GameServer similarly calls `Server::Send` with `kReliable`. Both implementations
pass `k_nSteamNetworkingSend_Reliable` to GameNetworkingSockets. The installed
1.4.1 `isteamnetworkingsockets.h` specifies in-order delivery of reliable
messages. The receive loops consume one message at a time and dispatch it
before taking the next. FinalBuildRevision correlates appearance with the Applied
build; it is not a live-slider sequence number. Duplicate finals are fenced on
the receiver as well as frozen on the server. A new connection still requires
pending-state reset.

`SkyrimVM64.cpp::HookVMUpdate` (relocation 53926) calls
`TiltedOnlineApp::Update` when the VM context is active; this calls
`World::Update`, which runs RunnerService and dispatches UpdateEvent.
CharacterService's OnUpdate is the existing engine service path. TransportService
is also pumped through UpdateEvent. The final receive callback queues its
snapshot; native rematerialization runs in the service tick. The
existence of this tick does not prove that a particular native reconstruction
operation completes synchronously there.

### Phase 4 audit: private base and WaitingFor3D distinction (historical)

The server rejects a nonempty BaseId for a player assignment. Player spawns
therefore normally take the custom-form/empty-BaseId branch and allocate a new
TESNPC through IFormFactory for that actor. This is allocation provenance, not
a runtime exclusivity guard: the current ECS does not retain a private-base
association. An applier must track that association at creation and verify it
against the current actor/base before deserializing; a dynamic FormID alone
does not establish exclusive ownership.

WaitingFor3D has two relevant states. With no Actor available,
CreateCharacterForEntity consumes its SpawnRequest and creates the NPC from
the cached bytes. With an already-created Actor, the waiting loop only applies
inventory/factions/animation/death setup once a NiNode exists; it does not
deserialize appearance again. RunSpawnUpdates can also reuse CachedRefId
without calling CreateCharacterForEntity. Thus replacing the three cached
appearance fields is necessary but insufficient for both existing-Actor paths.
The future adapter must preserve a pending appearance snapshot through that
initial 3D completion and apply it safely afterward, without starting a second
concurrent rebuild or replaying inventory setup.

## Phase 4 native application audit (historical; superseded below)

Do not enable the RaceMenu publisher until the generic remote adapter can
complete this sequence on the engine-safe update path:

```text
CharacterCreationService (existing 2 Hz sampler; forced close capture)
  -> local publication event
  -> client CharacterService (capture bytes + flags + tints)
  -> RequestCharacterAppearanceUpdate
  -> server CharacterService (owner check, replace canonical snapshot)
  -> NotifyCharacterAppearanceUpdate, in range excluding owner
  -> remote CharacterService (deserialize, rebuild, tint new geometry)
```

The final-only publisher is connected for the diagnostic below. The remote
snapshot applier and 2 Hz publisher remain disabled. Consequently initial tint
transition, live morphs and race changes are not visually fixed.

Concrete gaps found in the native audit:

- Before the diagnostic probe, `Actor::QueueUpdate` had no callers here. Its relocation 40255
  is CommonLib's `Actor::DoReset3D(bool updateWeight)` (SE 39181 / AE 40255),
  despite the wrapper's name. It temporarily changes the global
  `bUseFaceGenPreprocessedHeads` setting; the probe slice adds a null guard and
  a boolean reporting whether the call was issued. This does not
  establish a queued operation or a completion boundary.
- SKSE's `QueueNiNodeUpdate` routes through Character's native method. It is
  not evidence that calling the existing STRE wrapper provides the same
  lifecycle. No audited queue/completion adapter is present here.
- Actor stores its own `race` pointer in addition to its NPC base. The code has
  no SetRace/switch-race adapter establishing how a live deserialize refreshes
  that state and the related skeleton/sex/weight state.
- `FaceGenSystem::Update` marks `Generated` on the first available head. After
  an asynchronous rebuild request it could tint the old geometry and then
  never tint its replacement. `Setup` also returns immediately for zero tints,
  so clearing an existing rendered tint is not defined by that helper.
- `WaitingFor3D::SpawnRequest` caches spawn appearance for deferred creation.
  The future adapter must replace stale cached appearance too, including when
  the remote reference is unloaded, without losing intervening snapshots.

The next native slice must prove reference lifetime, race/sex/weight refresh,
new-geometry completion, empty-tint clearing and pending-snapshot convergence.
It must preserve server ID, RemoteComponent, interpolation, animation state,
inventory and ownership. A speculative Disable/Enable, network respawn or
fixed-delay completion heuristic does not satisfy this contract.

## Phase 4B diagnostic-only NiNode probe (historical)

The phase 4B diagnostic publishes only a fresh RaceMenu-close snapshot through
RequestLocalAppearanceUpdateEvent and generic CharacterService. It does not
publish from appearance-sample or switch-race-complete. One pending final is
latest-wins until a local player assignment is available and Send returns true;
disconnect/recovery lock clears it. Solo makes no network attempt. Send success
is not an acknowledgement of server storage or rendering.

The observer resolves RemoteComponent plus PlayerComponent and retains one
Notify in RemoteAppearanceProbeComponent. Received bytes/tints are never passed
to Deserialize or FaceGenSystem::Setup, including WaitingFor3D's spawn cache.
This intentionally tests the unchanged existing appearance, not the incoming one.

RemotePlayerAppearanceBaseComponent records actor and base FormIDs immediately
after STRE allocates a private TESNPC and temporary remote player Actor. Shared
base and non-player branches never get that marker. The tick verifies both IDs,
the temporary base, current Actor/base RTTI, RemoteComponent/CachedRefId,
PlayerComponent and remote-player extension before any native reset. Remote
teardown clears marker/probe before deletion; disconnect clears them globally.
Replacement creation invalidates provenance and rebinds diagnostic state only
after a new private allocation, retaining the latest diagnostic payload.

The probe waits for initial WaitingFor3D completion and existing third-person,
face and head geometry. It uses ExtraDataList::Contains(Interaction, type 0xA9)
as the rider guard and also skips IsMount actors. This mirrors CommonLib's
IsOnMount check conservatively. It refuses runtimes other than 1.6.1170.0.
It then calls the existing QueueUpdate/DoReset3D(true) once with NPC data
unchanged. At most 120 service ticks including initial waiting observe pointers,
race, weight, position and actor flags. Latest notifications do not reset this
budget or rearm a completed probe. Unchanged pointers yield inconclusive;
changed/null-return pointers only yield a diagnostic transition, never readiness.
FirstTransitionTick counts from the queued probe, including any initial waiting.

The remote player has no first-person mesh contract exposed by this adapter;
GetNiNode is the existing third-person getter. There is no audited occupied
furniture getter in this slice. No extra vtable slots were guessed for either.
Pointer tokens and per-tick logs are temporary local diagnostics to remove
before merge. Native preservation of inventory/position/ownership remains a
manual runtime check, not a consequence proved by the pure tests.

SKSE 2.2.6 Hooks_NetImmerse emits SKSENiNodeUpdateEvent after UpdateEquipment.
STRE's ScriptExtender integration loads StartSKSE but exposes neither a plugin
messaging/QueryInterface bridge nor a relayed Papyrus NiNode event to its ECS.
Therefore **SKSE NiNode event unavailable to STRE through the current adapter**.
The probe adds no hook, callsite patch or new SKSE infrastructure.

Primary references for the probe:

- [SKSE 2.2.6 Actor mesh-update documentation](https://github.com/ianpatt/skse64/blob/v2.2.6/scripts/modified/Actor.psc)
- [SKSE 2.2.6 NiNode event hook](https://github.com/ianpatt/skse64/blob/v2.2.6/skse64/Hooks_NetImmerse.cpp)
- [CommonLib IsOnMount implementation](https://github.com/CharmedBaryon/CommonLibSSE-NG/blob/main/src/RE/A/Actor.cpp)
- [CommonLib ExtraInteraction type](https://github.com/CharmedBaryon/CommonLibSSE-NG/blob/main/include/RE/E/ExtraDataTypes.h)

Native references:

- [CommonLib Actor implementation](https://github.com/CharmedBaryon/CommonLibSSE-NG/blob/main/src/RE/A/Actor.cpp)
- [CommonLib offsets](https://github.com/CharmedBaryon/CommonLibSSE-NG/blob/main/include/RE/Offsets.h)
- [SKSE64 2.2.6 PapyrusActor](https://github.com/ianpatt/skse64/blob/v2.2.6/skse64/PapyrusActor.cpp)
- [TiltedCore serialization](https://github.com/tiltedphoques/TiltedCore/blob/master/Code/core/src/Serialization.cpp)

## Phase 3/4B validation boundary (historical)

`Code/tests/character_appearance.cpp` covers both factories, cached/uncached
names, compatibility with the existing tint decoder, size limits, truncated
packets, malformed IDs, tint-only replacement, empty-tint clearing, duplicate
state, owner rejection, canonical fields and exact broadcast payload.
The ownership test calls the production policy with a component-shaped fixture;
it is not a running-server spatial routing or Skyrim integration test.

After completing the adapter, validate bidirectional two-client initialization,
race/morph/tint changes, forced final snapshot, deferred materialization and
Solo. Reuse the existing 2 Hz sampling loop, deduplicate the visual payload
independently of camera/actor diagnostics, and keep a final snapshot pending
until assignment/transport permits publication. A successful send alone must
not be described as proof of server or observer application. Campaign
disconnect scenarios must follow collective recovery, not partial-roster
catch-up. Black skin after complete application requires separate FaceGen
evidence rather than a hard-coded color workaround.

## Phase 4C application contract (historical; weight policy superseded by 4E)

Final-only publication remains on RaceMenu close. The 2 Hz sampler is local
only. Race GameId, sex and weight are captured with the native buffer and tints;
unknown race mapping, invalid descriptors or payloads are rejected. Both message
factories use descriptor schema 1. A phase 4C receiver rejects missing/unknown
schemas. This is not backward-compatible negotiation: a phase 4B receiver can
ignore the new tail. Use phase 4C on both clients and server. The server relays
the descriptor unchanged; it does not persist it or alter ordinary spawn wire.

The private-base provenance marker from phase 4B is mandatory. Incoming race
must equal both runtime Actor race and TESNPC base race through ModSystem's
GameId mapping; sex must match TESActorBaseData's female bit (bit 0). Weight is
allowed to differ. No temporary native object is decoded to obtain metadata.
This guard trusts sender metadata; native bytes are opaque. After Deserialize,
base/runtime race, sex, weight (tolerance 0.01), Actor/base identity and ECS
provenance are checked again. Failure stops without rollback or native reset.

Only nonempty tints are eligible. Initial WaitingFor3D must finish and current
3D/face/head must exist before mutation. Mounted actors and runtimes other than
1.6.1170.0 are skipped. One latest pending snapshot becomes immutable active
before Deserialize on the existing private TESNPC. At most one latest follow-up
is retained during that cycle. The existing QueueUpdate/DoReset3D(true) is called
once. No SwitchRace, Delete, respawn, new Actor or network component replacement
is performed by this path.

The 120-tick budget starts after reset, not while awaiting initial spawn 3D.
Only nonnull face/head with either token changed from before reset permits
FaceGenSystem::Setup. Main 3D change alone is insufficient. Setup replaces the
component with Generated=false. Update retries within the same budget and only
Generated=true produces the applied log. A later head change restarts generation;
return to the original head cannot pass. Generic FaceGen is blocked during the
cycle and remains blocked on failure until a successful follow-up or teardown.
A transition is an experimental readiness criterion; native rendering correctness
still needs human evidence. Pointer-address reuse can cause conservative timeout.

Before/after logs include actor/server/base, runtime/base race, sex, weight,
headpart count/hash, body RGB, hair color FormID/ABGR, position, aggregate inventory
and equipped counts, dead state and remote-player flag. No ownership epoch exists
in RemoteComponent, so the log explicitly says unavailable. Aggregates are useful
smoke evidence, not full inventory equality. No audited body/hair shader refresh
wrapper exists in this adapter; no speculative relocation or hard-coded color
has been introduced. If head tints succeed but body/hair remain incorrect, isolate
that native refresh seam in a later phase.

Disconnect, recovery lock and remote teardown clear state as in phase 4B. Solo
never publishes. Campaign persistence and collective recovery remain ADR-0018's
contract. Deferred ordinary materialization continues to use the server's stored
snapshot; there is no campaign late-join behavior.

Tests cover descriptor round trips/limits, race and sex rejection, weight changes,
latest follow-up isolation, face/head readiness and timeout, plus prior bounded
protocol, owner-policy, pending-final/offline and provenance cases. Native ECS
preservation and renderer behavior are manual tests, not mocked unit guarantees.
For runtime acceptance, use a fresh A/B Nord campaign with the same sex throughout,
change morphs/headparts/weight/skin, close RaceMenu, and verify one final send/reset,
transition, Generated completion, visual convergence and stable IDs/inventory/
position/ownership. Repeat B to A. Empty tints and cross-race/sex tests should log
an explicit skip before mutation. Follow collective recovery on disconnect.
## Phase 4D postcondition diagnosis (historical; superseded by 4E evidence)

The supplied runtime traces stop after Deserialize on both observers. The local
installed tp_client.log also records the serverId 3 descriptor weight 75, pre-state
weight 50 and generic stop. These are not post-state observations; weight-only
failure cannot be concluded from them. No reset/tint success was reached in those
traces. Partial visual changes do not justify an independent black-head fix yet.

AppearancePostState/CheckAppearancePostState now visits all 16 fields without
short-circuiting: actor/base pointer tokens and FormIDs, serverId, FormIdComponent,
CachedRefId, both provenance IDs, runtime/base race, sex, remote/player/private
markers and weight. Each logs pre/expected/actual plus match/mismatch. Identity
and race expectations come from the verified pre-state; sex/weight from the
incoming descriptor. Pre/post appearance logs record headparts and colors; these
may legitimately change and are not fatal invariants. Missing active state has
its own failure log. Pointer tokens are temporary local diagnostic evidence.

Weight remains fatal with the existing 0.01 tolerance. The handoff's prerequisite
for removing that check is not met. Source audit: Games/Forms.cpp delegates to
native Save/Load with GetChangeFlags/incoming flags and ScopedSaveLoadOverride;
it neither serializes nor assigns weight explicitly. Flags 0x02000860 include
race, face, factions and full name according to CommonLib's NPC flag declaration,
but that declaration does not establish the native face payload's weight layout.
See [CommonLib TESNPC ChangeFlags](https://ryan.commonlib.dev/TESNPC_8h_source.html).
DoReset3D(true) is not evidence of weight transport.

Weight live path not proven. There is no dedicated weight message/setter in the
audited source. SaveAnimationVariables -> ClientReferencesMoveRequest -> server
movement relay -> remote LoadAnimationVariables only transfers descriptor-selected
variables. Master_Behavior declares bodyMorphWeight (127) but does not select it
in the synchronized float table. Modded behavior selection is not evidence that
this runtime used a weight path. Actor.cpp's weight comment is inside SAVE_STUFF
reverse-engineering code, not the active NPC serializer. Local player TESNPC is
the descriptor source; no separate authoritative network weight contract has been
established. No new weight channel or forced weight assignment is introduced.

The remaining fatal stop after native mutation has no rollback and is diagnostic
only. The rest of the one-reset/120-tick/transition/tint pipeline is unchanged.
A new runtime trace must establish all actual post-values; if weight is the only
failure, still establish whether the buffer guarantees its restoration or whether
a separate path owns it before relaxing that check. No deployment before review.

## Phase 4E current weight and FaceGen contract

The user-supplied 4D trace in the 4E handoff isolates Weight as the only mismatch:
all fifteen identity/provenance/race/sex/marker fields pass; weight remains 50
instead of descriptor 75, while headparts and body/hair color mutate. Weight is
not an appearance-apply postcondition. CharacterAppearanceDescriptor.Weight is
diagnostic metadata only for this path. Remote weight convergence is observed
live through another runtime path not yet identified. AppearanceApply neither
owns nor writes remote weight. TODO: Identify existing live remote weight
propagation path. No new issue or separate network channel is created here.
Existing descriptor wire validation (finite 0..100) remains input validation;
it does not make that value an authoritative remote assignment.

CheckAppearancePostState still visits every field. Weight compares with the old
0.01 diagnostic tolerance, logs nonfatal=true and never contributes to its result.
Every other field stays fatal. Race/sex and allocation-time private provenance
remain preconditions. No native rollback, replacement Actor/base or guessed
SetWeight is added. QueueUpdate uses the actual native state at call time; its
weight-related renderer effect remains a manual nonregression test.

The active cycle now exposes a one-shot RequestReset gate, reset on Begin.
Postcheck success reaches the existing QueueUpdate(true) once. Transition remains
nonnull face/head with either changed; no fixed sleep. CompleteTints accepts
Generated only in the tint stage, clears active state and unblocks generic FaceGen.
The 120-tick budget, timeout behavior, latest follow-up and offline policy remain.

FaceGen diagnostics record tint count and deterministic ordered FNV-1a hash of
Type/Color/Alpha/Name (including lengths and count), without StringCache effects.
This is diagnostic, not authentication. Before Setup: current geometry tokens,
Generated and body RGB. After Setup: prior component presence/replacement,
Generated=false and stored count/hash. Each bounded Update logs Generated,
geometry, equality with current target and original transition, stored/active
hashes and body RGB. If geometry changes during Update, Generated is cleared
before completion; subsequent ticks retry within the same budget. No success
may be reported merely because an old component was Generated.

Weight is observed before reset, at transition and in the applied appearance log.
Existing pre/post logs cover IDs/provenance, headparts/hair colors, position,
inventory/equipment aggregates and death/remote status. Aggregates are smoke
evidence, not exact item equality. No safe public shader/material/texture-handle
inspection helper is currently exposed by this adapter; no new relocation or
material access has been added solely for diagnostics. A black head after actual
face-tints-applied/applied is a subsequent focused FaceGen investigation.

Runtime acceptance: after review, fresh Nord/Nord same-sex A/B, nonempty tints,
change face/headparts/skin/weight without race/sex changes, then close RaceMenu.
Expect diagnostic Weight, passed, npc-deserialized, one reset, transition,
facegen begin/setup/update generated=true, face-tints-applied, applied. Compare
appearance, weight before/after reset, stable IDs/provenance, inventory/equipment,
position, death and remote ownership; repeat reverse direction. Do not interpret
unit tests as these native observations. No deployment before review.
## Phase 5A audit and diagnostic-only race classification (historical)

The user's 5A handoff validates 4E same-race/same-sex final application in game
(Nord/Nord): full pipeline, correct morphs/hair/head/body colors, observed correct
weight and stable Actor without visible state loss. This does not validate race
switches, beast skeletons, sex changes or the unidentified live weight path.

No local SwitchRace wrapper/caller exists. CommonLib declares a_player and delegates
to AE ID 37925, without implementing its effects. A parameter name does not prove
its meaning for an STRE remote Character. Prior runtime database audit maps this
to RVA 0x69ADF0 on 1.6.1170.0, but on-disk disassembly was not usable evidence of
engine behavior. No loaded-engine disassembly or remote call was performed in 5A.
Neither mutation ordering with Deserialize nor automatic rebuilding/completion/
equipment/weight preservation is established. No bool value or order is chosen.

Primary audit references:

- [CommonLib Actor declaration](https://github.com/CharmedBaryon/CommonLibSSE-NG/blob/main/include/RE/A/Actor.h)
- [CommonLib delegating SwitchRace implementation](https://github.com/CharmedBaryon/CommonLibSSE-NG/blob/main/src/RE/A/Actor.cpp)
- [CommonLib SE/AE offsets](https://github.com/CharmedBaryon/CommonLibSSE-NG/blob/main/include/RE/Offsets.h)

ClassifyAppearanceUpdate returns SameRaceSameSex, RaceChangeSameSex,
SexChangeUnsupported or Invalid. Invalid covers descriptor/target resolution,
inconsistent current races or missing private provenance. Target mapping uses
ModSystem::GetGameId -> TESForm lookup -> TESRace RTTI and rejects temporary races.
Same-race then uses the unchanged 4E native cycle. Race changes log requested
from/to and skip with switch-race-bool-and-order-unproven before Deserialize,
Begin, reset or incoming tints. Unsupported sex has its own pre-mutation reason.
Classification is not authorization for native mutation. No race-change state
machine, target-race postcondition or skeleton readiness is enabled speculatively.

Local observation is limited to RaceMenu on 1.6.1170.0. One cached value per service
tick holds runtime/base/overlay races, sex/weight, third-person node, face/head,
actor/base IDs and ActorState flags. On the existing local-player completion sink,
log that prior observation, current event state and one next-service-tick state.
Multiple events before a tick coalesce the next observation. Cached state is
cleared outside the probe/menu and reset on open. No new hook, event injection,
network publication or native mutation. The prior sample is not guaranteed to
precede the engine call; it is labeled observation, not call-entry/pre-mutation.
GetNiNode is an observed third-person root, not verified skeleton identity.
The existing sink filters local player, so it cannot establish remote completion
availability or absence. Event Actor equality is checked against local player only.

Next minimal probe after review: local male Nord -> High Elf and reverse, retain
these three observations and corresponding existing diagnostics. To unblock native
activation, obtain read-only debugger/source evidence for actual SwitchRace callers,
argument semantics (including the remote/non-player path), effects and ordering
relative to base Load, reset, event and 3D reconstruction. Count rebuilds. The local
probe alone cannot supply that proof. Do not add a trial true/false call, second
reset, guessed rollback, respawn or shader mutation. Prospective remote tests remain
Nord <-> High Elf, same sex; beast/sex changes stay out of scope.

## Phase 5A.1 passive native observation (pre-activation history)

The 5A.1 handoff explicitly authorizes a detour, superseding the previous
no-new-hook diagnostic scope only. Remote appearance race changes still stop.
SwitchRaceProbe.cpp resolves ID37925 only after VersionDb reports 1.6.1170.0 and
requires a committed executable target page. RunTiltedInit loads VersionDb before
InstallHooks2/Initializer::RunAll. TP_HOOK_IMMEDIATE uses the existing TiltedReverse
FunctionHookManager -> MH_CreateHook/MH_EnableHook path; both must succeed before
the manager retains the hook. No custom trampoline or delayed unchecked install
is added. Manager API returns no status, so install-attempt is not success proof;
a real phase=enter trace is required. Runtime/plugin conflicts are not prevalidated
by compilation or the executable-page check.

The x64 thunk uses the established TP_THIS_FUNCTION/TP_MAKE_THISCALL convention.
It calls the original exactly once with unchanged arguments, outside diagnostic
exception handling. There is no conditional suppression or STRE mutation. Win32
last-error values are restored around diagnostics; diagnostic C++ exceptions are
contained. This does not catch native access violations or remove timing overhead.
On return it checks Actor lookup identity before observing the old pointer.

Entry arguments log targetRace/argPlayer and _ReturnAddress captured in the thunk.
VirtualQuery plus GetModuleFileName yields caller module/RVA when it is an image;
otherwise unknown is explicit. Entry/return log actor/token, isPlayerRef, runtime/
base/overlay races, sex/weight, 3D/face/head and flags, with callSeq and thread ID.
A summary compares six race/node fields, never declares readiness. Module/RVA
and pointer tokens are diagnostic and runtime-specific, not portable contracts.

A mutex-protected 64-record FIFO assigns monotonic callSeq. No lock is held while
calling the engine or logging. Event/next-tick correlation reads the latest record
for the Actor pointer and reports InFlight; sequence0 means unknown/evicted.
Nested calls retain distinct entry/return sequences, but a later asynchronous
event's latest record is not proof of its originating call. Pointer reuse and
history eviction can invalidate association. The existing completion sink remains
local-player/RaceMenu-scoped; the hook itself observes any actor naturally passed
to the engine function. Non-player completion is not claimed observable by that sink.

Runtime plan: first local male Nord -> High Elf -> Nord via STRE RaceMenu, then a
separate disposable non-player diagnostic actor using a vanilla mechanism chosen
for the clean session. The mechanism is only relevant if the hook actually sees
isPlayerRef=false. No matching entry means no bool evidence, not false or zero.
Do not invoke SwitchRace directly on a remote to fabricate a sample. Preserve
entry/return/event/next-tick chronology, base/runtime at entry, caller and node
changes. A local trace alone does not authorize remote order or bool.

No new trace is supplied yet. The audit export script can extract SwitchRaceProbe,
switch-race-complete and local observation lines from each future session; do not
label absent logs as completed runtime validation. Activation of remote5A.2 still
requires real player/non-player argument evidence, base/runtime ordering and a
justified bounded rebuild strategy. No extra QueueUpdate is inferred from silence.

## Phase 5A.2 remote race contract (historical; reset/readiness superseded by 5A.3)

This section supersedes the historical 5A/5A.1 refusal policy. Implementation and
validation status belongs exclusively to STATUS.md. The revised handoff supplies
two 1.6.1170.0 observations: player 0x14 via RaceMenu (caller SkyrimSE RVA 0x952200)
and loaded NPC 0x2E1B4 via setrace (RVA 0x371AF8), both argPlayer=false. Runtime/base
race change inside the call, while old nodes remain at return; rebuild is deferred.
NPC reverse direction also uses false. Player completion precedes full head readiness.
These supplied traces justify the selected experimental order, not remote visual success.

Actor::SwitchRace resolves AE37925 on exact 1.6.1170.0, rejects null/temporary targets
and true, and issues one x64 native call. Its bool return means call issued, never
visual completion. The passive detour observes the resolved-address call normally.

Eligibility retains remote/player markers, mapped and coherent current runtime/base
race, valid descriptor/target, private allocation provenance, stable cached IDs,
loaded root/face/head and unmounted status. Incoming sex must match before mutation.
Only Skyrim.esm-relative Nord 0x13746 and High Elf 0x13743 are allowed, resolved through
ModManager/GetFormId instead of loaded constants. Their current-sex skeleton paths
must be nonempty, bounded to 512 bytes and equal ignoring ASCII case/slash direction.
No beast/custom race is inferred compatible from a matching path alone.

The local TESRace read-only prefix exposes skeletonModels[2] at 0x98, with TESModel
stride 0x28, both compile-time asserted against the local layout. Source:
[CommonLib TESRace](https://github.com/CharmedBaryon/CommonLibSSE-NG/blob/main/include/RE/T/TESRace.h).
Equal paths are a deliberately narrow gate, not proof against overridden model assets.

Mutation order is preguards -> SwitchRace(target,false) once -> reacquire actor/base
and validate all fifteen identity/race/sex/provenance/marker fields -> Deserialize
the immutable target snapshot into the same base -> the same fifteen postchecks.
Expected runtime/base races are target at both boundaries. Weight stays nonfatal;
overlay is logged, never assigned or treated as invariant. Failure stops without rollback.

RaceAppearanceCycle records PendingRaceSwitch, SwitchRequested, SwitchReturned,
Deserialized, WaitingForRace3D, WaitingForRaceHead, ApplyingTints, Applied and Failed.
Only one active snapshot exists, plus one latest follow-up; an active cycle cannot
request another switch. RequestReset explicitly rejects a race cycle, and the native
race branch never reaches QueueUpdate. The 4E branch retains its original reset.

CharacterService copies remote completion actor ID/token and observed race into
RunnerService's queue. The service-thread handler associates only a matching active
actor/target and sets EventSeen. Delayed events can match a later same-target cycle;
this association is diagnostic, not causal or a prerequisite to progress.

At most 120 service ticks cover reconstruction and tint generation together. Every
waiting/applying tick checks the fifteen invariants against the target state. Readiness
requires nonnull new third-person root AND nonnull changed face or head. Completion
alone, an old root, null geometry or root-only change does not enable FaceGen. This
conservative policy can time out if allocation addresses are reused. Node readiness
reuses 4E Setup (Generated=false), then Update until Generated=true on stable target
geometry. Root changes also invalidate race-cycle generation. Existing AppearanceApply
FaceGen logs retain tint count/hash, geometry and Generated; RaceAppearanceApply adds
switch, transition/event/readiness and final applied diagnostics.

Before/after observations include actor/base/server IDs, provenance, race, sex, weight,
headparts/hash, colors, position, inventory entries/count, worn entries, dead and remote
marker. No ownership epoch accessor is available; that limitation is explicit in logs.
Observed decreases in inventory count or worn-entry count stop the race cycle. These
aggregates cannot detect substitutions with unchanged totals; item preservation and
visual equipment correctness require human comparison in a quiet test session. Nothing
rewrites inventory/ownership/position or replaces/recreates/disables the actor. No second
switch/reset or speculative skin/material fallback is permitted after failure.

## Phase 5A.3 single remote race reset

The supplied multiplayer failure supersedes 5A.2's assumption that direct SwitchRace
alone necessarily rebuilds the remote head. Surrounding RaceMenu/setrace paths were
not proof of that narrower call's effects. Runtime evidence and validation status are
recorded in STATUS.md. The new order is SwitchRace(false), immediate target/identity
checks, Deserialize target snapshot, fifteen fatal postchecks, then one existing
QueueUpdate/DoReset3D(true). No new relocation or delay is introduced. The FaceGen
INI prerequisite is checked before mutation for this path as it already was for 4E.

Race state gains ResetRequested after Deserialized. RequestRaceReset captures current
root/face/head as Before and starts the 120-tick budget; it cannot be repeated for the
active snapshot. The component is reacquired after the engine reset; failure or lost
state stops with no retry. Successful return enters WaitingForRace3D. Later ticks still
validate identity/provenance/markers/sex and target runtime/base, plus logical-loss
aggregates. Weight remains nonfatal. Pending follow-up retains latest-wins semantics.

After review, readiness requires nonnull root, face and head, a changed root AND
a changed face or head relative to pre-reset. Root-only, face-only without root,
and head-only without root transitions are rejected. All three changes are logged
separately each waiting tick. The same predicate protects subsequent FaceGen attempts.
Completion events remain diagnostic only. Existing 4E Setup/Update and geometry stability
checks are reused; no material/shader workaround, second reset or replacement follows
failure. SameRaceSameSex's original single reset and face/head predicate are unchanged.

## Phase 5B.1 passive sex-change diagnostics

SexChangeProbe observes the real TESNPC actorData.IsFemale bit (IS_FEMALE=1<<0)
while RaceSex Menu is open, including console showracemenu outside the STRE creation
phase. Service ticks capture identity, races/overlay, flags/weight, root/face/head,
headpart hash, hair/body colors, inventory aggregates, native Serialize bytes/hash/
flags, local descriptor race/sex/weight and both normalized sex-specific skeleton paths.
Only changed fingerprints log; tick is excluded from the fingerprint. Tint hash uses
ordered type/color/alpha bits/length-delimited names without the network StringCache.
NPC tints are explicitly unavailable: only PlayerCharacter exposes GetTints here.
Neither binary buffer changes nor descriptor changes prove remote Deserialize semantics.

DoReset3DProbe observes AE40255 on exact 1.6.1170.0 with the same immediate MinHook
installation path and executable-page check as 5A.1. It logs arguments, caller module/
RVA, thread, sequence, sex/races/geometry/weight/flags before and after one unchanged
original call. C++ diagnostic failures are contained and LastError restored; native
faults are not hidden. Installation attempt is not proof of activation: require enter.
The existing SwitchRace detour remains and forwards before/after observations to the
sex chronology. The new probe never initiates SwitchRace, QueueUpdate, setters, FaceGen,
packets or gameplay mutation. Existing 4E/5A.3 paths and sex refusal remain unchanged.

Sixteen bounded actor ID/token records follow observed calls for at most 180 service
ticks per call/sex transition; local menu observation remains active while open.
Records expire outside the menu, missing/reused actor tokens are never dereferenced.
Native signals log direct snapshots without ECS access; a mutex protects copied
observations and revisions reject stale concurrent sample writes. Reads/logs/native
calls execute outside that mutex. Tick deltas indicate observed windows, not causality;
sexTickDelta on signals is relative to the last observed sex transition. Events are
observed for all actors through the existing native sink and never declare readiness.
A sex change wholly between samples can be missed unless a hooked call/event exposes it.

Optional NPC tracking begins when SwitchRace/DoReset3D is actually observed; absent
calls cannot establish an NPC baseline or prove absence of engine sex mutation. If
NPC sex was already changed at first entry, old sex is unknown. No global actor scan
or forced console command is added. Getters/Serialize and logging have timing and
allocation overhead; this is diagnostic instrumentation, not a release feature.

## Phase 5B.2 same-race sex experiment

This section supersedes 5B.1's blanket sex refusal for the scoped experiment only.
ClassifyAppearanceUpdate now distinguishes SameRaceSameSex, RaceChangeSameSex,
SameRaceSexChange, RaceAndSexChangeUnsupported, Invalid. Classification is followed
by experiment guards; same-race sex does not authorize arbitrary races. Nord is resolved
from Skyrim.esm-relative0x13746 via ModManager, never a loaded constant. Both model paths
must be nonempty and <=512 bytes; different male/female paths are expected and allowed.

The supplied Player S1/S2 traces show actual sex before reset, stable race/overlay,
head=null, then DoReset3D(true) callerRVA95499B and renewed root/face/head. No SwitchRace
was observed in those windows. These observations justify a bounded remote experiment,
not a proven remote deserialization contract. STATUS owns validation results.

CharacterSexAppearance.cpp handles only the dedicated SexAppearanceCycle. Existing
RunAppearanceProbes dispatches eligible pending snapshots and active sex cycles to it;
4E/5A.3 native paths are otherwise unchanged. State order: PendingSexChange ->
DeserializedSex (single invocation reserved) -> SexVerified -> ResetRequested ->
WaitingForSex3D/WaitingForSexHead -> ApplyingTints -> Applied, or Failed. One immutable
active snapshot and one latest follow-up are shared through AppearanceApply; sex state
resets on a new cycle/rebind and generic FaceGen remains blocked until verified success.

All current private allocation provenance, remote/player/temp markers, ID/cache mapping,
coherent races, exact runtime, unmounted loaded geometry, INI and nonempty tints guards
must pass. Deserialize writes the incoming native buffer once into the same private base.
Reacquisition and fifteen postchecks require stable actor/base pointers/IDs, server/form/
cache IDs, provenance, markers, unchanged runtime/base race and descriptor target sex.
Weight compares nonfatally. Sex mismatch emits deserialize-did-not-apply-sex with
nativeReset=false, stops and never uses a setter or second Deserialize. This failure is
useful evidence for a later explicit engine adapter investigation, not authorization
for a fallback in this phase.

Only verified sex permits one existing QueueUpdate(true)/DoReset3D(true). Record pre-reset
root/face/head. Readiness requires all nonnull, new root and changed face or head; each
tick also verifies actual sex, races and all identity invariants. No event dependency.
The total 120-tick post-reset budget includes FaceGen completion. Incoming tints feed
unchanged Setup/Update; Generated must complete against stable verified geometry.
Reacquire/check again after Update before declaring applied. No shader/color/material
write or SwitchRace is added. Failures block generic FaceGen with no speculative rollback.

Pre/post appearance logs include position/death/inventory/equipment/color/headparts;
fatal checks log CachedRefId and provenance/markers. Inventory/worn aggregate decreases
stop, but totals cannot prove per-item equality and legitimate concurrent changes may
conservatively stop the experiment. No ownership rewrite/accessor is introduced.


## R2 mutation provenance diagnostics

AppearanceTrace is observational and leaves the native two-phase lifecycle unchanged.
Publisher attempts carry a process-local monotonic attemptSeq (not a new wire field).
Every capture rejection is named; pending flush/send logs correlate by attempt and
network actor ID. Idle flushes with no snapshot are quiet. Server broadcast markers
are emitted inside the existing recipient loop and mean Send was invoked, not an
acknowledgement. Receiver markers precede rejection and identify queue state.

RemoteMutation snapshots are targeted to RemoteComponent+PlayerComponent and sampled
before services, after spawn updates, after remote updates, after appearance application
and after services; known spawn/combined observations also sample. Only changed
fingerprints emit records. They include entity/server ID, Actor/Base IDs and pointer
tokens, races/overlay/sex/weight bits, headparts/count/hash, hair/body color, root/face/head,
WaitingFor3D, private provenance and CachedRefId. Weight is also printed as a float.
The diagnostic store is bounded to256 server IDs per connection; no actor pointers are
retained for dereference. Last valid identity survives missing samples and ECS rebind;
disconnect clears it. INITIAL_OBSERVATION and IDENTITY_UNAVAILABLE are distinguished
from REMOTE_RECREATED and IN_PLACE_MUTATION. The latter means identity was stable;
changedFields must still be inspected to distinguish visual from ECS-only changes.

Creation paths record source/phase and snapshot bytes/tints/flags, but spawn has no
race/sex descriptor field. Actor-created observations bind old/new IDs to a known source.
Native SwitchRace/reset logs retain callSeq/thread/caller RVA and before/after geometry;
new category/base-pointer markers join them to serverId through Actor form and pointer.
No unsafe ECS traversal is added on native hook threads. SexChangeProbe is preserved.
Combined entry is logged before preguards; every failed guard is named. Negative guard
names (notMounted/notWaitingFor3D/noInteraction) print their required truth value.

A changed sample is the first observed state, not proof of an exact engine instruction.
If no instrumented call brackets an in-place change, classify its cause as unresolved
and preserve the tick interval/native logs. Do not infer that Combined ran merely from
a matching binary hash, or blame it when no relevant entry exists in complete logs.


## Native caller provenance and RE constraints

A DoReset3D probe's callerRva records the return address, not the CALL instruction.
The launcher loads the game into its own reserved image and aliases module paths;
callerModule=SkyrimSE.exe alone must not classify an address as native game code.
Resolve caller ranges against the original game and launcher sections/symbols.
The observed0x40B115CE is a ThisCall return; native0x95499B follows a call from
unnamed52391, a player-global helper reached by RaceSexMenu. These are callers of
the same40255 primitive, not evidence of two separate reset implementations.

Do not expose52391 or its menu-specific preparation52409 as a remote Actor API.
The former selects the global player and the latter consumes menu/player state.
No native unknown flag (including animation graph byte+0x242) is assigned FaceGen
semantics without proof. Service and creation-probe update counters are separate
observation domains. Preserve the existing barrier,5B.2 and overlay behavior while
investigating state-dependent rebuild differences. Runtime status remains in STATUS.


## Passive sex-rebuild differential diagnostic contract

SexRebuildDiff extends the existing DoReset3D hook; it never invokes AE39033 for
observation. A per-thread stack frame binds branch events to the innermost matching
actor/callSeq. AE36947 is a narrow opcode1C enqueue observer; AE39395 counts inline
only from native return RVA7270AC. Missing or conflicting evidence is unresolved.
Each hook forwards the original exactly once and preserves Win32 last-error state.
New hook installation requires exact runtime, relocation RVA and RE prologue bytes;
install-attempt alone is not evidence that a hook was activated.

Fields with Known=false are unavailable, even when their numeric placeholder is0.
Biped1/2 denote the first/second DoReset3D operands: the global branch reads the
403521 player's +268/+8F0 only for the verified736110 virtual getter. The ordinary
actor branch reads +268 only for getter727C30; its second biped is unavailable/not
used. Unknown or replaced virtual getters are not called to discover their behavior.
The selected branch is applicable only when updateWeight=true. No biped internals
are sampled. AIProcess offsets are AE1.6.1170 RE observations; byte311 is unnamed.

SexChangeTick and DoResetTick belong to CharacterCreationService probe updates,
not CharacterService serviceUpdate. First-observed sex timing is not exact mutation
time. Server IDs are last service-observed diagnostic associations, bounded to256,
matched by actor pointer/form and cleared on trace reset; absence is explicitly unknown.
The native hooks do not access ECS. No field changes appearance strategy or readiness.


## ActorState interpretation after SwitchRace (AE1.6.1170 RE)

Logged actorStateFlags1/2 are Actor+C8/+CC, through the ActorState subobject at+C0.
They must not be confused with Actor's separate flags at+E8/+204. SwitchRace's native
38989 call resets the entire first word and applies (old2 & FFFF9008)|1008 to the second.
This is broader than selectively clearing41/80 and matches native constructor defaults.

The [CommonLib ActorState layout](https://raw.githubusercontent.com/CharmedBaryon/CommonLibSSE-NG/main/include/RE/A/ActorState.h)
identifies first-word01 as movingBack,40 as walking, and second-wordE0 as a three-bit
weapon state. Thus second-word80 within1088 represents state4, not a standalone
readiness bit. Treat these names as layout semantics, not proof the remote is actually
moving or wielding a weapon at that instant. No field is renamed as head/FaceGen-ready.

A normal clear movement/weapon state may remain clear after SwitchRace. Bit setters,
movement updates and archive restoration are not established head rebuild primitives.
No forced restoration, extra reset or functional delay follows from this diagnostic.
Inspected direct paths lack a41/80 head gate; indirect unexpanded dependencies remain
unknown. The successful same-race control rules out its shared INI/flags/process/biped
values as sufficient explanations individually, without proving a final combined fix.


## Controlled local rematerialization candidate (2026-09-17 audit)

Historical proposal: ADR-0020/0021/0022 supersede this routing and furniture
acceptance scope for initial Character Creation. The current contract is above.

This section specifies future design constraints, not implemented routing. Current
implementation and supplied human validation remain in STATUS.md. The source audit
supports retaining the same ECS entity, Remote.Id, PlayerComponent and ownership
while replacing the local private TESNPC/Actor for race changes. Keep same-race
cosmetic and sex changes on their existing hot paths. Both race-changing branches
are candidates for one shared materializer; neither replacement path is enabled.

Reuse the canonical AppearanceBuffer/ChangeFlags/FaceTints creation sequence shared
with normal spawn. Do not synthesize CharacterSpawnRequest, RequestRespawn,
assignment, ownership transfer or a server lifecycle for a local replacement.
Accept engine-assigned local reference/base IDs; preserve logical identity. Commit
FormIdComponent, CachedRefId and private-base provenance consistently on the game
update path, preserving interpolation and animation differential state.

Before activation, demonstrate candidate isolation from discovery and world side
effects, safe old-actor/private-base retirement, current-state capture/reconciliation,
and furniture occupancy/pose restoration. Actor::Create immediately spawns into
the world; its return is not an invisible staged object. DeleteTempActor/Delete is
the existing retirement boundary, not proof of synchronous deletion or NPC release.
Do not call destructive remote-component removal as replacement cleanup.

Active stays immutable; Latest coalesces without starting a second candidate. A
transaction must retain session/entity/materialization generation and reject stale
callbacks. Existing AppearanceApply::Rebind resets active state and is not a
transaction commit API. Persist current canonical appearance for later recreation;
WaitingFor3D's old SpawnRequest cannot overwrite newer accepted state. Inventory,
build/equipment and animation notifications are not guaranteed full replays.

CharacterCreationService's local phase alone does not establish remote eligibility.
Fail closed without verified participant/phase, private provenance, supported
humanoid race change and compatible world state. Seating is a required table-scene
contract, not an optional visual detail. Solo, recovery, unknown phase, unsupported
actors and unproven gameplay-state transfers must not enter the candidate path.

The detailed evidence, failure matrix and proposed small slices are in local audit
`_audit/character-appearance-controlled-rematerialization-audit.md`. No extra reset,
ActorState restoration, loaded FormID hardcode, beast support or packet is implied.


## Shared private spawn materializer (Slice 1)

The two existing creation callers now share file-local MaterializePrivateRemoteActor
for empty FormId, empty BaseId, IsPlayer=true. The helper accepts canonical appearance,
World/entity for the existing FaceGen setup, and a caller-owned diagnostic callback.
It performs TESNPC::Create, FaceGenSystem::Setup, Actor::Create in that order. Its
result contains borrowed native base/actor pointers; it carries no network identity,
readiness, ownership or rollback guarantee. The original temporary GamePtr is released
at the Actor assignment boundary. NPC initialization/Deserialize/overlay handling and
Actor remote/skip-save/world-spawn internals remain untouched.

Provenance binding (including the existing probe Rebind), player/network components,
placement, WaitingFor3D and later inventory/animation setup remain in their callers.
Existing NPCs, placed actors and non-player custom NPCs stay on separate branches.
FaceGen Setup still mutates the live caller entity and leaves it unchanged for empty
tints. This helper is creation-only; it is not used by AppearanceApply or a replacement
transaction. Staging, lifetime, state replay and seating blockers remain unresolved.


## Dormant discovery/retirement contract (Slice 2A)

RemoteMaterializationLifecycle is a pure model used only by TPTests. Its key contains
connection epoch, full entity identity/version, serverId and generation. It models
one old/candidate pair, publication eligibility and one-shot retirement intentions;
it never performs ECS writes, sends, creation or deletion. Tests receive already
correlated keys. Production FormID-only discovery events do not provide that
correlation, and no runtime routing is enabled by this model.

Future candidate-add must remain associated with its existing transaction without
ordinary assignment. Old removal before commit aborts and reports genuine old-binding
loss; after commit it observes only old retirement. Candidate removal after abort
cannot clear the live old binding. Removal of the new current actor is genuine
binding loss, never an excuse to restore the retiring actor. Authoritative removal
or session end overrides all publication eligibility and retires both known native
materializations. Late creation under a cancelled retained key is cleanup-only.

Correlation must be captured at observation time; looking up the current generation
when an old FormID event is delivered cannot distinguish reuse. Narrow routing must
cover all lifecycle subscribers, including ActorValueService. Unrelated events retain
normal behavior. No global discovery suppression is acceptable.

A timeout ends staging eligibility, not native ownership. Retain matched retirement
records until a proven terminal condition; if capacity is exhausted while completion
is unknown, fail closed rather than forgetting records or starting more candidates.
The pure model deliberately has no Completed/reset transition or timer. Discovery
absence, GetById absence and handle invalidation are distinct observations and none
alone proves private TESNPC release. Existing Delete remains the only audited removal
boundary; no manual free or new native deletion path follows from this contract.

Candidate-local tints/Generated state is structurally possible, but the current
materializer still calls Setup on the live entity and immediately spawns into the
world. Pre-exposure registration, native completion, FaceGen publication, state replay
and seating remain activation blockers. The next evidence step is passive native
lifecycle/correlation observation, not a replacement LAB. See STATUS for current truth.


## Passive native lifecycle observation (Slice 2B)

The existing Debuggers menu or non-MASTER F11 can explicitly arm a default-off
recorder for the next natural private remote player creation, on runtime 1.6.1170 only. This is
diagnostic observation, not a retirement policy. One record holds numeric
identities/tokens and the handle returned by the existing Spawn call; it never
owns an Actor/Base or changes ECS, transport, appearance, creation or deletion.
Disable/re-enable is required to select the next pair. Session IDs are local to
one process and must be combined with that process's log/build identity.

F11 toggles the same IsNativeLifetimeProbeEnabled/SetNativeLifetimeProbeEnabled
state used by the checkbox, on the game update path while Skyrim is foreground.
It works with the debug menu hidden and triggers once per sampled key-down edge,
not repeatedly while held. Neither control adds its own log or probe transition.
After a window ends the enabled state remains true: press/release F11 to disable,
then press/release again to rearm before the next natural creation.

Creation source boundaries capture the Actor before the existing Spawn call;
ordinary discovery dispatch is unchanged. Registry and captured-handle checks
use fresh lookups. Engine-returned pointers are compared only. The root marker
comes from Discovery's existing nonnull-root predicate and means first observed
there, not first native root allocation; removal does not prove a null root.
Known destructor detours forward unchanged arguments/result exactly once. They
read the FormID only from the valid complete-object entry argument, verify its
token/ID, and never access the object after forwarding.

Hooks for AE40288/AE24888 require exact runtime, address/RVA, executable mapping
and 16-byte prologue match. They are attempted only on explicit enable. An
installation log is not proof of execution; only a matched runtime entry is.
Disabling stops observation; installed pass-through detours remain until process
exit. The old destructor stub and TESFormDeleteEvent remain unused.

Polling occurs at most every 100 ms, for 180 s from creation or 30 s from the
first Delete/discovery-remove/server-remove/disconnect/destructor signal. A
deadline or identity mismatch stops correlation without freeing anything.
These diagnostic bounds are not the dormant Slice 2A retirement policy and do
not authorize forgetting a future transaction's outstanding native ownership.

Form absence proves only lookup absence, handle failure only failed resolution,
and discovery removal only scan loss. A scalar deleting destructor return with
its deletion flag follows that native destructor/deallocator path, but does not
prove all callbacks/references/private bases have retired. Native TESNPC ownership
and repeated retirement remain unknown until human runtime evidence. No manual
free, candidate, replacement LAB or additional native mutation is enabled here.
See STATUS for implementation/validation truth and TEST_PLAN for the human run.


### Observe an already bound private remote

Non-MASTER Shift+F11 and Debuggers ->
`Passive native lifetime probe (observe current private remote)` request a
single selection attempt on the next game service update, outside rendering.
This mode requires the probe to be disabled. It counts RemoteComponent plus
PlayerComponent entities before checking private provenance: exactly one must
exist. LocalComponent, WaitingForAssignmentComponent and WaitingFor3D must be
absent; FormId must equal CachedRefId. Fresh native lookups must resolve an Actor
marked remote player and its actual TESNPC base, both distinct temporary FormIDs,
with matching registry pointers and IDs. Known ECS aliases of actor/server/cache
or another bound actor sharing this base reject selection.

Present RemotePlayerAppearanceBaseComponent must match both current IDs. An absent
marker permits a validated-current-binding-fallback for this passive observer only;
contradictory provenance never falls back. This proves a current observable binding,
not exclusive native ownership, allocation history or a materialization generation.
Root/face/head presence is not an identity prerequisite; the existing no-node-read
observer retains unknown discovery/root history. Rejected attempts do not enable next mode, wait for a future spawn, replace an
existing observation or retry automatically.

Successful selection writes the same passive record and starts the same 100 ms,
180 s / 30 s window at observation start. It saves serverId, packed entity/version,
Actor/Base IDs/tokens and, if available, a matching already published high-process
handle. Handle resolution is temporary and followed by registry revalidation;
no returned handle pointer is dereferenced and no GetHandle is called. Initial
discovery/root history remains unknown; no creation events are synthesized.
The current selection emits `armed-current-private-remote`, then
`current-private-remote-evidence` with `selectionEvidence=durable-provenance` or
`selectionEvidence=validated-current-binding-fallback`, `provenancePresent`, session,
server/entity and Actor/Base IDs, then the original `current-private-remote-selected`
record including address tokens. The requested durable-provenance label means a
matching allocation marker is present NOW; it promises no persistence across recovery.
Failure emits `current-private-remote-rejected`
with its reason. F11 disables either mode with the existing disabled log. The
checkbox reflects the shared enabled state; its unchecked-to-checked action
continues to arm next-private mode. Shift+F11 never silently cancels that mode.


### Appearance provenance is not durable materialization identity

ApplyAppearanceSnapshots intentionally clears both RemoteAppearanceProbeComponent
and RemotePlayerAppearanceBaseComponent when transport is disconnected OR the
campaign runtime gate is locked. The original phase 4B contract invalidates appearance
state on disconnect, recovery lock and remote teardown. Creation paths bind the
marker only after private allocation; unlocking does not restore it for a live actor.
The 18 September log shows this after OnCharacterSpawn and after REMOTE_RECREATED,
with the same Actor/Base tokens through each clear and a subsequently ready binding.

The controlled-rematerialization plans above must not treat that marker as durable
identity. Rebinding it at a future commit alone does not cover a later recovery clear.
This is a runtime integration blocker: a separate materialization identity/generation
source with explicit retention/invalidation across recovery must be designed before
wiring the dormant Slice 2A model. The passive fallback neither supplies that contract
nor authorizes ownership, Delete, replacement or appearance mutation. No production
clear, provenance lifecycle, AppearanceApply operation or network behavior changes here.
