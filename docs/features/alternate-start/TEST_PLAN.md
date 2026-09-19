# Alternate Start — Test plan

> **Status: Automated audits plus M7, New Game, MQ101/post-Helgen, standalone T+4, pre-deadline wounded-survivor, and focused campaign-bootstrap checks executed; the campaign bootstrap and multiplayer T+4 occupied projection pass their validated runtime paths, while the remaining permutation matrix stays pending**

## Automatic final activation - ADR-0023 acceptance (2026-09-18)

Initial Character Creation has no live remote appearance synchronization. The
matching pending authoritative Applied build publishes one canonical final after
build seal; observers automatically rematerialize through the shared natural-join
path in every build, including MASTER. No Ctrl+F11 or debug activation is required.
Post-creation appearance changes and collective seating remain out of scope.

| ID | Required automated evidence | Human acceptance boundary |
|---|---|---|
| T1 | Matching Applied/final revision reaches the transaction with no enabled input; official trigger and duplicate fences retained. | Fresh A+B final rematerialization without a toggle. |
| T2 | Functional publisher/receiver/policy have no Enabled/SetEnabled state; functional menu removed; Ctrl+F11 cannot arm the passive probe. | No debug step in the procedure below. |
| T3 | Functional helpers, native hooks, materializer and publisher survive MASTER preprocessing; isolated C++ compilation with IS_MASTER=1 supplements source checks. | Full MASTER runtime acceptance remains separate. |
| T4 | Startup emits final-rematerialization-enabled, source=official-character-creation-default; no opt-in state exists. | Both clients report this on startup. |
| T5 | Matching pending Applied is the sole event producer; no menu/tick publisher. | Intermediate closes and ModifyRace remain local. |
| T6 | Duplicate Applied cannot publish after the pending flag is cleared; manual menu is not a producer. | Later showracemenu publishes nothing. |
| T7 | Disconnected/Solo eligibility rejects; publisher cannot queue/send without transport. | Solo creation succeeds without appearance traffic or remote work. |
| T8 | Legacy hot dispatch remains unreachable; candidate failure preserves valid old binding and never invokes SwitchRace/Reset3D. | Failure has a precise rejection/abort, with no hot fallback. |
| T9 | Versioned entity, serverId/PlayerId/ownership and animation/interpolation identity preserved; only native binding changes. | Correlate old/new Actor/Base and unchanged logical identity. |
| T10 | One shared canonical bytes/flags/tints materializer; natural spawn callers and race-independent projection unchanged. | Full race/sex/cosmetic/no-change matrix remains pending. |
| T11 | Exact-runtime, binding, native safety, canonical, capacity, readiness and lifecycle tests retained; no sit/sleep veto. | Invalid binding/state/geometry must still fail closed. |
| T12 | Recovery/disconnection abort or defer cleanup; generation/tokens quarantine candidate and retirement observations. | Coordinated recovery/reconnect matrix remains pending. |

Run TPTests, the six structural suites listed below, client/server/launcher builds
and git diff --check. MASTER source checks are not a full MASTER executable build
or game run; record the exact scope of any additional compilation in STATUS.
Papyrus is unchanged by this promotion: compare PSC/PEX hashes to the verified
checkpoint and PEX bytes to its retained compiler output; recompile only if changed.
Current implementation and validation results are recorded in STATUS, not inferred
from this acceptance matrix. The maintainer subsequently confirmed automatic
final rematerialization without debug activation on the tested multiplayer run;
see STATUS. The remaining permutations and full MASTER runtime remain pending.

## Final-rematerialization runtime checkpoint and regression gate

The maintainer's successful 2026-09-18 A-observes-B run is recorded in STATUS,
including the exact Actor/Base change, logical identity, candidate commit and
human visual acceptance. It is one tested scenario, not full matrix acceptance;
no exact race/sex direction or reverse transition is inferred from the handoff.
The same trace's +30 s registry presence is explicitly inconclusive for lifetime.

Before a feature checkpoint, all relevant structural suites must pass, including
the actual tracked PSC and PEX. Fragment_0/4 both call BeginCharacterCreation;
no seat MoveTo or GetSitState gate may precede stage 20. Recompile the current
PSC, compare tracked PEX with that output, and run CK packaging/MQ101 audits.
The finalization restores the drifted pair; the previous 56/57 result is history,
not an accepted exception for this checkpoint. TPTests and client/server/launcher
builds must also be green. Never include logs/audit/build output in the commit.

The next human runs still need:

- Repeated local rematerialization cycles and bounded observations of native
  Actor/private TESNPC lifetime; Discovery absence is not destructor completion.
- Recovery/reconnect and cancellation permutations under ADR-0018.
- Full race/sex/cosmetic/no-change coverage, including explicit directional
  observations; every valid race uses the same natural-join path.
- A separate future collective seating phase, outside this checkpoint.

The earlier procedures below remain regression instructions. Their diagnostic
mission results are historical; current implementation/validation truth is STATUS.
No new Skyrim execution is required to prepare this already-tested checkpoint.

## LAB DontMove veto correction - preceding diagnostic mission

The latest supplied run isolates unsafe-actor-state for 01200041 / 00001008.
Exact-build RE identifies life=9 as SetDontMove; every other tested state field
is idle. The LAB now permits only this additional life value. No actor state
is rewritten and no posture-dependent condition is added.

Automated checks must cover the supplied pair, every life value 0..15, every
knock/attack/fly/weapon/recoil field value, stagger/sprint/swim, combined hazards,
and all posture field values. Exhaust the former combined 14-bit mask and prove
only DontMove is newly admitted. Structural checks require gate and log decoding
from the same observed words at capture and all later LAB safety boundaries.

Human runtime procedure (no game launch or deployment by the coding task):

1. Use the matching rebuilt artifacts, fresh A+B client processes and a fresh
   campaign; keep the existing required posture-independent bootstrap PEX.
   Final rematerialization is now automatic on publisher and observer (ADR-0023).
   Intermediate RaceMenu closes remain local until final build sealing.
2. Seal the final build. For the reported pair, expect actor-state-accepted with
   lifeState=9 lifeName=dont-move stateVeto=none safeState=1, all other decoded
   state fields idle, then gate-accepted, transaction-reserved and
   candidate-create-enter if the other existing guards still pass.
3. If capture rejects native state, the SAME gate-rejected line must include
   detail=unsafe-actor-state, stateVeto=<specific reason>, lifeState/lifeName,
   knockState, attackState, flyState, weaponState, recoilState, staggered,
   sprinting, swimming and each named predicate result, alongside the existing
   binding/recovery/raw-word evidence. stateDecode=unavailable means there was
   no native Actor to read; it must not be interpreted as an idle Actor.
4. A later native state veto must produce actor-state-rejected at old-binding,
   precommit or post-placement, with its decoded reason before the usual abort.
   DontMove combined with an attack/knock/recoil/stagger must still reject.
   A fly/weapon/sprint/swim reason denotes an unchanged conservative LAB limit,
   not proof that this locomotion or pose inherently makes deletion unsafe.
5. Preserve full A/B tp/_client.log and server logs immediately after the terminal
   rejection/abort/complete, before restarting or allowing log rotation. Include
   artifact hashes and the sequence from final-snapshot-received onward. A green
   pure gate test does not establish native candidate success or final rendering.

The ActorState mission left the recorded PSC drift unchanged. The subsequent
finalization restores/recompiles the tracked pair and requires that test to pass;
its earlier failure remains visible in the historical evidence.

## Runtime rejection diagnostics - preceding placement/binding mission

The supplied current run failed at move-validation-failed and at the LAB's old
unsafe-or-inconsistent-current-binding aggregate. Do not mix an older
player-not-standing log into this run. The matching binary proves deferred player
movement; the subsequent LAB trace now identifies life=9 (see the correction above).

Automated coverage adds deferred position/cell readiness, five-second timeout,
non-finite position rejection, one MoveTo only, update-driven observations,
identity/campaign/recovery cancellation and complete diagnostic fields. LAB
source checks cover every named current-binding predicate and its observation.

The preceding diagnostic suite detected PSC drift back to seated entry fragments
and preserved that file's baseline hash. Finalization now repairs the drift and
requires the unchanged bootstrap test to pass. The earlier failure must not be
hidden or reclassified as a native diagnostic regression.

Next native diagnostic run (human execution):

1. Retain the exact artifact versions/hashes and complete logs for both clients
   and server. The required posture-independent PEX remains a runtime prerequisite;
   no Papyrus or game installation is deployed by this mission.
2. After bootstrap authorization, expect standing-index-resolved, then
   standing-move-observation result=pending or reached. Each observation contains
   targetPosition, actualPositionBefore, actualPositionAfter, distance,
   targetRotation, actualRotation, targetCell, actualCell, actor/token/validity,
   elapsedMs and sample. Pending reports occur at most every 250 ms.
3. Within five seconds, a matching cell and position within 32 units produces
   standing-position and then Race menu requested. A timeout/fence failure logs
   result=rejected and standing-position-rejected reason=move-validation-failed
   detail=<exact reason>. No extra MoveTo or posture operation should occur.
4. Seal the build using the automatic final flow described below. If capture fails,
   gate-rejected detail=<exact guard> replaces the old aggregate. Inspect the same
   line's FormId/CachedRefId, Actor/Base IDs and tokens, entity/version,
   WaitingFor3D, Local/Assignment, recoveryState/recoveryLocked, actor flags and
   safeState. -1 means a native result could not be observed, not false.
5. Distinct reasons include remote/player-component-missing, formid-missing,
   cachedref-mismatch, actor-lookup-failed, actor-not-remote-player, base-not-tesnpc,
   base-lookup-mismatch, waiting-for-3d, assignment-pending, local-component-present,
   entity-alias-collision, base-alias-collision, actor-disabled/dead, combat, mounted,
   unsafe-actor-state, cell-missing, root-not-ready and missing interpolation/animation.
   Recovery and disconnect emit abort detail=recovery-locked or transport-disconnected.
6. Save full logs immediately on rejection (before rotation/relaunch). No guard
   should be relaxed until the actual failing detail and values have been reviewed.

## Temporary standing placement by PlayerId - regression

The local placement contract ignores SlotId and maps the lexicographically sorted
sealed roster PlayerIds to creationPositionIndex 0..9. Solo uses 0; A+B must occupy
0/1. This temporary rule does not relax campaign/wire/admission SlotId validation.

standing_creation_campaign.cpp uses SQLite-backed CampaignAdmissionService,
CampaignLobbyDirectory, roster sealing, snapshot wire round-trip and bootstrap
authorization before invoking the same selector as native placement. Both players
retain their campaign admission; it no longer determines this visual index.
Changing only SlotIds to empty, identical or malformed values in the selector's
input must preserve the result. A reversed ten-player roster must yield exactly
0..9, without changing canonical roster order. Missing local/roster PlayerIds,
duplicate local or other PlayerIds, or local identity outside the roster reject.

test_standing_placement_diagnostics.py requires the durable authentication identity,
no SlotId/GetAdmission/whole-snapshot validity dependency in placement, Solo index
zero, current-cell checks and a diagnostic for each failure. Existing campaign
admission tests continue to enforce SlotId semantics outside this step. Native
Skyrim movement is not executed by these tests.

For the next A+B run, use the rebuilt client/launcher and the already restored
posture-independent PEX. Use a fresh campaign. Expected native logs:

1. campaign bootstrap authorized; validating standing placement
2. standing-index-resolved source=sealed-roster-player-id, same campaign/revision,
   distinct durable playerId and creationPositionIndex 0/1
3. standing-position playerId=... creationPositionIndex=... rosterCount=2,
   then Race menu requested

Solo expects creationPositionIndex=0 and rosterCount=1. Extend acceptance to ten
players: indices 0..9, stable if roster order changes. Preserve the full client
logs on any failure, especially standing-position-rejected reason=....

| Reason | Meaning |
|---|---|
| missing-local-player-id | Durable local authentication identity unavailable/empty. |
| missing-roster-player-id | A sealed member has no PlayerId. |
| duplicate-player-id | At least two roster members have the same PlayerId. |
| local-player-not-in-roster | Local durable identity is absent from the sealed roster. |
| sealed-roster-unavailable | Snapshot absent, unsealed, or empty roster. |
| missing-campaign-id | The canonical snapshot has no campaign identity. |
| campaign-phase-not-character-creation / campaign-runtime-not-active | Canonical phase/runtime is ineligible. |
| sealed-roster-incomplete | A required member is absent. |
| creation-position-index-out-of-range | More than ten members or index outside 0..9. |
| player-missing / creation-quest-missing | Native player or quest unavailable. |
| creation-marker-missing | Plugin-local marker could not resolve to a live reference. |
| creation-marker-reference-mismatch / creation-marker-cell-missing | Stale reference or missing loaded current cell. |
| player-cell-missing / player-marker-cell-mismatch | Current player cell missing or different from anchor cell. |
| creation-marker-transform-invalid | Marker position or rotation is non-finite. |
| move-validation-failed | See detail and standing-move-observation: invalid identity/context, non-finite position, or cell/position timeout after at most five seconds. |

Success is logged only after final validation. The existing ten anchors and
offsets remain, without furniture activation or an arbitrary multiplayer fallback.
No posture field participates in placement; the standing-position label is
historical. final_respawn.cpp exhausts all 16 sit/sleep field values, accepting
each when independent guards pass and rejecting independent safety failures for
each posture. Structural checks prohibit furniture gating/normalization in LAB,
native placement and both Papyrus entry fragments. Position tests cover finite
nearby coordinates, excessive drift and non-finite actual/target coordinates.
Natural-join materializer/projection and logical identity remain unchanged.

## Slice 3 natural-join correction - acceptance

ONE final follows sealing of the entire build. All earlier RaceMenu closes remain
local. Entry and final rematerialization are independent of posture; collective
seating is a separate future slice after all players complete creation. The corrected native/CK flow has no game runtime acceptance yet. Automated
checks establish source/policy/model behavior, not native rendering or lifetime.
Current execution results belong in STATUS and the corrective A-K audit.

| ID | Automated evidence | Human acceptance still required |
|---|---|---|
| T1 | Nord final, both sexes: common validation, eligibility and lifecycle model. | Correct new Actor/Base, body/head/tints. |
| T2 | Orc final, both sexes: identical policy/model. | Same path and correct visual result. |
| T3 | Khajiit final, both sexes: identical policy/model. | Same path, correct body/head/tints. |
| T4 | Argonian final, both sexes: identical policy/model. | Same path, correct body/head/tints. |
| T5 | Source check rejects racial allowlist, race names/IDs and beast/humanoid gate in LAB/shared policy/projection. | No race-family rejection in logs. |
| T6 | Source: no transition classifier; appearance bytes/flags/tints only enter shared materializer. | Same process for cosmetic, sex-only, race-only and combined edits. |
| T7 | Source/model: unchanged full ECS key/server/Player; only native binding commits. | Correlate identities before/after and late old removal. |
| T8 | Source: shared materializer, Initialize, Place, Complete3D, ReadyFor3D in natural paths and LAB. | Match natural-join rendering, native state, inventory and equipment. |
| T9 | Source: no lifecycle/assignment/ownership send; candidate Discovery fenced before subscribers. | Correlated server/client traces show no synthetic network lifecycle. |
| T10 | Source: hot receiver/applier bypassed; only matching pending Applied publishes once. | RaceMenu edits/closes/reopens remain local until seal. |
| T11 | All 16 sit/sleep values accepted independently of safety guards; no furniture eligibility check or normalization; both CK fragments share one placement helper; sorted PlayerId ranks and ten anchors. | Seated/standing/transition states do not gate entry or final rematerialization; valid cell/position; no forced furniture exit or chair restoration. |
| T12 | Source: official pending Applied is the sole publisher. | Later manual showracemenu never publishes/rematerializes. |
| T13 | Superseded by ADR-0023 activation T1-T4 above: automatic in all builds; no functional toggle. | Fresh A+B run without Ctrl+F11 remains required. |
| T14 | Policy/source: disconnected/Solo never publishes or starts a transaction. | Solo creates a local character regardless of posture, with no remote work. |

Additional regression coverage: stale generation/session/entity, candidate failure,
recovery lock and timeout preserve the valid old representation; late old removal
cannot strip the candidate; canonical live counts and worn/magic equipment are
checked; duplicate delivery cannot reserve/commit twice. Old Slice 3 T1-T16 IDs in
historical reports remain historical and are not this corrective matrix.

Run TPTests and these Python suites in Tools/Scripts:
test_natural_join_rematerialization.py, test_remote_respawn_lab.py,
test_private_remote_materializer.py, test_remote_materialization_lifecycle.py,
test_native_lifetime_probe.py, test_standing_placement_diagnostics.py. If Papyrus
changes, compile the fragment to the tracked PEX; otherwise verify checkpoint
hashes and retained compiler output. Run audit_ck_packaging.py and
audit_mq101_quickstart5.py against the ESP.
Build client/server/launcher and run git diff --check. No game is launched by tests.

### Human automatic-final procedure (not executed by this promotion)

1. Use a NEW campaign, fresh client processes A/B, matching schema-2
   server and rebuilt client/launcher plus the verified Alternate Start PEX. Keep
   the existing ESP. This mission does not install any of these artifacts.
2. Check the new entry: Papyrus emits `[STRE][AlternateStart] Creation placement
   ready; entering stage 20`. Both legacy stages 10/11 use this helper; stage 20
   has no GetSitState condition. The lobby opens after placement at the common
   start marker. After sealed-roster authorization, EACH client must log
   `[STRE][CharacterCreation] phase=standing-position playerId=... creationPositionIndex=... rosterCount=...` before
   RaceMenu. Distinct durable PlayerIds must select distinct positions. Check feet,
   floor, furniture clearance, camera and controls for both rows; later extend to
   all ten slots. Solo must select its first position. Missing/duplicate PlayerId,
   marker, anchor/cell or invalid position must fail; posture alone must not.
3. Do not enable any debug gate. On EACH client, expect the startup log
   `[STRE][RemoteRespawnLAB] phase=final-rematerialization-enabled
   source=official-character-creation-default`. Ctrl+F11 has no activation role;
   the former functional menu item is removed. F11/Shift+F11 remain optional
   non-MASTER passive-probe controls, unrelated to final activation.
4. First run: A chooses Orc, B chooses Khajiit (or reverse). Change race/sex and
   cosmetics locally; close RaceMenu, use ModifyRace and reopen. Observers must
   keep the old appearance. No final snapshot/create occurs before final sealing.
5. Seal each complete build. Publisher logs authoritative build finalization and
   AppearanceTrace Publisher captured/send-attempt/sent. The observer should log
   final-snapshot-received, gate-accepted, transaction-reserved,
   old-binding-captured, candidate-create-enter/return, inventory-restore-status,
   candidate-ready, candidate-commit, old-retirement-requested and complete.
   No seat-restore-status is expected. `complete` establishes local binding commit,
   not native destruction. Rejection/abort must never fall back to hot apply.
6. A must see the final Khajiit; B must see the final Orc. Check exact appearance,
   correct position from the initial spawn, rotation/cell/world, alive/enabled
   state, canonical inventory and equipment; seating is not a success criterion. Compare same serverId,
   PlayerId, versioned ECS/Remote.Id, new Actor/Base and materialization generation.
   Old removal must not strip the new binding. No SwitchRace/Reset3D, assignment,
   ownership transfer or server-side spawn/despawn should be attributable to this
   operation. No final collective seating is implemented in this mission.
7. Keep the connection stable after retirement. In non-MASTER, wait at least
   30 seconds and retain the additional retirement-window observation. MASTER
   retains transaction logs but omits that passive lookup window. Save logs after
   completion or immediately on failure/crash, BEFORE relaunch/rotation. Collect the complete A/B
   `Data/SkyrimTogetherReborn/logs/` directories, especially tp_client.log and
   tp_client.1.log through tp_client.3.log, server logs and any crash/Papyrus logs.
   On this machine client logs are under `C:/Program Files (x86)/Steam/steamapps/
   common/Skyrim Special Edition/`. Record timestamps, artifact hashes and chosen
   races/sexes. A filtered console excerpt is insufficient.
8. Restart client processes between independent creation attempts: attempted
   finals and tombstones remain process-scoped; there is no reset toggle. Repeat
   Nord/Orc/Khajiit/Argonian, both sexes, unchanged,
   cosmetic-only, same-race sex change, race-only and combined changes. Exercise
   duplicate final, abort, recovery/disconnect, automatic activation, Solo and later manual
   showracemenu. The latter is out of scope and must not publish a new final.

If Khajiit/Argonian fails, compare natural join with the EXACT SAME final canonical
snapshot first. Natural join success means a LAB projection difference to fix.
Only failure in both paths supports investigating capture/payload compatibility.
Do not add a race-specific replacement branch as a shortcut.

Repeat entry and final rematerialization with seated, normal and transitional
sit/sleep states (including WantToStand), and with furniture Interaction present:
none may independently reject or cause a posture wait. Verify no forced exit,
furniture activation, ActorState normalization or seat restoration occurs. A real
cell/position/binding failure still rejects; keep its full diagnostic log.
Mounted/combat/ragdoll/assignment/WaitingFor3D/inconsistent bindings still reject.
Native staging can briefly expose both actors; atomic visual replacement is not
proven. Physical placement clearance and private TESNPC end-of-life remain runtime
questions. Base registry persistence alone proves neither leak nor completion.

## Completed checks

- successful Windows xmake build;
- conforming strict audit of 63 STRE-owned CK records plus the explicit
  Skyrim-master override allowlist;
- conforming audit of 41 catalog/ESP references;
- compiled `Code/tests/character_build.cpp` tests;
- in-game Mage bootstrap test;
- targeted buffs tested between two PCs;
- single-player fallback present in the service and tested through the build flow;
- New Game interception validated for first and same-process second New Game;
- MQ101 structural and generated-fragment invariant audits conform;
- post-Helgen MQ101/world-state projection validated in game after xEdit Quick
  Auto Clean while MQ102/MQ102A/MQ102B remain untouched;
- native `TPTests` pass with 126 test cases and 1753 assertions, including the
  focused join-code directory, wire validation, opcode stability, native CEF
  binding manifest/action routing, bounded Unicode lobby-pseudo validation, and
  one-shot local campaign-bootstrap gate tests;
- the seven focused Playwright campaign-bootstrap scenarios pass: entry/create,
  entry/join and code normalization, Solo intent, shared persisted connection
  address without password persistence, lobby projection/Start authority,
  disconnected form/back behavior, and required/trimmed/bounded Unicode pseudo
  handling across the five-argument native action contract;
- pre-deadline Helgen investigation bootstrap validated with independent Hadvar
  and Ralof wounded positions and dedicated wounded packages;
- bidirectional rubble-squeeze interaction validated in game with the expected
  prompt, fade, and local player transfer;
- investigation quest exclusion from generic quest synchronization builds
  successfully;
- TPTests pass with 1837 assertions in 137 test cases;
- CK packaging audit passes with 19 managed files and zero compiled PEX files
  under `Scripts/Source`;
- `git diff --check` passes for the current increment.

These checks do not constitute exhaustive validation of every combination.

## Static audits

```powershell
py -3 .\Tools\Scripts\audit_stre_plugin_records.py `
  .\GameFiles\Skyrim\STRE_AlternateStart.esp `
  --manifest .\docs\features\alternate-start\CK_RECORDS_M7_IMPLEMENTED.json `
  --output .\_audit\STRE_AlternateStart.records.m7.tsv `
  --strict `
  --reject-unexpected
```

```powershell
py -3 .\Tools\Scripts\audit_mq101_quickstart5.py `
  .\GameFiles\Skyrim\STRE_AlternateStart.esp
```

```powershell
py -3 .\Tools\Scripts\audit_mq101_generated_invariants.py `
  .\GameFiles\Skyrim\Source\Scripts\QF_MQ101_0003372B.psc
```

```powershell
py -3 .\Tools\Scripts\audit_character_build_catalog.py `
  .\GameFiles\Skyrim\STRE_AlternateStart.esp `
  .\Code\common\CharacterCreation\CharacterBuildCatalog.cpp `
  --client-source .\Code\client\Services\Generic\CharacterCreationService.cpp
```

## Local smoke test

```text
resetquest STRE_QUEST_AlternateStart
startquest STRE_QUEST_AlternateStart
setstage STRE_QUEST_AlternateStart 10
```

Verify RaceMenu, UI, preview, summary, level 1, exact inventory, equipment, exact spells, cleanup, and absence of hash rejection.

## Mage matrix

Test all nine combinations:

- Fire × Protection;
- Fire × Exploration;
- Fire × Matter;
- Frost × Protection;
- Frost × Exploration;
- Frost × Matter;
- Shock × Protection;
- Shock × Exploration;
- Shock × Matter.

Each build must produce exactly 7 canonical spells: 3 Destruction plus 4 Alteration.

## Priority multiplayer tests

- identical client/server/plugin versions on both PCs;
- independent creations with different choices;
- Accepted, then Applied states;
- no `RejectedInventoryHash` or `RejectedSpellHash`;
- remote appearance and equipment;
- Mineral Aegis: `DamageResist` increases, then returns;
- Water Breathing: effect active, then expires;
- Lighten Burden: `CarryWeight` increases, then returns;
- no application to the wrong target;
- controlled recasting and stacking.

The three spell names above are translated descriptions of currently localized French display strings; runtime IDs remain unchanged.

## Class regression tests

- Warrior: heavy equipment, weapons, smithing, pendant;
- Thief: outfits, weapons, 10 lockpicks;
- Mage: visual outfit, 7 spells;
- build change before acknowledgment;
- reject a second build after the Applied state.

## New Game bootstrap regression

Validated on 15 August 2026:

- first New Game enters the inn and reaches Character Creation;
- during the initial character-creation bootstrap, MQ101 remains at stage 0
  and the STRE Alternate Start quest reaches stage 20;
- a second New Game works after returning to the main menu without restarting Skyrim;
- an ordinary existing save loads without retriggering the bootstrap.

## Post-Helgen continuity regression

Validated on 16 August 2026:

- MQ101 reaches stage 1000 and stops;
- MQ102, MQ102A, and MQ102B remain untouched;
- the cleanup helper reaches its completion marker;
- Helgen exterior remains destroyed;
- entering `HelgenKeep01` shows already-collapsed rubble without replaying the
  vanilla proximity collapse or dragon/collapse roar;
- the accepted minor limitation is a brief rubble sound during Keep load.

## Campaign-bootstrap runtime evidence and pending matrix

Run from a fresh Skyrim process unless the scenario says otherwise. Retain the
matching client and server logs; automated tests are not evidence that these
runtime scenarios passed.

Runtime prerequisite: disable `Alternate Start - Live Another Life` whenever
`STRE_AlternateStart.esp` is active. The two alternate-start implementations
were observed to be incompatible during this validation. This slice does not
add compatibility behavior.

### Executed happy-path evidence — 23 August 2026

The following observations, and only these observations, are recorded as passed:

- the Solo bootstrap works;
- Create Campaign works on the first PC;
- a second PC joins that campaign with its four-character code;
- both players progress through Character Creation and arrive in the STRE inn;
- the persisted last server address is reused;
- the transient creator/joiner pseudos appear correctly in the lobby.

### A — Solo regression — validated scope

The Solo bootstrap was exercised successfully. No additional negative Solo
scenario is claimed by this evidence.

### B — two-PC Create/Join happy path — validated scope

PC A created a campaign and PC B joined it with the displayed four-character
code. Both selected pseudos were displayed correctly, the saved server address
was reused, and both players completed Character Creation and arrived in the
STRE inn.

On a future repetition, retain server evidence for the create/join/start
operation and result, internal campaign/revision context, PartyService
alignment/leadership decision,
and the sealed `CharacterCreation` snapshot. Retain client evidence for
the bootstrap transition, lobby projection, canonical authorization, and a
single CharacterCreation release. Logs must not contain passwords.

### C–L — pending manual scenarios

- C: A creates, B then C join, and Start seals exactly those three members.
- D: an unknown valid-shaped code is rejected without roster/party mutation or
  CharacterCreation release.
- E: a valid code entered in lowercase is normalized and joins the same lobby.
- E2: empty, control-containing, or over-24-code-point pseudos are rejected
  before connection or party/campaign mutation; surrounding whitespace is
  trimmed and ordinary Unicode pseudos are preserved.
- F: after A+B seal, C cannot reuse that code and the sealed roster is unchanged.
- G: B disconnects before Start; A cannot seal/progress until B legitimately
  reconnects/resumes, after which Start succeeds.
- H: B cannot force Start through a manipulated UI or direct request.
- I: with two simultaneous lobbies, B joins A's code and D joins C's code with no
  campaign or transient-party cross-wiring.
- J: wrong password, unreachable server, and protocol mismatch keep the mandatory
  bootstrap active and never release Character Creation.
- K: a second New Game in the same process reopens a clean bootstrap.
- L: loading an ordinary existing save does not open the bootstrap.

For each pending scenario, retain the operation/result, internal campaign and revision
where available, transient/durable player context allowed by logging policy, and
PartyService decision. Confirm the roster is unchanged and no password is logged.
None of C–L is claimed as manually passed by the happy-path run above.

## Pre-deadline Helgen investigation regression

Validated on 20 August 2026:

- project post-Helgen continuity first and confirm MQ101 reaches stage 1000;
- start `STRE_QUEST_HelgenInvestigation` and advance its diagnostic stage 10;
- verify Hadvar, Ralof, and both wounded-marker aliases are filled;
- verify Hadvar and Ralof reach their independent intended locations;
- verify Hadvar uses the Wounded02-based furniture pose and Ralof the
  Wounded03-based furniture pose;
- verify neither actor immediately resumes an incompatible vanilla travel
  package;
- verify `Se faufiler` appears only around the intended rubble opening;
- traverse the rubble in both directions and verify fade, destination, and
  immediate movement after arrival;
- verify the dead bandit and pickaxe do not obstruct the interaction;
- verify the strict record audit reports 63 expected records with no unexpected
  master override;
- verify `audit_ck_packaging.py`, client build, TPTests, and
  `git diff --check`.

Still required for this slice:

- save/load while the investigation is active;
- repeat/reapply stage-10 projection and confirm safe behavior;
- cell reset to determine whether the current vanilla corpse ActorBase respawns;
- multiplayer traversal with two players using the squeeze independently.

## Standalone T+4 Helgen occupation regression

Validated on 23 August 2026:

- project the clean STRE post-MQ101 baseline and verify Helgen is destroyed and
  burning, skipped MQ101 actors are absent, and occupation bandits are absent;
- start `STRE_QUEST_HelgenInvestigation`, leave both survivors in
  `WoundedInCave`, and advance four full game days while the player remains in
  `HelgenLocation`;
- verify the deadline moves `HelgenWorldPhase` to
  `BanditOccupationPending` without introducing occupation bandits or moving the
  survivors while the player remains in Helgen;
- leave Helgen, wait for the standalone presence re-evaluation, and verify the
  transition commits to `BanditOccupied`;
- verify Bethesda's post-Helgen occupation appears in the exterior and
  `HelgenKeep01`, the major post-attack fire/smoke FX retires, the collapsed
  bridge and its debris remain projected, and the STRE `Se faufiler` traversal
  is no longer active;
- verify both survivors still in `WoundedInCave` transition to
  `CapturedInKeep`, move to their respective STRE jail markers, use the captured
  package, and have their selected vanilla jail doors closed and locked;
- verify strict CK audit with 67 expected STRE-owned records and no unexpected
  master overrides, CK packaging with 17 managed files, client build, TPTests
  (1552 assertions / 116 test cases), and `git diff --check`.

Still required for the T+4 slice:

- reach the four-day deadline while already outside the affected Helgen footprint
  and verify the direct `RecentPostAttack -> BanditOccupied` path;
- verify mixed survivor states, especially `Freed + WoundedInCave`, and prove
  that `Freed` never regresses to captivity;
- verify `Departed` non-regression once that projection exists;
- save/load before the deadline, while `BanditOccupationPending`, and after
  `BanditOccupied`, then verify idempotent reprojection;
- cell-reset regression for the bandit occupation, jail doors, survivor
  projection, and current rubble-excavator corpse;
- run the multiplayer vertical-slice matrix below;
- verify collective checkpoint/recovery once the campaign recovery path is
  implemented; Helgen has no separate state replay mechanism.

## Multiplayer T+4 Helgen vertical slice

Native and protocol automation validated on 23 August 2026:

- exact sealed roster and `ACTIVE` gate are required;
- readiness and spatial notification messages round-trip through their
  factories;
- the generic group evaluator passes 1-player, all 2-player inside/outside
  combinations, N-player last-exit, exact interior/exterior, unknown-position,
  missing-member, empty-footprint, and closed-gate cases;
- client and server builds pass; TPTests pass 1837 assertions in 137 test cases.

Runtime revalidation on 24 August 2026 confirmed the final occupied projection
in standalone and in a multiplayer campaign: the readiness/admission path
authorizes the transition, major post-attack FX retires, occupation encounters
appear, the temporary STRE traversal retires, and the collapsed bridge/debris
projection remains intact. The following additional permutations remain manual;
this runtime evidence does not mark them as passed:

1. start the investigation on A and B, verify neither local T+4 clock arms until
   both have crossed the collective start barrier;
2. at T+4, move A outside while B remains in any exact Helgen cell and verify
   both remain `BanditOccupationPending` with no physical occupation;
3. move B outside and verify both clients call the existing local
   `CommitBanditOccupation()` projection within the five-second convergence
   interval;
4. repeat with B leaving before A;
5. repeat with both players already outside when T+4 arrives and verify direct
   occupation on both;
6. repeat with one player in `HelgenKeep01` and one in a Helgen exterior cell;
7. disconnect either required member before commit and verify the remaining
   client receives no progression authorization;
8. verify `Freed` and `Departed` never become `CapturedInKeep`, and confirm no
   `STRE_QUEST_HelgenInvestigation` stage packet is emitted.

## Tests still blocked by missing features

Other blocked coverage:

- rescue/liberation and `Freed`/`Departed` survivor projections;
- neutral MQ102/MQ103 vanilla main-quest handoff;
- Valen and scene;
- exit and vanilla resumption;
- save/load at every campaign phase;
- sealed roster, coordinated checkpoints, disconnect recovery, and collective
  build/campaign restoration;
- 4 and 10 players.

## Future campaign continuity matrix

Roster and activation:

- extra player after roster seal is rejected;
- replacement player and wrong `CharacterBinding` are rejected;
- activation with one missing roster member is rejected;
- full roster activates only with every expected slot/binding.

Disconnect boundaries:

- disconnect during open world;
- disconnect during combat;
- disconnect during dialogue or a scene;
- disconnect around a quest-stage change;
- disconnect during checkpoint creation;
- no persistent campaign mutation or new checkpoint is accepted while in
  `RECOVERY_LOCK`.

Checkpoint failure and restore:

- one player save failure;
- client crash during a candidate checkpoint;
- server interruption before and after the checkpoint commit boundary;
- a failed candidate leaves the previous committed checkpoint valid;
- all clients load the exact checkpoint and per-slot save selected by the server;
- wrong, stale, missing, or mismatched save metadata is rejected;
- restored server revision matches the selected checkpoint;
- resume occurs only after all roster members acknowledge the same restore;
- retry and duplicate/delayed acknowledgements are idempotent;
- no duplicate build grants occur after collective restore.

## Log collection

For every test, retain the date, runtime, load order, BuildVersion, client/server versions, logical choices, and `CharacterBuild`/`CharacterCreation`/`MagicService` lines. `_audit` reports, TSV files, and logs remain local and are not versioned.

## Phase 4E appearance acceptance after review

Use fresh A/B Nord, same sex, nonempty tints. Modify face/headparts/skin/weight,
close RaceMenu, then reverse direction. Capture diagnostic Weight nonfatal=true,
all fatal checks passing, one reset, transition, FaceGen begin/setup/update
Generated=true, face-tints-applied and applied. Compare count/hash before/after
Setup/Update and geometry against the transition. Verify no visual weight
regression across reset, stable IDs/provenance/position/inventory/equipment/death
and ownership. Live weight propagation is observed but its exact path is unknown.
This 4E regression procedure excludes race/sex changes and live publication. Pure tests do not
prove native rendering. If the head remains black after applied, retain the
FaceGen logs for a focused next phase; add no speculative material/color fix.
## Phase 5A diagnostic probe and activation gate (historical)

4E Nord/Nord same-sex final appearance is human-validated per the 5A handoff.
After review, the 5A build only observes a local Nord male -> High Elf male and
reverse switch: previous observation, completion callback, next service tick.
Compare runtime/base/overlay races, flags, 3D/face/head tokens and sex/weight.
The observer must currently refuse race change before mutation with an explicit
unproven-bool/order reason. Same-race 4E remains the regression test.
Remote activation requires caller/argument and engine-order/rebuild evidence first;
these observations alone are insufficient. Then, in a later authorized build,
verify target races, skeleton/3D readiness, one justified rebuild sequence,
FaceGen, inventory/equipment, position/death/ownership and stable network Actor
in both Nord/High Elf directions. No beast/sex test or deployment before review.

## Phase 5A.1 passive SwitchRace trace (historical procedure)

First verify a phase=enter line (install-attempt alone is insufficient). In a
clean local test, male Nord -> High Elf -> Nord through STRE RaceMenu; preserve
callSeq arguments, entry/return snapshots, changed fields, caller module/RVA and
local completion/next-tick lines. Then separately exercise a disposable NPC via
a vanilla mechanism and require an actual isPlayerRef=false hook entry. No hook
entry proves nothing about that mechanism's bool. Do not fabricate a remote call.
Use _audit/Export-SwitchRaceProbe.ps1 with each session's input log to produce
STRE-5A1-player-switchrace.txt and STRE-5A1-npc-switchrace.txt. No actual traces
were captured during implementation. Bool/order/readiness remain unvalidated;
no remote activation, second reset or inferred readiness from pointer changes.

## Phase 5A.2 remote acceptance (historical; failed head transition)

The revised handoff supplies the player and loaded-NPC 5A.1 observations. See STATUS
for current implementation/validation; these observations do not validate remote 5A.2.

After review, use a fresh session with the full required campaign roster, A and B
initially male Nord, nonempty tints and a quiet inventory/equipment state. Record
runtime 1.6.1170.0, build hashes/load order, actor/base/server IDs, private provenance,
inventory items/count and worn items, position, dead state and available ownership.
A opens RaceMenu, selects male High Elf, changes face/hair/tints, then closes it.
On B expect one SwitchRace false, target runtime/base immediately at return, target
post-Deserialize checks passing, no additional reset, new root and head/face readiness
within 120 service ticks, FaceGen Generated=true, face-tints-applied and applied.
Completion event is optional and must not by itself initiate tinting.

Compare body, ears/headparts, hair, skin/tints, weight, logical equipment and inventory,
position/death/ownership and identical actor/base/server identity. Retain both clients'
AppearanceSync, AppearanceApply, RaceAppearanceApply and SwitchRaceProbe lines; no
binary appearance dumps. Repeat male High Elf -> male Nord in a separate clean state.
Run the existing 4E same-race regression and confirm Solo emits no appearance packet.

Stop on race/identity/provenance mismatch, missing actor/base, observed logical loss,
timeout, failed FaceGen, disappearance or instability. Do not recover by respawn,
second SwitchRace, QueueUpdate or actor replacement. A failed native mutation is not
rolled back. Do not extend acceptance to sex/race+sex changes, Khajiit, Argonian,
custom races, live 2 Hz, camera or seated repair. Pure tests exercise guards and
state transitions, never simulate or certify native rendering.

## Phase 5A.3 acceptance after review

Fresh full-roster session: A/B male Nord, quiet inventory, nonempty tints. Record build
hashes, runtime 1.6.1170.0, load order and IDs/provenance/inventory/equipment/position/death.
A opens showracemenu, selects male High Elf, modifies face/hair/tints and closes. On B,
expect successful SwitchRace target races and Deserialize postchecks, exactly one
reset3d requested reason=race-head-rebuild-required, preResetRoot/Face/Head, separate
transition booleans, race-head-ready, FaceGen Generated=true, face-tints-applied/applied.
No completion event is required. Readiness/tints must finish within 120 post-reset ticks.

Visually verify High Elf body AND head/ears, correct hair/headparts, nonblack head skin,
body/head seam, stable weight/equipment/position and no disappearance/crash. Compare
Actor/Base/ServerId, CachedRefId/provenance/markers, target races/sex, logical inventory
and equipped items, death and available ownership before/after. Retain observer logs.
Keep the existing 4E same-race and Solo no-publication regressions. Do not test sex changes.

Stop on timeout, race regression, identity/provenance change, logical loss, FaceGen
failure or crash. No second reset, respawn, actor replacement or shader fix. Tests of
pure state transitions do not establish this native/visual result.

## Phase 5B.1 passive runtime procedure

After review, S1: clean out-of-combat male Nord Player, showracemenu, keep Nord and
change only sex to female, wait 2-3 seconds, close. S2 if stable: reopen, remain Nord,
change female to male, wait and close. Preserve complete logs, runtime/load order and
binary hashes. Require SexChangeProbe baseline with model paths, sex-flag-change,
changed state/geometry and any SwitchRaceProbe or do-reset3d-arguments/enter/return.
Do not assume either function is called; verify hook activation separately before
interpreting absence. Compare caller RVAs, actual bools, native sex, target/current
races, immediate return versus service ticks, event sexTickDelta, buffer/flags/tint
hash and descriptor behavior. Event alone never proves readiness. No remote sex test.

S3 optional: select and record a disposable loaded human NPC, invoke console sexchange,
retain matching actor traces and, if stable, return to original sex. Probe tracking is
triggered by actual hooked calls. If none occur, report unavailable NPC chronology;
if first call sees new sex already, report missing pre-mutation evidence. NPC GetTints
is unavailable, not empty. These probes do not prove remote deserialization or pick a
remote mutation order. Stop before remote implementation while flag/reset ordering,
sex-only SwitchRace behavior, skeleton models or head readiness remain ambiguous.

## Phase 5B.2 runtime acceptance after review

S1/S2 local Nord sex-only diagnostic evidence is supplied; do not repeat it as a gate.
Fresh full-roster two-client session, A/B male Nord, nonempty tints, quiet inventory.
Record runtime1.6.1170.0/load order/binary hashes and observer actor/base/server/cache IDs,
provenance, position/death/ownership and logical inventory/equipment. A showracemenu,
change only Nord male->female (face/hair/tints may then be adjusted), close.

B may produce either useful result. A: sex-postcheck passed, one reset3d requested
reason=sex-head-rebuild-required, sex-head-ready, FaceGen generated=true, applied.
Verify female body/head, skeleton/animation stability, hair/headparts, nonblack skin,
head/body seam, weight/equipment/position and no disappearance/crash, stable identities.
B: sex-postcheck mismatch, deserialize-did-not-apply-sex nativeReset=false; stop with no
setter, reset, retry or respawn. Preserve complete SexAppearanceApply/AppearanceApply
and passive probe logs. A later engine-adapter investigation is required for case B.

Stop on identity/race regression, reset failure, 120-tick readiness/FaceGen timeout,
logical equipment loss or instability. No combined race+sex, beasts/custom, camera,
seating or2Hz. Reverse female->male only after first direction works and if useful;
not an automatic prerequisite. Regress 4E,5A.3 and Solo no publication/application.


## Generic vanilla humanoid domain acceptance after review

Human-supplied evidence covers only 4E's original scope, 5A.3 Nord male -> HighElf
male and 5B.2 Nord male -> Nord female. Unit tests do not widen that runtime evidence.
Builds must pass before review/deployment; this mission performs no deployment.
Use a fresh full-roster session, correct binary hashes and exact runtime1.6.1170.0.
Record source/target races and sexes, loaded identities and models, observer
actor/base/server/cache/provenance IDs, position/death, inventory/equipment and logs.
Use supported vanilla records with nonempty tints and quiet inventory.

| Test | A edit then close RaceMenu | B expected route |
|---|---|---|
| R1 | HighElf male -> HighElf female | SameRaceSexChange, zero SwitchRace |
| R2 | Nord male -> HighElf female | RaceAndSexChange, one SwitchRace before Deserialize |
| R3 | Breton female -> Orc female | RaceChangeSameSex, same generic race cycle |
| Optional | Imperial male -> DarkElf female | Combined outside historical pairs |

All require a single Deserialize; noncombined requires one reset, combined now requires
the two phases described below. Require successful target/identity checks,
renewed root plus face/head, FaceGen Generated on stable geometry, applied. On B
check race/sex, body/head/ears/hair/headparts, tints/skin/no black head, animations,
equipment, position, identity and no disappearance/crash. Stop on any mismatch,
logical inventory/equipment loss, reset failure or readiness/FaceGen timeout; retain
logs and do not retry via respawn/replacement/setter. Weight remains diagnostic.
Normal movement between frames must be distinguished from an appearance-induced
position change; aggregate totals alone are not proof of per-item preservation.

For sequence acceptance use successive final-only RaceMenu sessions: Nord male ->
HighElf male -> HighElf female; Nord male -> Nord female -> HighElf female; Imperial
female -> DarkElf female -> DarkElf male -> Breton male. Record the actual attained
state before each next transition. Unit tests separately cover receiving Latest
while Active is immutable. Live RaceMenu sampling/publication is deliberately absent.
Do not run all permutations. Reverse tests are optional with symmetric implementation.
Regress 4E, original5A.3, original5B.2, Solo/offline and unsupported-domain refusal.
Argonian/Khajiit investigation follows separately with the identical applier; only
actual evidence may justify a different native strategy. Keep passive probes until
these new runtime paths have been validated.


## Combined two-phase native lifecycle � R2 first after review

The prior R2 failed: observer head stayed null through120 ticks despite correct final
native data. The new adapter is experimental. Do not treat the pure tests or prior local
RaceMenu chronology as remote acceptance. After review/deployment, use fresh A/B Nord
male, full roster, exact1.6.1170.0 and recorded hashes/load order. On A use the creation
RaceMenu, change Nord male -> HighElf female, close to emit the final-only snapshot.
Post-creation showracemenu does not publish and is not an acceptance test here.

Compare A's passive chronology with B's CombinedAppearanceApply and AppearanceApply:

1. B classification=RaceAndSexChange; combined-race-stage begin/before-switch.
2. One SwitchRace(false); target HighElf runtime/base, SOURCE male sex, stable private
   IDs/pointers/cache/provenance/markers. Compare headparts/hash, overlay, hair/bodyColor,
   male/female models, position/death and inventory/equipped before/after SwitchRace.
3. Reset ordinal1, no final Deserialize or FaceGen. Retain after-reset headparts and
   geometry. Wait for renewed root plus face/head, all nonnull, still HighElf MALE.
4. combined-race-stage ready must precede final Deserialize. If head never becomes
   valid, expect combined-race-stage-head-timeout, zero Deserialize/reset2/FaceGen.
   STOP, retain logs; no second workaround in this build.
5. One final Deserialize: HighElf FEMALE and stable identity. Compare headparts/hash,
   hair and bodyColor with A. A transient null head is acceptable before reset2.
6. Reset ordinal2, renewed root and face/head relative to preReset2, all nonnull,
   target race/sex. No third reset. Expect combined-final-stage ready.
7. One final FaceGen Setup with incoming tint count/hash; bounded Update polling until
   Generated on stable geometry, then face-tints-applied and Applied.

Do not reproduce the observed15/13 tick delays. Readiness, not sleeps, gates progression.
Log both source-sex and final-sex phases, each up to120 ticks with phase2 including tint
completion. Stop on identity/provenance drift, unexpected phase sex/race, reset failure,
logical loss or timeout. No fallback/setter/respawn/actor or NPC replacement.

Visually verify female HighElf body AND head, ears/hair/headparts, no black/stale Nordic
head, tint/skin match, stable animations/equipment/position and no crash. Retain complete
A/B logs and images. A's supplied example was265 bytes,29 tints,7 headparts and hash
FBB4EB09E1465634; use actual captured values for the new run, not fixed acceptance data.
After R2 human success, resume R1/R3 and only then consider a live publisher mission.


## R2 provenance-only acceptance � one clean run after review

Use ONE fresh R2, not repeated appearance experiments: A/B initially Nord male;
A CharacterCreation RaceMenu -> HighElf female -> close. Live remains off. Prepare
clean A/server/B logs from process start, record timezone, binary hashes and process
paths, identify B's player/server ID and A's remote actor on B before menu closure.
No deployment or game execution is part of the code mission.

Collect the full logs, not only Combined markers. Fill each answer with a matching
line/time/attemptSeq/actorId or explicit absence in the complete test interval:

- Publisher begin? captured/captured-native? rejected reason? sent/send-failed/pending?
- Server received? owner-accepted/rejected reason? broadcast recipient B?
- B notify-enter? entity-found? queued active/latest? ignored reason?
- Combined enter? nativeInProgress? all preguards and exact failedGuard?
- Actor/Base IDs and pointer tokens before closure and after visible mutation?
- First changedFields record for runtimeRace, baseRace, sex, headparts/hash, root,
  face and head, with tick, source and immediately bracketing native hook callSeq/RVA?

REMOTE_RECREATED requires actual nonnull identity differences: use OnCharacterSpawn-
created or CreateCharacterForEntity-created source and old/new tokens, and inspect
assignment/WaitingFor3D provenance. Missing identity is not itself recreation.
IN_PLACE_MUTATION requires stable identity; correlate changedFields with hot apply
entries and native before/after calls. If only update-before/after samples bracket
change, record the exact observed interval and cause=unresolved, not an invented writer.
Initial samples alone cannot prove a transition or identify an actor seen before tracing.

Decision: full publisher/server/receiver/combined chain supports H1; capture/send absent
localizes H2; concrete creation source plus changed identity supports H3/H4; stable
identity and no hot/creation chain directs H5 to native probes. Broadcast logging proves
invocation, not receipt, so still require B notify-enter. Do not conclude from missing
markers without complete matching-process logs. Stop after this one run and report the
chain and earliest observed mutation; propose no appearance fix before that evidence.


## R2 RaceStageReady service barrier acceptance - after review

This supersedes the preceding provenance-only run plan. The supplied R2 established
hot combined entry, stable Actor/Base and successful reset1 race/source-sex geometry
on A observing B/serverId3. Final geometry still failed in that supplied run.

Perform ONE fresh R2 only after code review and separately authorized deployment:
A/B initially Nord male, CharacterCreation RaceMenu -> HighElf female -> close.
Live publication stays disabled. Retain complete A/server/B logs, binary hashes,
process paths, timezone and the remote actor/server ID being observed. Observe the
other player's representation; use A observing B as in the supplied evidence.

Require this order for one immutable active snapshot:

1. SwitchRace(false), target race/source male, reset ordinal1, renewed nonnull
   root/face/head and `combined-race-stage ready phase-boundary=return-to-service`.
2. On a distinct later `serviceUpdate`, `combined-final-stage resume-from-race-ready`
   with the same identity and target race/SOURCE sex, all resume invariants passing,
   WaitingFor3D=false and race geometry still strictly ready.
3. Final Deserialize, target race/TARGET female checks, reset ordinal2 in that same
   resumed invocation; no extra barrier or third reset.
4. Final head nonnull with strict new geometry, FaceGen reached, Generated and Applied.
   Confirm HighElf female head/body/hair/ears/tints visually, no stale Viking head,
   stable Actor/Base, no equipment/position loss, no crash.

If head remains null after reset2 through the existing120-tick budget, STOP and retain
logs. Do not retry with extra resets, sleep, fallback, live publication or beast changes.
Passing TPTests proves pure service-update fencing, immutable Active across Latest,
post-barrier failure stopping before writes and one-shot bounds; it does not prove
Skyrim renders the final head. Existing full-suite 4E/5A.3/5B.2, solo, classification
and transport coverage remains mandatory; this mission performs no new game regression.


## Native sex rebuild RE - next diagnostic, not another corrective R2

The supplied barrier R2 already failed while proving distinct service updates4649/4650.
Do not repeat the preceding acceptance plan expecting the unchanged build to fix it.
Next, after review, prepare only targeted passive observation around52391 and existing
40255: original caller/return RVA, actual image range, thread, explicitly named update
counter domain, global401069/403521 pointers, target Actor/Base, process/biped identities,
INI value and RE-identified process flags. No mutating getter/predicate invocation.
Menu1A3/1A4 and gate-array states may be observed with bounded reads if needed.
No such new probe was installed by the RE-only mission.

Compare local sex rebuild and combined remote in one reviewed A/B run. Do not
request/repeat the validated5B.2 control C before analyzing that A/B evidence. Identify which menu caller/gate precedes the local reset and whether
native40255 executes inline or takes its queued branch. Correlate head readiness with
exact reset entry/return; a graph-byte write after a nonnull head return cannot be its
prior cause. Preserve complete matching-binary logs and update-domain distinctions.
Do not infer a14-tick requirement, rename unknown bits, modify overlayRace, invoke
player-global/menu helpers on remote, add reset3, or change5B.2. Stop with evidence if
no Actor-generic preparation is established; do not turn this diagnostic into a fix.


## Sex-rebuild differential A/B capture (instrumented, runtime pending)

After separate review and deployment, perform ONE initial combined run: both actors
initially Nord male; A and/or B selects HighElf female during creation. Retain complete
matching-binary client/server logs. No5B.2 retry before A/B analysis. Deployment is not
part of the instrumentation task.

Join SexRebuildDiff lines by callSeq/thread/actorPtr, then serverId when Known=true.
For the local wrapper, bracket RaceMenu52391-enter, DoReset3D-enter/return and
RaceMenu52391-after-reset/return on the same thread/player. Compare pre-wrapper and
reset-entry flags, without inferring causality from a changed0x800 bit. For remote,
identify reset ordinal2 from the existing combined traces, preserving ordinal1 and
the distinct serviceUpdate barrier as context. Use each line's Known fields; never
interpret unavailable zero as an observed null pointer, clear flag or inline branch.

Require a positive task-enqueue event for queued-op1C or inline-AIProcess-enter from
return7270AC for inline. Neither/both is unresolved. Installation logs are attempts;
actual entry traces are activation evidence. Preserve unfiltered logs if a prologue
check skips a hook. Read-only samples are sequential, not an atomic engine snapshot.

Populate the audit A/B table only from this run. A clear Actor-generic difference
permits proposing a single-variable next experiment, not implementing it. If A/B
provides no useful discriminator beyond menu/player state, then request control C.
Multiple differences remain ranked observations, not an established cause. Existing
5B.2 success and combined failure remain the runtime truth until human observation.


## ActorState RE follow-up (supersedes pending Nord5B.2 control above)

The new handoff supplies the successful Nord same-race remote sex control. Do not
repeat it merely to fill the earlier pending-control gate. No new passive hook is
needed to establish the SwitchRace41/1088 ->0/1008 transition: existing brackets and
exact-image helper writes account for it. Absence of an intermediate on-change sample
does not prove that no transient engine write occurred between samples.

No restoration experiment is approved or implemented by this RE. Proposed next
single experimental factor, only after separate review: use HighElf instead of Nord
for the successful same-race male->female control, with no SwitchRace in that lifecycle.
Establish source HighElf male from fresh creation, not a preceding experimental race
switch; keep the same setup, sex transition, observer, build and instrumentation.
Capture source/target race, overlay, reset entry/return and rendered head/body. If this
succeeds, target-race sex rebuild alone is insufficient to explain the combined failure,
and SwitchRace history/overlay/caches remain candidates. If it fails, investigate the
target-race/base/asset path before blaming cleared ActorState bits. Neither outcome
alone proves causality. No direct bit write, MarkChanged, reset3, delay or respawn.


## Controlled rematerialization candidate acceptance (not implemented)

The 2026-09-17 source audit replaces the prior immediate RE-control priority with a
lifecycle feasibility investigation. Its supplied fresh-spawn success is not a
transactional replacement pass. The complete evidence/failure matrix is in local
`_audit/character-appearance-controlled-rematerialization-audit.md`; STATUS remains
the validation authority. No new native acceptance run was performed by this audit.

Before enabling any replacement, prove staged/retiring discovery routing, private
TESNPC/Actor cleanup, remote Character Creation eligibility, state replay ordering
and the seated table contract. An unseated lifecycle test does not validate seating.
Future pure tests cover immutable Active/coalesced Latest, one candidate, atomic
binding, stale generations, FormID/entity reuse, every creation/restore failure,
late add/remove events, disconnect/phase exit/recovery and no synthetic assignment,
respawn, ownership or spawn/despawn packets. Preserve animation differential history
and pending movement/equipment/build data; inject updates on both sides of commit.

Then test Nord male -> HighElf male, followed only after success by Nord male ->
HighElf female. Regress Nord male -> Nord female and same-race cosmetic hot paths.
Repeat with A/B observer roles reversed, simultaneous changes, Latest while staging,
no subsequent animation/movement packet, and disconnect at every transaction stage.
Require same ECS/server/player/ownership identities, coherent new local Actor/Base
bindings, no duplicate/ghost actor, and complete old actor/base retirement.

Compare position, rotation, scale, cell/worldspace, enabled/death state, actor values,
per-item inventory extra data, worn/magic equipment and weapon draw. Verify the same
chair/marker, occupancy, pose and collision plus subsequent exit/entry. Check target
head/body/ears/hair/tints/skin seams and animations. Record matching build hashes and
both clients/server logs; repeated replacements must not leak native bases/handles.
Solo must never rematerialize a remote or emit related packets. TPTests/build success
only validates the source/test layer, never these native lifecycle/visual outcomes.


## Slice 1 shared private materializer structural regression

Run `python tools/Scripts/test_private_remote_materializer.py` alongside TPTests and
the client/server/launcher builds. Seven source-contract checks cover both callers,
private-player gating, the single CreateNPC -> FaceGenSetup -> CreateActor order,
borrowed result/temporary GamePtr release, no network identity/lifecycle logic in the
helper, separate existing-NPC/placed/non-player paths, unchanged empty-tint behavior,
and absence of new local/Solo/hot-update call sites. The extraction review also
compares the exact mission baseline to verify unrelated methods and caller setup.
These checks do not run native Skyrim or prove allocation/cleanup/rendering safety.
Future controlled rematerialization acceptance above is still pending and disabled.


## Slice 2A pure lifecycle and source fences

TPTests now exercises candidate-add without changing logical identity, late old-remove
after commit, candidate-remove after abort, authoritative removal at every stage,
stale session/entity/server/generation keys, duplicate discovery/retirement intentions,
early old/candidate loss, genuine committed-binding loss, creation after cancellation,
ambiguous form identities and unrelated actors. No case invokes Skyrim or a real ECS
transaction. A logical ready transition is supplied test evidence, not geometry proof.

Run `python Tools/Scripts/test_remote_materialization_lifecycle.py` for T7/T8: no
model-global discovery suppression and no production dependency using the dormant
model. Re-run Slice 1's structural script alongside the required TPTests and three
builds. Mission baseline hashes verify production source remains unchanged.

Next evidence gate: verify exact-runtime passive observation boundaries, then propose
a human-approved ordinary remote spawn/removal run. Distinguish discovery/root loss,
Delete request/return, form lookup absence, handle invalidation, Actor teardown and
private-base unregister/destruction. Observe identity before world exposure and event
ordering; never add a probe-held reference that extends the measured lifetime. Retain
bounded leak/handle observations across normal lifecycle repetitions. No controlled
replacement LAB until correlation and retirement evidence suffices. Unseated LAB
success, if later obtained, would still not establish the seated table contract.


## Slice 2B passive native lifetime acceptance (human runtime pending)

Offline checks: run `python Tools/Scripts/test_native_lifetime_probe.py`, both
prior slice source scripts, TPTests, and client/server/launcher builds. The six
window cases cover throttle/no catch-up, no-removal deadline, first-removal
deadline and duplicate events, zero/late removal, backward timestamps and a
finite maximum lookup budget. These checks do not prove native lifetimes.

After code review, a human may run the following ordinary lifecycle scenarios
with the reviewed local build. No agent game launch or deployment is implied.

1. R1: on observer A, before B connects, enable Debuggers ->
   `Passive native lifetime probe (next private remote)` in a non-master build.
   Alternatively, with Skyrim foreground, press and release F11 once; no F2/F3
   menu or mouse interaction is required. Both controls use the same transition.
   Verify `armed-next-private-player` and save each hook-install-attempt or
   hook-skipped line. Let B connect normally. The recorder selects one natural
   private player only. Capture the creation, registry, Spawn, handle, process
   scan, root-from-discovery and discovery-add markers. Compare lastServiceTick,
   thread, elapsedMs and probeSession; order equal-ms events by log order only
   where the same thread/source call establishes it.
2. R2: B disconnects normally well before the 180 s creation deadline. Preserve
   server-removal/Delete/discovery-remove, fresh lookup samples and any matched
   destructor entry/return. Keep A running through the 30 s post-removal window.
   Its start is the FIRST removal signal and duplicates cannot extend it. An
   earlier discovery loss can therefore shorten the later Delete observation.
3. R3: after saving logs, A reconnects/restarts while B exists. Arm before the
   next natural creation (after restart the default is off). Within the same
   process disable/re-enable to select a new pair, after the prior window ends.
   With the keyboard, press/release F11 for `phase=disabled`, then press/release
   F11 again for `phase=armed-next-private-player`. Holding F11 must not repeat.
   Do not synthesize a spawn, Delete, reset or server respawn to get evidence.
4. R4: perform at most three ordinary R2/R3 repetitions and retain all per-pair
   terminal samples and process/build identities. Tabulate Actor/Base FormIDs,
   last registry/handle states and matched destructors. One-pair snapshots are
   not a heap census: forms still registered at a prior window's end may retire
   later. A restarted process starts a new session-ID namespace.

Do not invoke DebugService's PlaceActorInWorld/debug spawner: its m_actors vector
holds GamePtr references. Keep the same debug panels/settings across runs and
record them; other existing collectors/engine references can affect timing.
The new probe does not put the observed remote in that vector. Its GetByHandle
lookup briefly acquires/releases a native reference within the existing helper;
that release may synchronously cause a destructor, so the returned pointer is
never dereferenced and the recorder mutex is not held over lookup calls.

For every run retain both clients/server logs, build hashes, runtime/version,
hook guard results, process identity and observer roles. No showracemenu, race
change, inventory change, seat manipulation, combat or candidate is part of this
slice. A hook-install-attempt is not destructor evidence. A skipped hook or a
missing marker is NOT OBSERVED WITHIN WINDOW, not leak/failure proof. Poll samples
are not atomic across engine calls and may miss transient registry/handle states.
The root marker is first seen by Discovery, not first native allocation.

Accept only separately recorded facts: registration, visibility, logical removal,
registry absence, handle invalidation, destructor entry, destructor return and
private-base observations. A matched deleting-destructor return with flags bit 0
set supports completion of that destructor/deallocator path, not global callback
quiescence. Do not infer TESNPC destruction from Actor destruction or Base absence.
The source pre-Spawn seam is available, but runtime ordering and any future
assignment fence remain unproved. Keep NO-GO replacement LAB until these runtime
facts support all six mission gates; no LAB is implemented by Slice 2B.

F11 ergonomic regression: before the addition, no F11 binding was found in STRE,
the vendored Tilted sources or the available tiltedcore package/cache sources.
The structural check verifies shared checkbox/hotkey state and setter, non-MASTER
guards, foreground gating, a held-key latch, no menu-visibility dependency and
no separate logging/network path. Human checks remain pending: one press arms,
holding does not toggle again, release/press disables, and checkbox state follows
keyboard changes. After expiry, two distinct presses disable then rearm.


## Slice 2B current-private mode (runtime acceptance pending)

Use this when A already sees B; retain next-private F11 for R1 creation history.
With only A+B, Skyrim foreground on A, first disable any enabled probe using F11
and verify `phase=disabled`. Hold Shift, press/release F11, then release Shift.
The menu equivalent is `Passive native lifetime probe (observe current private remote)`.
On the next game update require `phase=armed-current-private-remote`, then
`phase=current-private-remote-evidence` and `phase=current-private-remote-selected`
with the expected server/entity/FormIDs and comparison-only address tokens.
The evidence line must contain `selectionEvidence=durable-provenance` when the
allocation marker is present and matches, or
`selectionEvidence=validated-current-binding-fallback` when it is absent and the
current binding passes all guards. Correlate by probeSession; the first label is
not a guarantee that the marker survives recovery.
This starts the same bounded passive window at the current observation time.
Disconnect B normally within the window and collect logs exactly as for R2.
Archive before disabling/rearming. To observe current again: F11 disable, then
Shift+F11. To return to R1: F11 disable if enabled, then F11 to arm next-private.

Positive recovery acceptance: with only A+B, wait for LOCAL_RECOVERY_COMPLETE
and B's stable existing binding, then request current observation with the probe off.
If appearance tracing reports provenancePresent=0, require the fallback evidence
and the actual CURRENT pair (including after REMOTE_RECREATED), not cached old IDs.
No extra create/delete/replacement/appearance apply/network activity may result.
Retain F11 next-private for R1 and archive full client logs before another launch.

Negative acceptance: invoke current with no B, multiple remote players, an incomplete
binding (Local/assignment/WaitingFor3D/cache mismatch), a missing or inconsistent
native Actor/Base, or while either probe mode is enabled. Expect respectively
no-current-remote-player, multiple-remote-players, private-binding-not-ready,
native-binding-not-ready, or disable-existing-probe-first. Contradictory present
provenance rejects private-provenance-mismatch; known actor/server/cache aliases
reject ambiguous-current-binding and another bound actor sharing the base rejects
shared-current-base. Exercise injected impossible states offline only, never mutate
native state to fabricate them during R1-R4. Unsupported runtime and a binding disappearing
during capture also reject. No future connection may start a rejected request
without a fresh user action. Holding the shortcut must not repeat the attempt.
After expiry, the state stays enabled until explicitly disabled as before.

Offline structural checks cover one-shot game-thread selection, unique existing
remote/private/native binding gates, absence in MASTER, shared menu/hotkey request,
identity capture into the original Record, unchanged passive window, no synthetic
creation/discovery evidence and no dereference after temporary handle resolution.
TPTests also covers absent/matching/contradictory provenance, recovery-changed base,
non-temporary or equal Actor/Base IDs and mismatching cache IDs. Structural fences
keep this classifier confined to the non-MASTER passive selection and check evidence
labels, alias guards and unchanged production separation.
These checks and builds do not substitute for the pending human acceptance.


## Explicit creation markers (2026-09-18)

Automated: Solo/two/ten durable PlayerId ranks, exact marker mapping, invalid
identity/index, missing marker/wrong cell and nonfinite transforms, bounded
arrival/timeout, full marker orientation, no SlotId or posture dependency.
The accepted roster-2 checkpoint below covers the tested Marker01/02 flow.
Ten-player Marker01..10 placement/facing and physical clearance remain unvalidated.
Source tests do not execute native MoveTo. Papyrus bootstrap is unchanged.

## Individual Applied seating acceptance (2026-09-18)

Automated tests are domain/wire/source tests, not native engine execution.
Regression cases (roster-2 visual acceptance is recorded below; failure and
recovery cases are not implied by that acceptance):
- Solo finishes: controls unlock, quest stops, Seat01 is activated.
- A Applied while B creates: A seats immediately; B stays at MarkerXX.
- B then Applied: B seats independently; no Valen/collective transition.
- Duplicate Applied: no second activation. Wrong identity/revision: reject.
- Remote absent/WaitingFor3D/final replacement pending: retain intention;
  observe only the committed matching revision/current binding; project through STR replay, never remote Activate.
- Occupied by another actor or reserved: no ejection or teleport.
- Already seated on own SeatXX: success without activation.
- Rematerialized native token: project only onto the new valid binding.
- Recovery lock: no activation; disconnect: clear stale session intentions.
- Native function missing: pending-native-functions, no unsafe fallback.
- Confirm observer animation/interpolation, facing and furniture approach.
- Confirm Marker01/02/10 map to Seat01/02/10 and entry stays posture-independent.

Log prefix: [STRE][CreationSeating]; phase activation-issued is an issued request,
not proof of seating. The Solo failure on 0e421a99 invalidates the old phase
seated criterion (correct furniture plus GetSitState). Do not use it as visual
acceptance. pending-entry-timeout-no-reactivation never requests another entry.

Local marker approach (non-MASTER, existing Marker01..10 / Seat01..10):
- Preserve the existing ten reference IDs. Reposition/orient them manually in CK
  where necessary; no new SeatApproach records. The same references serve initial
  creation and seating, so verify both uses. See CK_IMPLEMENTATION/ADR-0025.
- Run `python -B Tools/Scripts/audit_seat_approach_markers.py`; require all ten
  existing pairs, persistent/enabled XMarkerHeading references, correct cell,
  finite upright transforms and exact C++ ID concordance. This is not a physical
  clearance or visual acceptance test. No fixed spacing threshold is a proof.
- Record actual executable/ESP/PSC/PEX hashes. First revalidate offline Solo
  rank 0 and a one-member network campaign, then B's own client in a fresh full
  roster of two. Determine B's durable sorted rank, not connection/join order.
  Keep A connected; B may finish first and must not wait for A's Applied.
- Require sessionConnected=true, localPlayer=true, actorRemote=false, Actor 14,
  durablePlayerId, creationPositionIndex=1, assignedSeatName=Seat02 and
  assignedApproachName=Marker02 for B. source=ck-marker must log the exact
  Marker02 position/rotation/cell saved in CK, never a computed chair offset.
  All ten ranks use the same pipeline; no rank is reassigned to Seat01.
- Native order: individual Applied/local finalization -> approach-target ->
  one MoveTo -> actual same-cell arrival within 2 units -> marker orientation ->
  later engine UpdateEvent -> one Activate. Require readiness update greater
  than facingUpdate and rotation within 0.05 radians. The preparation deadline
  is 5 seconds. Even if already at the marker, the update barriers still apply.
- Missing/disabled/wrong-cell/wrong-base marker, invalid transform, binding or
  target-transform drift, timeout, lost position or facing error must stop with
  an approach-* reason. No second move, fallback activation or unsafe repair.
- Occupied/reserved seat: wait; no ejection or remote MoveTo. Recovery/identity
  guards continue to apply. Repeated Applied/Tick must not rearm the operation.
- Keep all keyboard/mouse input released for at least 45 seconds after build
  validation (up to 5 s preparation plus 40 s capture), or Enter+5 s if later.
  Record video beforehand. Require visible native movement/sit animation and a
  truly seated body plus Furniture Enter. Engine/camera/logical state alone is
  insufficient. Old Seat01 geometry PASS does not validate the marker flow.
- Later validate remaining local ranks/markers and initial creation clearance;
  automated ten-rank support does not claim ten-seat runtime acceptance.
  Remote observer projection remains a separate validation, not this mission.

Solo and B-local marker runs are accepted by the maintainer; the steps below
remain a regression procedure, not an outstanding acceptance gate.
With the diagnostic build:
1. Start fresh connected creation: B must have durable rank 1 and Marker02.
   Keep A connected; B may finish first and must not wait for A's Applied.
   Record binary/asset hashes. Seat01 passes remain regression evidence.
2. Start video recording BEFORE final validation. Finish the build, release the
   validation key/button, then provide NO keyboard or mouse input for at least
   45 seconds including preparation: no movement, look, wheel, interaction,
   console or debug menu.
   If Enter occurs late, remain hands-off until five seconds after it, even when
   that extends beyond 40 seconds. Do not infer a hands-off run from action names.
   Save the full tp_client.log from before Applied through observation-window-ended
   (and any later first Enter + 5 seconds), alongside video and the human input
   attestation. Preserve the ten-second timeout line; capture continues beyond it.
3. Compare before-activation/after-activation, pending-entry-sample,
   furniture-enter-immediate and post-enter-sample. Check elapsedMs/sinceEnterMs,
   distance/orientation, transforms/cells, occupied, sitState, root and graph.
   The immediate sample is read synchronously inside the native event callback.
   Periodic samples must continue after entry-animation-observed as well.
   At Enter +30.750 s capture lasts through +40 s; at Enter +39 s it lasts
   through +44 s; a first Enter after +40 s reopens five seconds of capture.
   Check process-sample: current/run-once/effective package, targetIsSeat,
   procedureIndex/start time, raw path point and furniture idle, plus move/look
   input vectors and autoMove. Missing native process data must stay unknown.
   Raw path point/engine input are not navmesh-solver or physical-input proof.
4. Inspect furniture-event enter/exit and animation-action (event, target, idle,
   flags, transitionNoAnimation, skipped, result, remoteBlocked), especially the
   five seconds after Enter. The budget renews at 32 actions/second and resets
   at first Enter; inspect actionsDropped before interpreting missing actions.
   No event/action alone proves a pose, pathing failure or user intervention.
5. furniture-logical-only must not complete the local diagnostic projection.
   entry-animation-observed additionally requires a matching enter event, root
   and graph, and valid true isInFurniture/isIdleSitting reads. A -1 graph value
   is unknown. These observations do not prove the actual rendered pose.
6. During the hands-off interval, record the visible approach and actual pose.
   Only AFTER that interval, verify chair camera and exit/control recovery with
   input. Keep those interventions distinguishable from the passive capture.
   Save the log even if the pose still fails; do not retry activation in code.
7. Record manual-chair interaction separately as a baseline. Do not force
   ActorState, graph flags, teleport or CK changes to manufacture a pass.

The maintainer has runtime-accepted Seat02; the agent did not deploy or launch the game.
Save the full client `Data/SkyrimTogetherReborn/logs/tp_client.log` and rotated
`tp_client.1.log` through `tp_client.3.log` if present, plus video and human
input/pose attestation, after observation-window-ended and any later Enter + 5
seconds. The rotating logger is limited to 5 MiB per file; preserve the complete
run promptly before another launch or rotation can overwrite it.
Require exactly one approach-move-issued and one activation-issued per token;
an early rejection permits neither a retry nor fallback activation. The success
criterion is the visible approach, native sitting transition and truly seated
body with Furniture Enter, not the camera or engine/logical flags alone.
The accepted roster-2 result does not authorize Seat03..10 runtime claims or publication beyond the local checkpoint.

## Final rematerialization pre-reservation weapon wait (2026-09-19)

This supersedes immediate terminal rejection for WantToSheathe/Sheathing only;
WeaponSheathed and every actual replacement safety predicate remain unchanged.
The official flow applies in MASTER too; MarkerXX -> SeatXX remains non-MASTER.

Automated coverage: safe receipt; 4 -> 0 and 4 -> 5 -> 0 with exactly one
reservation/candidate; persistent 4/5 expiring at ten seconds without mutation;
duplicates preserving the first-final deadline; other ActorState and binding,
provenance, canonical-data/runtime failures; changed entity/version/native tokens;
disconnect/recovery cancellation; and unsafe state after reservation still aborts.
Tests use a deterministic steady-clock timeline, no sleeping or native emulation.
Structural checks verify fresh capture only at Ready and unchanged native guards.

Human roster-2 acceptance (not performed by the implementation agent):

1. Start fresh client processes and a new sealed roster of two, using the matching
   reviewed artifacts. Record exact artifacts, run time, durable PlayerIds/ranks,
   serverIds and which machine is A/B. Keep full logs from both clients and server.
2. Run BOTH completion orders in separate fresh client processes/campaigns:
   run 1: A finishes first while B is still creating, then B finishes;
   run 2: B finishes first while A is still creating, then A finishes.
   Observe A-sees-B AND B-sees-A in EACH run (four independent results).
   Local seating follows each player's own Applied without waiting for everyone.
   Do not introduce a barrier or force sheathing.
3. On each observer correlate the remote's Applied revision with
   final-snapshot-received. If already safe, expect admission-ready directly.
   Otherwise expect admission-wait-begin, weaponState=4 or 5, safeState=0,
   bindingGuards=passed and elapsedMs counted from the first final snapshot.
4. For 4 -> 0 or 4 -> 5 -> 0, expect any admission-weapon-changed lines, then
   admission-ready with weaponState=0 -> gate-accepted -> transaction-reserved
   -> candidate-create-enter -> candidate-ready -> candidate-commit -> complete.
   There must be only one transaction/candidate for this identity/revision and
   none during Pending. Remote seating must target only the final committed
   binding. Independently confirm both local players remain visibly seated.
5. If 4/5 persists, expect admission-expired detail=weapon-sheathing-timeout,
   no reservation/create/retirement and the old representation retained. Record
   this as FAIL visual / admission safely timed out, not a fixed black placeholder.
   Preserve both logs to investigate the upstream state producer/resolution;
   no forced ActorState, animation, artificial sheathing or hot apply is permitted.
   Another invalid guard must give its exact terminal reason, never be retried
   under the weapon wait. Duplicates must not rearm or extend the deadline.
6. Record A-observes-B and B-observes-A separately: final appearance visually
   correct (race/sex/face/tints), local seating intact, remote seating if observable.
   candidate-commit alone is not human visual proof or native memory-release proof.
7. Save both clients' complete tp_client.log and tp_client.1.log..tp_client.3.log
   after terminal admission/commit and the seating observation window (40 seconds,
   or Furniture Enter + 5 seconds if later), before restarting. Server logger uses
   logs/STServerOut.log relative to its launch working directory, with rotations
   STServerOut.1.log..STServerOut.3.log; preserve the same run from that directory.
   On the currently audited installation it is under
   Data/SkyrimTogetherReborn/logs, but confirm the server launch directory if moved.

No game launch/deployment or visual acceptance is implied by builds or TPTests.
The maintainer has validated both observers in both completion orders; see the
accepted animation replay checkpoint below and STATUS for evidence limits.
## Remote seating through STR actions - observer diagnosis (2026-09-19)

Historical diagnostic procedure; final reciprocal acceptance is recorded in the
animation replay checkpoint below and in STATUS. Earlier pending statements in
this section describe the diagnostic candidate before the accepted replay fix.

ADR-0026 replaces remote Activate with passive observation of the existing action
stream. The only functional change is neutralizing that invalid call. No replay
or buffering fix is implemented; final appearance is unchanged.

### Established timeline, 19 September 01:18, observer A seeing B

| Time | Evidence |
|---|---|
| 01:18:43.309 | Server Applied for B, transport player 2 / serverId 3 / revision 2 |
| 01:18:43.318 | A receives Applied |
| 01:18:43.319 | Seating intent awaits CommittedActor |
| 01:18:43.337 | Server validates owner and invokes final-snapshot send to player 1 |
| 01:18:43.352 | A receives final snapshot |
| 01:18:43.751 | Weapon 4 -> 0, admission Ready after 398 ms |
| 01:18:43.752 | Transaction reserved / candidate-create-enter |
| 01:18:43.881 | candidate-commit FF000846/9374E2F0, base FF000837; appearance visually correct per maintainer |
| 01:18:43.881 | Old code calls Activate on that final actor and Seat02 080BF3DC; returns false |
| 01:18:53.893 | Old activation attempt times out without reactivation |
| Unknown | B chair action arrival, execution on old/final actor, remote Furniture Enter |

Evidence files: `_audit/remote-seating-diagnostic/runtime-initial/tp_client.1.log`
and `STServerOut.log`. Unknown is not absent: the old probe only traced A's own
native actions. A's local chair entry generated IdlePlayer then IdleChairRightEnter
(action 00013009, idle 0003B070, target 0). This does not establish B's entry event.

### Static route and limits

1. HookPerformAction captures local action IDs, event/idle/target, flags, native
   state and variables. Its someFlag==1 / g_forceAnimation return does not publish
   an ActionEvent. The remote guard rejects independent native actions; the
   network replay route already uses ActorMediator::ForceAction separately.
2. OnActionEvent appends to LocalAnimationComponent or the existing early buffer.
   RunLocalUpdates serializes queued actions into ClientReferencesMoveRequest at
   the existing 100 ms cadence. Serialization clears the batch before Send.
3. Server OnReferencesMoveRequest requires the owner's entity view; its script
   HandleCharacterMove veto can skip an action. The movement broadcast excludes
   the owner and out-of-range recipients, copies the action batch, then clears it.
   These paths are unchanged. The supplied server log has no per-action evidence.
4. A receives ServerReferencesMoveRequest. Missing Remote/Interpolation/
   RemoteAnimation view skips that update; otherwise every action is enqueued.
5. RunRemoteUpdates resolves the current FormIdComponent. AnimationSystem waits
   for action tick (existing 300 ms interpolation delay) and graph readiness,
   calls the existing ForceAction once, then pops even if the result is false.
   Existing state and variable writes are unchanged.
6. Final local commit preserves the ECS queue and changes FormId/CachedRefId.
   It neither requeues LastRanAction nor requests a new server spawn replay.
   Existing server ActionReplayCache (32 actions) recognizes chair enter events
   and refines them to IdleChairEnterInstant for spawn replay. This local-only
   rematerialization does not request it again. Do not add replay without a real
   loss/application chronology.

### Next human run (no B log collection required)

Use the prepared non-MASTER diagnostic client. Record hash, observer machine,
start time and completion order. Use the same neutralized implementation on both
clients for eventual no-remote-Activate acceptance; no protocol/server update is
needed. No agent deployment or game launch is implied.

1. Fresh processes and new sealed roster 2. A finishes first, then B. Repeat
   separately with B first. Each local seating follows that player's own Applied.
2. On A correlate B's serverId/revision and Applied, final-snapshot-received,
   candidate-create-enter and candidate-commit. Never identify B by FormID alone.
3. Collect `[CreationSeating][RemoteProbe]` phases: applied-intent-created,
   awaiting-committed-actor, received-enqueue, received-filtered-missing-remote-animation-view,
   received-untracked-filtered, action-not-due, graph-not-ready, action-dequeued,
   force-action-return, queue-reset, committed-binding-selected, queue-at-binding,
   last-force-action-at-binding, binding-sample, native-action, furniture-event,
   observation-window-ended and session-cleared.
4. Match serverId + actionTick + action/idle/event/target/type. Compare actualActor
   and actualActorToken with LAB old/final identities. committedObserved is the
   last seating-update observation and can lag the real commit inside an update.
   Use candidate-commit timestamp and actual target token at this boundary.
5. action-dequeued precedes existing state/variable application; force-action-return
   follows the existing call and precedes pop. Result is not visible-pose proof.
   Samples show root/graph, graph bools (-1 means unknown), raw ActorState, furniture
   handle, packages/target and transforms/distance. Furniture events, if produced,
   get immediate read-only snapshots.
6. Storage is bounded to ten identities. Pre-Applied capture starts on first remote
   movement packet; Applied and first committed binding each start one 40-second
   window, no duplicate extension. First matching Enter can extend to Enter+5 s.
   Samples <=2 Hz; action lines <=32/s and 512/epoch; event/binding logs capped.
   Counters and droppedDiagnosticLines expose incomplete coverage. Unknown filtered
   entities have a separate 32-line session cap. No saved engine pointer or action
   payload retained for replay. Disconnect/fresh creation clears diagnostic state.
7. Observe at least 45 s after the last commit, and through a later Enter+5 s.
   Record A-sees-B and B-sees-A appearance/sitting, and both local seatings separately.
   Save A's full tp_client.log and tp_client.1.log..tp_client.3.log before restart,
   plus the matching server STServerOut.log and rotations. Current installed log
   directory: `C:\Program Files (x86)\Steam\steamapps\common\Skyrim Special Edition\Data\SkyrimTogetherReborn\logs`.

| Evidence on A | Permitted conclusion |
|---|---|
| Received and returned on old token before commit, no application on final token | Supports consumed-old-action hypothesis for that action, subject to complete coverage |
| Received before commit, returned on final token afterward | Queued action survived; no extra replay justified for it |
| Returned on final token, result false or no visible pose | Application/graph problem remains; loss is not demonstrated |
| received-filtered... | Exact observer view filter demonstrated |
| queue-reset between receipt and execution | Queue lifecycle reset observed; correlate reason/identity |
| No action, packets present, complete window | No matching decoded action observed at A; emission versus relay/transport remains unknown |
| Dropped lines or expired window | Missing lines cannot prove absence/loss |

Without B logs and without per-action server traces, static code identifies
emission/relay vetoes but cannot prove which occurred in a run with no action at A.
Report this limit rather than "never emitted" or "not relayed". If later server
evidence proves receipt without relay, classify that separate boundary. This
mission adds observer diagnostics only, no server instrumentation.

Acceptance: visible sitting and final appearance in both directions, local sitting
preserved, no remote Activate/MoveTo or ad hoc animation. No extra replay, ActorState
change, materializer/fence change, commit or push before evidence and acceptance.

## Animation replay continuity after final binding (2026-09-19)

**Acceptance recorded:** the maintainer confirms PASS for both roster-2 completion
orders: final appearances correct in both directions, both local players seated,
and both remote representations visibly seated after binding replacement. The
A-first log-backed checkpoint is 19 September 02:48:13; the other order is human
attestation without a separately identified timestamp. STATUS records the exact
trace, inspected candidate hash and validation limits. The following steps remain
the regression procedure, not pending acceptance of this slice. Ranks 2..9,
MASTER runtime, recovery/reconnect and native lifetime are not covered by that PASS.

This extends the preceding diagnostic-only procedure after old-binding consumption
was demonstrated. ADR-0027 owns the reconstruction contract; STATUS separates
earlier evidence, automated checks and the final human acceptance.

Use the prepared replay candidate on both clients, recording its manifest SHA256.
No automatic deployment, Skyrim/CK launch or server protocol update is performed.
Restart both client processes between runs because final-rematerialization
tombstones are process scoped. Run a fresh sealed roster 2 twice:

1. A completes creation first, B second; then B first, A second in a separate run.
   Each local seating must start after its own Applied, without waiting for the
   other player. Preserve the same existing markers/SeatXX assignments.
2. On observer A, correlate B's serverId, versioned entity, final revision,
   final-snapshot-received, admission Ready and candidate-commit. Final appearance
   must remain correct. Identify old/new Actor FormIDs and tokens from the logs.
3. After candidate-commit, require `[STRE][AnimationReplay] phase=binding-reconstructed`
   with matching session/generation/entity, oldActor/oldToken, newActor/newToken,
   sourceCount, pendingBefore, injected and pendingAfter. For a consumed chair
   entry, `cache-source` names the original event and `replay-injected` names the
   shared `IdleChairEnterInstant` counterpart with action=0/idle=0. No runtime
   FormID, seat number or source entry direction is hardcoded in the mechanism.
4. Require `replay-force-action-return` for that refined action on **newActor/newToken**,
   `matchesNewBinding=true`, after candidate-commit. Record result; do not infer
   visual pose from result=1. `replay-graph-pending` means the existing graph gate
   is delaying execution. It logs once per reconstruction, without a new graph
   mutation or arbitrary timeout. The synthetic prefix must execute before the
   previously pending move/turn actions, then live received actions continue.
5. A pending original entry alone must not be duplicated: injected=0 is correct
   when no consumed history needs reconstruction. An already received exit can
   report pending-exit-invalidated-history. A newer exit during graph wait reports
   replay-cancelled/reason=newer-pending-exit. Neither case may resurrect old seating.
   Disconnect/recovery must discard the synthetic prefix and history, with no
   stale replay after reconnect. Never remove guards to make a rejected run pass.
6. Observe at least 45 seconds after the last commit. Record independently:
   A sees B's final appearance; B sees A's final appearance; A seated locally;
   B seated locally; A sees B visibly seated on Seat02; B sees A visibly seated
   on Seat01. Adapt labels to the actual durable ranks rather than transport IDs.
   Any remote standing/floating/incorrect pose is FAIL even with return=1.
7. Save A's complete `tp_client.log` and available `.1.log`..`.3.log` rotations
   before restart, plus the matching `STServerOut.log` and candidate manifest.
   Files are in `Data/SkyrimTogetherReborn/logs` under the installed game. B logs
   are not required; reverse visual attestation must not be reported as a proven
   reverse action chronology. Keep each run and completion order separate.

The new replay logs have independent bounds (<=32 cached/refined/returned actions
per reconstruction and one graph-wait message). The older RemoteProbe can still
drop native/action lines; inspect its counters before treating absence as proof.
No remote Activate or MoveTo is allowed. Reciprocal visual acceptance in both completion orders is recorded above;
new changes require their own evidence and publication authorization. Native lifetime remains outside this test.