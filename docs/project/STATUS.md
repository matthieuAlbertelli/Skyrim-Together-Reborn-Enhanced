# Current STRE Status

> **Status:** source of truth for implemented and validated state.
> **Last updated:** October 21, 2026.

This document describes **the repository's actual current state**. Product
direction and release gates belong in [`ROADMAP.md`](../../ROADMAP.md),
operational progress belongs in the GitHub Project governed by
[`docs/production/GITHUB_GOVERNANCE.md`](../production/GITHUB_GOVERNANCE.md),
and technical detail belongs in each feature's documentation.

## Steam 1.7.104 first Solo gameplay acceptance (2026-10-21, Refs #99)

**Scoped Solo PASS, performed and attested by the maintainer.** New Game /
Alternate Start reaches the STRE inn; RaceMenu and creation work; local sitting,
walking to Valen/opening dialogue, and save/reload were reported successful.
The [lot-5 audit](../audit/2026-10-21-runtime-1.7.104-lot5-solo.md) separates
that report from captured technical evidence and records the remaining limits.
Compiled identity remains `runtime17-efdeab46623fbd25fafa5c8a7fa7e0cb4ebebda7`;
all 293 deployed files, three EXE/SKSE/AL inputs and 914 installed content files
still match the accepted lot-4 receipts. No code, binary, content or policy change.

| Functionality | Human attestation | Technical evidence | Accepted scope |
|---|---|---|---|
| New Game / Alternate Start | PASS, arrival in inn | Fresh stage-20 gate, offline Solo authorization, correct creation cell/marker | PASS for this Solo run |
| RaceMenu / creation | PASS | Open at 11:49:05.766, close at 11:49:12.218 | PASS for the exercised flow; race/sex matrix untested |
| Character Build applied | Creation reported | `class.thief`, BuildVersion 5, three selections, local application complete, level 1, 11 resolved item grants, five equipment entries, zero class spell grants | PASS for this local Thief build; exhaustive kits/attributes/spells and final visual fidelity untested |
| Local seating | PASS | One MoveTo, one Activate, entry confirmed, terminal observation closure; no subsequent seating trace in the captured segment | PASS for Solo Marker01/Seat01 |
| Controls after creation | Walking to/interacting with Valen reported | Controls unlocked | Limited PASS; intermittent control loss is not proven permanently fixed |
| Valen dialogue | Access/opening confirmed | Local Thief class projection stage 114; no dialogue-navigation event captured | Limited human PASS; branches, conditions, skip and closure not comprehensively tested |
| Save / load | PASS | Quick-save boundary, same SKSE co-save written/read, native loading completion, matching `.ess`/`.skse` pair present | PASS for this save/reload; exact UI route and detailed restored properties not established |
| Continue | Not attested | No specific Continue evidence | NOT TESTED |
| Multiplayer / collective recovery / World Sync | Not tested on 1.7 | Solo evidence cannot qualify these | NOT TESTED |

The new client-log segment is 11:48:43.897–12:01:26.821 Europe/Paris, with one
observed initialization sequence; no PID, exit code or second process is inferred.
Seat entry is confirmed at 11:49:18.908 and observation ends at 11:49:23.911
(`sinceEnterMs=5005`, `actionsDropped=0`). No repeat activation/observation loop
appears after closure in this capture. The ordinary reload completes at
11:52:13.576; this does not prove Continue or collective restoration.

No gameplay failure was reported or demonstrated by these logs. Retained
diagnostics include the successful native UI queue fallback for RaceMenu,
Discord error 4, animation-signature warnings and CEF CPU/GCM/configuration
messages. Optional `DialogueProgress` and intro/custom menu remain disabled by
their historical runtime guards. The old SkyUI Show All Saves compatibility
problem remains separate and is not cleared by this quick-save/reload.

The [next manual protocol](../features/alternate-start/TEST_PLAN.md#steam-17104-manual-qualification-sequence)
combines a targeted Continue/persistence check with preparation of two fresh
roster-2 completion orders. The installed server's native-plugin admission policy
still requires `BetterGrabbing.dll`, independently of `bEnableModCheck=false`.
With that DLL disabled, multiplayer admission is a prerequisite to resolve;
Better Grabbing 2.0 is not installed/qualified by this Solo PASS. No policy bypass
or second-PC deployment is authorized by this preparation. ADR-0018 is unchanged.

## Steam 1.7.104 local DEV main-menu acceptance (2026-10-21, Refs #99)

**PASS: official launcher to usable vanilla main menu, keyboard/mouse and
normal Quit (exit0).** The maintainer supplied visual/input acceptance because
Computer Use denied application access. STRE intro/custom presentation remains
**disabled** by its historical1.6 guard; this is not custom-menu acceptance.
The [resumed lot-4 evidence](../audit/2026-10-21-runtime-1.7.104-lot4-resume.md)
owns chronology, hashes, local workflow and limitations.

Steam1.7.104.0 is the sole future development target by maintainer decision;
the ordinary main-PC Windows/Steam/Vortex environment and normal profile writes
are authorized. This supersedes the earlier isolation prerequisite below.
The branch remains separate from main. CK/AutoMod requires1.7 requalification
before authoring; no guard/ABI or native source changed in this resumed lot.

Compiled source `efdeab46623fbd25fafa5c8a7fa7e0cb4ebebda7`, identity
`runtime17-efdeab46623fbd25fafa5c8a7fa7e0cb4ebebda7` in both client and server.
The actual DEV entry routes explicitly to the worktree, uses fresh output
directories and excludes CK import/remote deployment. It is a local ignored
script modification, not a versioned tool change. All293 deployed file hashes
match the new candidate at the actual game destination, including51 UI files.
Sixteen divergent installed content files were preserved/backed up; all914
content files remain unchanged. Two obsolete test executables were archived
and removed; no extra root executable/DLL remains in the runtime directory.

Exact EXE1.7.104/SKSE2.3.1/AL hashes still match lot3. The maintainer disabled
the old Better Grabbing, CCExtender and CKPE proxy through Vortex; zero SKSE
plugin DLLs and no root winhttp proxy remain. The live module list confirms
SKSE1.7.104, deployed EarlyLoad/libcef and Windows SYSTEM32 WINHTTP. SKSE reports
`init complete`; native UI registration and `MAIN_MENU_ENTERED campaign=none`
are logged; CEF requests installed UI resources. One launch, no startup fix or
retry. Non-blocking Discord/CEF/animation diagnostics are retained in the audit.

Full TPTests **459 cases/49,287 assertions PASS**, Python **234 tests PASS**,
and three local routing rejection checks PASS. Bounded preservation verified
12,613 records, with nine expected deployment/profile metadata/log changes;
historical artifacts, saves/state, primary checkout/main and its pre-existing
dirty STATUS remain preserved. No matching crash event/new dump or residual
game/CEF process was found after exit in the inspected scope.

During resumed lot 4, no solo, multiplayer, Valen, creation/appearance/seating,
native save/load or campaign recovery test ran. No server, Pc-vero, push, PR,
manual CI, merge or release. Historical1.6 results do not establish1.7 compatibility.
That execution stopped after the vanilla menu; subsequent Solo evidence is
recorded separately above and does not qualify custom presentation or multiplayer.

## Steam 1.7.104 isolated-startup preparation (2026-10-20, Refs #99)

Lot 4 has explicit authorization for an isolated main-PC main-menu smoke.
**Preflight BLOCKED before launch; runtime smoke NOT RUN**, not a runtime FAIL.
The [dated evidence](../audit/2026-10-20-runtime-1.7.104-lot4.md) establishes that
the existing launch path shares Windows Documents/AppData and the launcher's
HKCU selection: `--exePath` persists TitlePath/TitleExe, path routing only
changes CWD/DLL directories/PATH, and SKSE writes under CSIDL_MYDOCUMENTS.
General launch permission is not missing; a verified independent profile is.

The exact three lot-3 input hashes, six binary hashes and 51 UI hashes match.
Compiled source remains `b07bf205adc5bb3693d079ad2dc854e9c57ac59b`, identity
`runtime17-b07bf205adc5bb3693d079ad2dc854e9c57ac59b` in client and server.
All seven prior commits and their receipts are retained; no rebuild, gate,
gameplay, protocol or content change. The 18 static qualifications remain valid
for the same inspected bytes; no new engine compatibility is established.

Executed offline: 12 TPTests cases / 3,421 assertions PASS; 7 bootstrap source
checks PASS; 4 SKSE resource checks PASS using exact extracted production
function bodies and private fixtures (no DLL execution); 16 assembled main-menu
package checks PASS. An independent payload contains 1,205 files / 824,987,268
bytes, including the exact three client binaries, UI, CEF/Discord dependencies
and unchanged tracked Data. All copies have matching SHA256, one hardlink and
no symlink/junction ancestors. No full game, effective load order or Windows
test account was provisioned; this is an offline payload, not a deployment.

Installed BetterGrabbing resource0.1.12.0 and CCExtender fixed resource1.12.0.0
are statically predicted to be rejected by the exact SKSE2.3.1 V5 metadata
policy. The installed `winhttp.dll` imports CKPE.dll and is another source
contamination risk. These files are untouched and excluded from the payload.
No multiplayer Better Grabbing admission requirement was relaxed.

The automatic receipt has 14 offline PASS checks and 5 blocked environment/
isolation conditions, with `launch_allowed=false`. No Skyrim/STRE/SKSE-loader/
server process, native initialization, visible menu, input or normal-exit
acceptance exists for this lot. No user visual attestation was requested or
invented. The precise remaining preparation is a separate maintainer-provisioned
Windows test profile and coherent legitimate Steam environment on this PC,
followed by a fresh preflight. See the [runbook](../development/EXPERIMENTAL_RUNTIME_1_7.md)
and [menu-only procedure](../features/main-menu/TEST_PLAN.md#first-main-menu-only-runtime-gate).
Save/Continue, creation, appearance, seating and campaign tests remain outside
the authorization and unvalidated on 1.7.

Receipts: `_audit/rt17-lot4/DELIVERY.md`, `inputs.json`,
`candidate-verification.json`, `packaging-manifest.json`, `preflight.json`,
host/path/plugin inventories and protected before/after comparison. All 11,360
protected file records match; metadata for 21,087 installed files and the
226-file profile set also match. Registry selection, prior artifacts, primary
dirty STATUS and main remain preserved within that bounded scope. No shared profile
redirection, production/Vortex/Pc-vero deployment, Steam update, push, PR,
manual CI, merge or release occurred.

## Steam 1.7.104 static native qualification (2026-10-20, Refs #99)

Lot 3 continues `feat/skyrim-1.7-runtime` from
`90da92ccdb097d2520b86631ce42e0e56fc455d6`, retaining all five earlier local
commits and their artifacts. The maintainer deliberately updated Steam and
activated SKSE before this lot. Independently copied Skyrim 1.7.104.0,
SKSE 0.2.3.1 and matching AL104 were inspected without executing the game.
Exact paths/hashes, native instructions, ownership boundaries and divergences
are recorded in the [lot-3 audit](../audit/2026-10-20-runtime-1.7.104-lot3.md).

The [finite register](../development/RUNTIME_1_7_NATIVE_CONTRACTS.md) now has
**18/18 mandatory static qualifications**: ten ExactSource from lot 2 and eight
ExactBinary (B02/B03/C06/C07/C08/C09/C12/C13). Consequently the unchanged
`NativeAdmissionReady(NativeContracts)` expression evaluates true. This prepares
an experimental candidate for a separately authorized smoke; **no 1.7 engine
validation or global compatibility claim exists**. The version/store/AL/SKSE
preflight and missing-evidence fail-closed policy remain in place.

Implemented: most-recent selection now reads native remembered filename240/
length248 rather than list[0]; shared bounded manual index mapping and regression
tests; trace names reflect that BSString; MenuControls byte83 is correctly named
inputProcessingBlocked without changing toggle values; SkillsMenu's four-plus-two
NOP writes become one complete six-byte instruction. Production FinalAppearance
and LocalSeating guards admit the exact qualified 1.7.104 target. Life9, native
ownership/retirement boundaries and furniture events were inspected; real
destruction/private-base reclamation, visual appearance and controls are still
unvalidated. O02/O03/D01 retain their guarded fallback/disabled behavior.

The three dormant RTTI conclusions and 45-initializer lot 2 inventory stand.
No ADR-0018 authority, sealed roster, per-player native save, collective recovery,
Valen/content, Better Grabbing admission or network/schema rule changed.

Executed lot-3 Windows x64 Debug evidence, source
`b07bf205adc5bb3693d079ad2dc854e9c57ac59b`:

- Targeted TPTests: **12 cases / 3,421 assertions PASS**; full TPTests:
  **459 cases / 49,287 assertions PASS**.
- Full Python discovery: **234 tests PASS**. An initial sandbox run failed
  on temporary-fixture permissions; the complete host run passed.
- Actual client/launcher, EarlyLoad, TPProcess and server DLL/launcher linked;
  Angular production build succeeded. **51 UI files** are independently retained.
- Client EXE and server DLL both embed `runtime17-b07bf205adc5bb3693d079ad2dc854e9c57ac59b`.
  The new six-binary SHA-256 manifest is `_audit/rt17-lot3/candidate-manifest.json`;
  human-readable receipts are `BINARIES.md` and `DELIVERY.md` there.
- Static receipt: exact three input hashes/PEs, **319 literal references**,
  nine reviewed byte anchors and all **256 CRC entries** checked. This does
  not substitute for the per-contract signature/ownership analysis.
- **9,329 protected records unchanged**, including prior lot artifacts and
  the bounded installed-content/save/state inventory. Primary HEAD, main,
  primary pre-existing dirty STATUS and unrelated STATUS sections are preserved.
  Coverage is explicit local paths, not an exhaustive personal/remote scan.

Toolchain: VS2022 BuildTools / MSVC14.44.35207 / SDK10.0.26100.0 /
xmake3.0.9+HEAD.2b184e178; Node20.19.5 / pnpm8.15.9; Python3.14.
No source changed after this compiled identity; the later receipt commit is
documentation only. Warnings in successful builds are retained in the logs.

Historical evidence remains unchanged: lot 2 source
`faed1a01e5390ac6e5ddfd4735073cd1653d1123`, receipt 90da92cc, 456 C++ cases /
49,256 assertions and 233 Python tests; lot 1 source cf207f69/97779fbf,
receipt 824ddd37, 454 C++ cases /49,109 assertions and 232 Python tests.
Their binaries retain their own source identities and manifests.

No deployment, installed Steam/Vortex mutation, Skyrim/server/CK/xEdit launch,
manual CI, push, PR, merge or release occurred in this lot. No content/save/
database migration was performed. Follow the [experimental runbook](../development/EXPERIMENTAL_RUNTIME_1_7.md)
and feature plans only after explicit maintainer authorization; the
[compatibility matrix](../testing/COMPATIBILITY_MATRIX.md) retains stable support.

## Local Character Creation seating termination correction (2026-10-18, Refs #91)

The pre-correction maintainer-supplied roster-2 PC A/rank-0/Seat01 trace shows one activation,
matching Furniture Enter, correct occupied chair, sitState 3, available root,
ready graph and isInFurniture=1, while isIdleSitting remains 0. The previous
AnimationObserved predicate therefore stayed false, the projection remained
AwaitEntry and logged a ten-second pending timeout. Its max(Activate +40 s,
Enter +5 s) deadline kept early-Enter samples running for 40 seconds; closing
the diagnostic did not terminate an unconfirmed local intent. These are supplied
runtime observations and source findings, not a game run performed by the agent.
The supplied run did not reproduce persistent control blocking.

The local correction confirms the composite current actor/token/assigned-chair
evidence without requiring isIdleSitting, which remains logged. Success is
latched and logged once; subsequent updates cannot time it out or reactivate the
same token. The observation deadline is now 40 seconds without Enter, replaced
by first matching Enter +5 seconds when received before closure. One final log
distinguishes confirmed entry from a real unconfirmed timeout, marks the intent
terminal and clears callback tokens even if native readiness is lost. Duplicate
Applied retains that tombstone; a late Enter cannot reopen it. Constants remain
40,000/5,000/500 ms. The existing ten-second unconfirmed request-status timeout
remains visible without authorizing a retry.

Admission checks were extracted without changing their identity/roster rules so
behavioral tests exercise duplicate Applied against pending, confirmed and
terminal state. Native local-only MoveTo/Activate, rank mapping, final appearance,
MASTER/non-MASTER and Solo scope, session/campaign/recovery fences and ordinary
cell-transition boundaries remain in place. Remote action replay, server/wire
formats, control primitives, CK/ESP/PSC/PEX and checkpoint recovery are unchanged.
This realigns the existing local lifecycle contract; no new ADR is needed.

Executed locally on Windows x64/debug from base
`d7275f08889b79fa2ab10d6ee05067ed5c7f9271`:

- `xmake build TPTests`: exit 0.
- `TPTests.exe '[creation-seating],[character-creation],[seating],[animation-replay],[appearance-apply],[final-respawn],[helgen]' --reporter compact`:
  90 cases / 37,112 assertions passed.
- `TPTests.exe --reporter compact`: all 447 cases / 45,866 assertions passed.
  Executable for both runs: `build/windows/x64/debug/TPTests.exe`.
- `python -m unittest discover -s Tools/Scripts -p 'test_creation_seating.py' -v`:
  18 tests passed. Equivalent targeted commands for
  `test_remote_seating_projection.py` and `test_animation_binding_replay.py`:
  7 tests passed each.
- `python -m unittest discover -s Tools/Scripts -p 'test_*.py'`:
  all 227 tests passed.
- `xmake build -j 2 SkyrimTogetherClient`: exit 0; native static client library
  and the target's automatic Angular production build succeeded. The existing
  xmake warning that `-fPIC` is ignored remains; no compiler error or missing
  prerequisite blocked validation. This build did not run install/deploy.
- `git diff --check`: exit 0; final diff inspection found only local seating,
  its tests and the four affected canonical documents.

Initial automated outputs are retained in the ignored directory
`_audit/local-seating-lifecycle-20261018/`. Git submodule inspection required
normal host access after the sandbox denied `.git/modules/Libraries/TiltedConnect`;
the authorized host check found a clean starting tree. That implementation phase
did not execute gameplay or deploy. Subsequent publication, deployment and human
acceptance are recorded below; the earlier human checkpoints retain their scope.

### Human local runtime acceptance (2026-10-18)

The maintainer accepts source candidate
`c3560f387a43531b070331364e9a8d08e019c20f`, based directly on
`d7275f08889b79fa2ab10d6ee05067ed5c7f9271`, with tested client SHA-256
`D68205CA0BFC545C48A059D08FCC0CC75B2D1EB5CBE8D00D40E500E819C9AD08`.
Two fresh runs used a sealed full roster of two players. On PC A, both A-FIRST
(A completes Character Creation first) and A-SECOND (A completes second) pass
local sitting, normal controls, ordinary chair exit, walking and the ordinary
inn exit, with no crash or recovery lock.

**A-FIRST is log-backed and human-observed.** The supplied PC A log covers only
this run. Its reported October 18 chronology is:

- `19:27:03.534`: exactly one `phase=activation-issued`.
- `19:27:04.629`: `furniture-event event=enter`, `firstEnter=true`.
- `19:27:04.630`: exactly one `phase=local-entry-confirmed`.
- Subsequent samples show the assigned chair occupied, `sitState=3`,
  `logicalSeated=true`, `enterEvent=true`, `entryConfirmed=true`,
  `identityMatches=true`, `graphReady=true`, `isInFurniture=1` and
  `isIdleSitting=0`; the latter is now diagnostic only.
- `19:27:09.633`: `phase=observation-window-ended outcome=entry-confirmed`,
  `sinceEnterMs=5004`, closing at Enter +5 seconds without the old 40-second
  sampling tail.
- No `pending-entry-timeout-no-reactivation`, `entry-unconfirmed-timeout` or
  second activation occurs in the supplied log.

The supplied server evidence establishes A-FIRST: A Applied at `19:27:03.441`,
B Applied at `19:27:40.934`, followed by authorization of the collective 2/2
barrier, with no relevant server error. Local entry therefore precedes the
other player's Applied, as required by individual seating.

**A-SECOND is an explicit maintainer attestation of the same human outcomes.**
No PC A trace for that run is supplied; no timestamp, serverId or log-backed
chronology is attributed to it. No PC B log is required for this local increment.
These are human executions and supplied evidence, not Skyrim runs by the agent.

Before the human smoke, both automatic `pull_request` workflows passed on the
exact candidate SHA, attempt 1:
[Build windows 39145911254](https://github.com/matthieuAlbertelli/Skyrim-Together-Reborn-Enhanced/actions/runs/39145911254)
and
[Build linux 39145911222](https://github.com/matthieuAlbertelli/Skyrim-Together-Reborn-Enhanced/actions/runs/39145911222).
The automated local tests listed above also passed. The authorized normal
`build-and-deploy-dev.ps1` exited 0 on October 18, 19:00:11--19:02:39 Paris time;
distribution, local staging, Pc-vero and installed Data had the same client hash.
The 698 expected content files, protected scripts and manifest stayed unchanged;
685 CK import checks were identical and none copied content into the repository.
Publication/deployment receipts remain in ignored
`_audit/local-seating-publish-20261018/`. The generated BuildInfo retains its
historical version tag; source HEAD and the client hash above identify the tested
candidate, and this increment does not correct BuildInfo.

The acceptance-recording commit changes canonical documentation only; runtime
code/content remains strictly identical to the tested candidate. It introduces
no new build/deploy or Skyrim/CK/xEdit execution. This acceptance does not cover
general remote seating, other ranks/rosters or the race/sex matrix. Normal controls
in these two runs do not establish that the historical intermittent control-block
cause is permanently fixed; no checkpoint/reload test of that old block was
performed. Checkpoint/recovery behavior is unchanged. This is an increment
referencing #91, whose remaining remote-seating and historical control-block
scope stays open.

## Valen exact-byte promotion and local DEV verification (2026-10-18)

The maintainer explicitly authorizes versioning the runtime-accepted ESP,
SHA256 `61C4426840ED7697706E25B0C5E826B85B722F4174A82DC3A71DD3DC12C1883E`,
without regenerating it or requesting another human xEdit review. GameFiles,
installed Data, local Vortex staging and Pc-vero already contained these exact
bytes at preflight. No ESP recopy into GameFiles, authoring or rewrite was needed.
The original preparation session and its review state remain historical;
this authorization is not a fabricated xEdit attestation or ImportValidated run.
Local content commit `dea9f8f87eca7874f56c655c9f9b80babcb5d999` now versions
those exact bytes and the four navigation identities.

Six residual C FUZ assets were found at all four locations and in the packaging
manifest. Following explicit authorization, all 24 file entries were backed up
and removed, and only their six packaging lines were removed. Packaging is again
697 files: one ESP and 696 companions, each byte-identical to the preparation
inventory at all four locations. No FUZ/LIP/XWM remains under this plugin's voice
directory. The canonical record manifest adds only four navigation identities,
393 to 397; the unchanged ESP contains 553 records including TES4, four more
than the preceding D candidate.

Local content checks pass: strict records/reject-unexpected, MQ101 structure and
generated invariants, character catalogue, seat markers, corpus/generated review,
serialized topology, 22 class contexts, 100 survivor states and 812 runtime text
fields. The corpus/editorial fingerprint, native D source, SkyrimTogether.esp
and protected build/import scripts are unchanged. All 226 Python tests,
149 PowerShell checks and TPTests (440 cases / 45,505 assertions) pass.
`git diff --check` passes.

The normal `build-and-deploy-dev.ps1` completed successfully (exit 0) from
content HEAD `dea9f8f87eca7874f56c655c9f9b80babcb5d999`, at
`2026-10-17T22:48:10.2881885Z`--`2026-10-17T22:49:47.8184979Z` (October 18
in Europe/Paris). Its snapshot contains 695 files; all 685 automatic imports
are IDENTICAL, zero copied and zero Vortex backups imported. Independent
post-deploy verification confirms the accepted ESP hash in the Git index,
GameFiles, Data, local Vortex and Pc-vero, plus all 696 unchanged companions.
The client/server/DLL hashes match the distribution, Data and both stagings;
the deployed D binaries are unchanged. Repository clean after deployment.

- Deployed client SHA256: `992BA1F9B83A36D58600AC3AA31F92C1F46A635649847E4ED80E884A51671EF0`.
- Build/deploy log SHA256: `E14ABDE39AEC14129501F7186FBE95CFD2C2B916280767975EFB88AFB57C176C`.
- Evidence: `build-execution.json`, `build-internal-snapshot-verification.json`,
  `deployed-content.json` and `final-gates.json` in the private directory below.

The normal CK collector finds 695 owned files and 685 auto-importable files,
all identical between Data and GameFiles. Its packaging update is byte-idempotent.
Private cleanup backups, hashes, audit/test logs and preflight receipts are in
`_audit/valen-promotion-20261018/`. The human Solo, save/reload and roster-2
acceptances below retain only their reported scope. No push, CI, Ready or merge
is authorized by this local promotion; PR #97 remains DRAFT and #96 open.
Skyrim and CK were not launched. Stop for explicit authorization before push
and final CI.

## Valen corrected candidate: scoped human roster-2 and save/reload PASS (2026-10-18)

The maintainer reports **runtime roster-2 PASS** on the corrected candidate,
limited to the tested scenario and these five human observations:

- Valen dialogues function in multiplayer.
- Helgen progression is shared.
- Hadvar discovery is shared.
- Ralof discovery is shared.
- Valen dialogues evolve correspondingly.

The maintainer subsequently confirms: "Save/reload et scénarios testés PASS".
**Native save/reload and the scenarios actually tested are therefore accepted
as human PASS.** This follow-up supersedes the previously pending save/reload
status; it does not extend acceptance to untested cases.

The reports do not detail the initiating player, discovery order, both A-to-B
and B-to-A permutations, individual Say Once persistence checks or presentation
isolation. Those details are not separately established. ADR-0018 collective
recovery is not established by a save/reload PASS. No new binary fingerprint
was supplied with these reports; the earlier temporary-deployment identity
and evidence remain recorded below.

This update records the maintainer's scoped report in STATUS only. It does not
authorize further push or CI, or extend the prior review/import authorization.

## Valen temporary candidate: scoped human runtime PASS (2026-10-16)

The maintainer explicitly reports runtime PASS on the temporarily installed ESP
SHA256 `61C4426840ED7697706E25B0C5E826B85B722F4174A82DC3A71DD3DC12C1883E`,
from private session `10ba31de6cad4839ae0b8d5c85aaf89a`, with the existing D client.
The acceptance covers only the six observations supplied:

- Valen's root menu is corrected.
- Helgen is visible.
- Navigation works correctly.
- Hadvar discovery passes.
- Ralof discovery passes.
- Valen's dialogue evolves correctly with the discoveries.

No discovery order, individual stage-label check, undiscovered-survivor
non-disclosure case, exhaustive category traversal or multiplayer role is
separately attested by this October 16 report. Native save/reload persistence
and roster-2 were then pending. The October 18 reports above add scoped roster-2
and save/reload acceptance for the scenarios actually tested, without extending
that evidence to unreported permutations or ADR-0018 collective recovery.

This is acceptance of a temporary Data-only smoke, not a validated import or
merge approval. The maintainer explicitly deferred human xEdit review and
ImportValidated; GameFiles, manifests, Vortex staging and the D client were not
changed by the smoke deployment. The deployment receipt and the subsequent
verbatim human report/attestation are retained separately in
`_audit/valen-topology-smoke-20261016/20261016T175538Z-149dedb9cc5b42c1bb77a36892705265/`.
Earlier deployment/preparation receipts retain their original pre-test state.
No new build, deployment, commit, push, CI or formal import accompanies this
evidence update; PR #97 remains DRAFT and #96 open.

## Historical D navigation failure and pre-smoke correction (2026-10-16)

The maintainer reports two navigation failures on deployed D: after browsing,
closing and reopening Valen, the root can contain only RECRUIT; the Helgen root
entry never appears. These are human FAIL reports, not an inferred failure of
the accepted native skip mechanism. Hadvar validation was paused until the
[root-menu smoke](../features/alternate-start/VALEN_PLAYER_DIALOGUE_TOPOLOGY.md#required-human-menu-smoke)
passed; the later temporary-candidate report above supersedes that pause.
Earlier class and Solo-arrival attestations below retain their limited
scope; they do not establish correct menu navigation.

Read-only inspection confirms eleven zero-Response navigation INFOs, all eleven
category branches already Top-Level, no explicit root-return topic and a mixed
Helgen navigation/state-response starting topic. The
[topology contract and vanilla evidence](../features/alternate-start/VALEN_PLAYER_DIALOGUE_TOPOLOGY.md)
explain the generator correction and its limits. The adapter adds blank
Responses without Force Subtitle, explicit submenu/root links and a dedicated
three-stage Helgen root; it removes unintended TPIC previous-topic dependencies.
Eleven categories remain Top-Level; six internal report branches become Normal.
Four adapter records are new. Narrative conditions/effects, corpus/fingerprint,
native gate D and all companions stay fixed.

At this pre-smoke checkpoint implementation was local and not deployed.
The synthetic xEdit output passes
the topology reader, including 22 class contexts and 100 survivor combinations;
107 Python Valen tests (including 11 navigation tests), 19 PowerShell navigation
guards and 9 existing presentation guards pass. Official xEdit synthetic
DryRun/Prepare and the independent audit pass in session
`fe44f7c729fc45f2ac2f680a6e8eda31`, with original fixture/master hashes unchanged. Human
xEdit on the new production candidate and root-menu runtime smoke were
**NOT TESTED** at that checkpoint. Automated results alone do not authorize
import/build/deploy.
This iteration has no CI and no push; PR #97 is DRAFT and #96 open.

## Valen D: human skip, three classes and Solo Helgen arrival accepted (2026-10-16)

The maintainer's runtime-finalization mission and class follow-up attest the
following on deployed D, build `7e0834906aeb4e41275cdd5dcdad2a280c562f58`:

| Human scenario | Attested result and scope |
|---|---|
| Manual STRE skip | PASS: the ordinary vanilla control advances the STRE response after the configured delay. D is accepted; no measured timing or additional unreported subtest is inferred. |
| “Pourquoi m’avez-vous fait venir, moi ?” | PASS for Mage, Thief / Voleur and Warrior / Guerrier. Exact follow-up: “J'ai testé les 3 classes actuellement implémentées: mage, voleur et guerrier et ça fonctionne”. The other 18 classes and fallback have structural evidence only. |
| Solo Helgen lead and arrival | PASS for the tested flow: Valen dialogue, acceptance of “Les ruines de Helgen”, stage/objective 10, real travel and entry into Helgen, stage 20, objective 10 completed and objective 20 displayed. This validates arrival by the first player in Solo only. |

Objective 20's expected wording is “Enquêter sur les ruines de Helgen.” No
separate post-D typography check was explicitly attested, so global typography
acceptance remains unconfirmed. Repeated skips, silence/idle duration, vanilla
dialogue non-regression and menu behavior must not be promoted to separate human
PASS claims merely from the accepted skip mechanism. Retain the GFx gate, WAVs,
dialogue flags and skip behavior unless a new reproducible defect appears.

At this D checkpoint, remaining runtime evidence was **NOT TESTED / not yet
attested**: Solo individual
and both-survivor discoveries without disclosure of the unknown survivor;
native save/reload and local Say Once persistence; roster-2 stage-20/objective
and discovery propagation in both initiation directions; presentation remaining
local; representative SELF/GROUP/RECRUIT/DRAGONBORN/DRAGONS/HELGEN/HQ/SKYRIM/
MISSION/PERSONAL/EXIT navigation. The later temporary-candidate report above
updates only its six attested observations. The earlier #95 stage-10 tests prove neither
stage 20 nor discovery stages 11/12. A normal Solo reload will not establish
ADR-0018 collective recovery. The focused sequence and reporting rules are in
the [test plan](../features/alternate-start/TEST_PLAN.md#runtime-finalization-sequence-for-96).

This update records human reports only. Source text and the exact class follow-up
are retained in `_audit/valen-runtime-finalization-20261016/`; the original D
deployment receipts remain unchanged as historical pre-smoke evidence.

The maintainer reports **C runtime FAIL** on `STRE_INFO_VALEN_SELF_010_R /
0D6BC3`: all six native FUZ/LIP response assets still leave manual skip blocked.
Voice-asset experiments and Process Monitor are permanently abandoned for this
defect. This supersedes the pending C smoke and any earlier conditional plan to
generalize LIP/FUZ below. No additional runtime PASS is inferred.

Experiment D adds a native client permission gate for exact Steam runtime
**1.6.1170.0**. Only the open `Dialogue Menu` and a live `currentTopicInfo` whose
origin and winning source file are `STRE_AlternateStart.esp` qualify. The native
menu update reads `DialogueMenu_mc.bAllowProgress` through GFx. A false value
arms a `steady_clock` deadline; after 750 ms the same response may receive one
normal, non-sticky write to true if it is still false. True does nothing; a
subsequent false after vanilla skip starts a fresh deadline. Menu/movie/INFO,
segment token, speaker and session changes reset it. Missing/invalid native or
GFx state fails closed. No input hook, SkipText call, automatic progression or
SWF replacement is introduced. Scope, ABI sources and human criteria are in
[`VALEN_PRESENTATION_SMOKE.md`](../features/alternate-start/VALEN_PRESENTATION_SMOKE.md).

Native load entry, Main/Loading Menu opening and network connect/disconnect
invalidate the pending gate under the same lock as GFx writes. Every native menu
message still chains to the original handler. Temporary transition diagnostics
include INFO, plugin, false/arm and confirmed true, capped at 128 events per
process; no per-frame logging. Human manual-skip acceptance is scoped above;
the automated cases do not become individual human lifecycle attestations.

The content restoration is exact private A, SHA256
`31B8DD797A68BB51542F4C24986C82B21A2154E916B42D80B4574B1D7DB03471`.
This is the existing xEdit-authored candidate in session
`d184bc1581e7492980e551c20b810adb`, not a newly edited ESP. It differs from B/C
at one flag byte only: Force Subtitle + No LIP File (`0x0A00`) restored. All 392
runtime typography substitutions remain; canonical corpus/fingerprint and WAVs
are unchanged. The maintainer explicitly waives a new xEdit pass for this exact
byte restoration; this is **not** a fabricated new human xEdit attestation.
The six C FUZ were backed up and removed from repository, Data, local Vortex and
Pc-vero. Packaging returns exactly to the pre-C manifest (697 files / 696
companions); 549 ESP records and 393 canonical identities remain unchanged.

Local gate tests pass: seven cases / 99 assertions, including non-STRE, absent
menu/INFO/GFx, <750 ms, >=750 ms, true/false cycles, six successive skips,
failed-write suppression and identity/session resets. Full TPTests passes
440 cases / 45,505 assertions. The changed native client target compiles.
Evidence: `_audit/valen-dialogue-gate-d-20261016/`. All nine content audits pass,
including strict records, packaging, MQ101, catalogue, markers, serialized
typography/graph/classes and all 100 survivor combinations. Seven Python tests
and 22 PowerShell checks also pass. The actual DEV collector finds 695 owned
files and 685 automatic imports identical to Git; its manifest update is
byte-idempotent.

DEV build/deploy **PASS** from local implementation HEAD
`7e0834906aeb4e41275cdd5dcdad2a280c562f58`, exit 0,
`2026-10-16T15:28:59.2642917Z`–`2026-10-16T15:31:01.9328535Z`.
The internal snapshot contains 695 files; 685 automatic imports are IDENTICAL,
zero copied and zero Vortex backups. Independent verification confirms all 697
managed files across Git/index, installed Data, local Vortex and Pc-vero, with
the restored ESP also matching the private A and internal snapshot. The six C
FUZ are absent everywhere. Client/server/DLL hashes match the distribution and
all three installed destinations; SkyrimTogether.esp and protected tools remain
unchanged. Repository clean after deployment.

- Deployed client SHA256: `992BA1F9B83A36D58600AC3AA31F92C1F46A635649847E4ED80E884A51671EF0`.
- Build/deploy log SHA256: `FD546BBC65466FF20A844F0D7FB90EB2CACF134694F134DAE67F95B01A128346`.
- Receipts: `build-execution.json`, `build-internal-snapshot-verification.json`,
  `deployment-verification.json` and `final-gates.json` in the evidence directory.

The build was handed off before human testing; its later scoped acceptance is
recorded above. The temporary-candidate checkpoint now identifies the tested
content and remaining follow-up; this D deployment receipt remains historical.
No CI or push during runtime validation; no workflow changes. Once the requested
runtime scenarios are complete, prepare the final local hashes, exact results,
untested cases, local checks and unpushed commit list, then **STOP for explicit
authorization before final push / CI / Ready / merge**. PR #97 remains DRAFT at
remote HEAD `54bcc6a2954a51da201f0fee043969203a755da2`; #96 remains OPEN.

## Historical Valen A/B/C presentation experiments (2026-10-16)

The maintainer tested `c48be632f4a3dbd3f37311fce369cda67c346ab6` and reported
two failures: curly apostrophes render as squares, while French accents render
correctly; silent replies cannot be advanced manually before their timing WAV
ends. This supersedes the earlier pending runtime gate. It does not invalidate
the historical xEdit/import checks below or establish a cause for the skip issue.

The presentation adapter projects only the six approved punctuation substitutions
into DIAL FULL, INFO RNAM/NAM1 and narrative objective NNAM. The canonical corpus
and editorial fingerprint remain frozen. Its independent reader rejects every
other record/field change, checks actual serialized text, and reevaluates the
22 class contexts and 100 survivor combinations. Historical authoring plans,
receipts and binary fixtures are retained unchanged.

The bounded skip experiment targets `STRE_INFO_VALEN_SELF_010_R`, local FormID
`0D6BC3`: “Parlez-moi de vous.” → “Qui êtes-vous vraiment ?”. This repeatable
INFO has six response segments and no quest effect. A retains flags `0x0A00`;
B clears only `No LIP File` (`0x0800`), retaining `Force Subtitle` (`0x0200`).
Both private variants use identical punctuation projection and unchanged WAVs.
C is now authorized only on this INFO after the reported B failure; no
generalization to 668 assets is approved. Static asset checks are not engine-load
or runtime skip proof.

Full Windows/Linux CI is explicitly deferred until the merge HEAD for this
content iteration. Local tests/audits, exact new-candidate xEdit review, DEV
build/deploy and STOP before the human smoke remain distinct gates. PR #97 stays
DRAFT; #96 stays open.

**Reviewed B imported; human skip FAIL.** Preparation/import HEAD is
`54bcc6a2954a51da201f0fee043969203a755da2`, session
`3505022e7434419188eb6f4b1a02b2b1`. The maintainer's exact reply, “tout est PASS
sans error”, attests the requested Sort Masters, Check for Errors, save/close,
reopen and second Check for Errors. Warnings were not separately specified.
The saved ESP SHA256 is
`852E2CF392CA64F7694C7C8914BF53D54D3C93BFEBF88F2EF84F3BF240934933`;
the transcription log SHA256 is
`E750C56E942E1D229120B995EE26578795341EE074BD55830E5104E9A1F3D466`.
The receipt pins xEdit
`CCB7CA9E6D826868B186B0B9A0FC012AFFB3FDC9688250583353E6D84E6A2545`.
No raw xEdit output is inferred from this attestation.

Official `ImportValidated` succeeded with the ESP alone: zero companions copied
or changed. Saved candidate, validated snapshot, installed Data and repository
hashes agree. The plugin still contains 549 records including TES4, with 172
changed payloads, 392 text substitutions and exactly one INFO flag change.
The audit checks 796 serialized text fields (121 FULL, two RNAM, 671 NAM1,
two NNAM), the graph, 22 class contexts and all 100 survivor combinations.
All 696 companions, canonical corpus/fingerprint, both manifests and
SkyrimTogether.esp are unchanged. Packaging remains 697 files; the canonical
record manifest remains 393 identities. Nine post-import audits passed.

The maintainer subsequently confirmed the selected INFO's **A skip FAIL** on
the tested baseline: subtitles display apart from the reported typography defect,
advance automatically after their delay, and cannot be skipped manually.
No Process Monitor capture or exact timing/control was supplied. The maintainer
initially directed continuation of the A/B diagnosis with WAV resolution
**unproven**. That diagnostic prerequisite was subsequently withdrawn in favor
of C below. No engine-read evidence or additional runtime PASS is inferred.

Post-import local tests passed: 215 Python tests, 130 PowerShell checks
(66 general, 21 matrix preparation, 21 matrix import, 13 collector and nine
presentation guards), and TPTests 433 cases / 45,406 assertions. The actual
build collector finds 695 owned files and 685 auto-importable files, all
byte-identical between Data and the repository. `git diff --check` passed.
The diagnostic content commit is retained locally because synchronizing the PR
would automatically trigger Windows/Linux CI. The maintainer explicitly forbids
that CI for this iteration; it is reserved for the merge candidate after runtime
validation.

**DEV build and deployment SUCCESS** on local content HEAD
`ffaf089dbfec3e3eb233f0047fcbf846e4cd5634` (exit 0,
2026-10-16 13:16:00–13:17:53 UTC). The existing workstation workflow deployed to
local Vortex and its configured Pc-vero share. Independent verification confirms
all 697 managed files in Data/repository/both destinations and the ESP's indexed
blob match the reviewed bytes. Its internal snapshot contains 695 files; all 685
automatic imports were IDENTICAL, zero copied, zero Vortex backups. Both manifests
and SkyrimTogether.esp remain unchanged; Git was clean after deployment. The
client, server and STServer.dll match distribution = installed Data = local
Vortex = Pc-vero. Native binaries are unchanged from c48be63; client SHA256:
`C4FF4BAADD05D4CCF24D1371EBA3D9B4E3E35759CEBFD9D329F03765F8F540A3`.
Build/deploy log SHA256:
`13D7C4531468B192C0D7158D0D8269CF5647544B3D83933B1DECE6B15481E6B4`.
Receipts are retained in the B session (`build-execution.json`,
`build-internal-snapshot-verification.json`, `deployment-verification.json`,
`final-gates.json`). The following documentation commit records these observations;
it does not change deployed content. Neither local commit is pushed, so no new
Windows/Linux CI is triggered. The agent stopped before Skyrim.

The maintainer then tested deployed B on “Parlez-moi de vous.” → “Qui êtes-vous
vraiment ?” and explicitly reported **B runtime FAIL**: the response still cannot
be advanced manually. This supersedes the pending B skip result above; no new
typography, timing or transition PASS is inferred. Effective WAV loading remains
unproven. The maintainer explicitly stopped Process Monitor/instrumentation and
authorized a normal native voice prototype C on this one INFO, without a file-I/O
capture prerequisite. B must not be generalized.

**C prototype deployed; local audits PASS, human smoke pending.** Evidence is
retained under `_audit/valen-vanilla-c-20261016/`, with the portable hashes in
[`VALEN_VOICE_C_PROTOTYPE.json`](../features/alternate-start/VALEN_VOICE_C_PROTOTYPE.json).
The installed LipGenerator with FonixData generated six native LIPs using the
exact runtime NAM1 transcripts and French language. xWMAEncode encoded the six
unchanged silent WAVs; LIPFuzer packed each LIP/XWM pair into a FUZ with the
implicit voice basename for MaleWarlock and INFO `000d6bc3`, responses 1–6.
Only these six FUZ were imported; loose LIP/XWM remain private evidence.
Native unpacking restores the exact generated bytes. All six decoded PCM streams
are mono 44100 Hz/16-bit, every sample zero; codec padding is at most 26 ms.
The first packaging invocation returned exit 0 but no output without trailing
directory separators; the exact-count guard rejected it. The corrected invocation
produced the six audited assets. No failed output is included.

The ESP remains byte-identical to reviewed B (`852E2CF3…40934933`), so its xEdit
receipt remains applicable: 549 records, no new record or flag changes. This INFO
has no VMAD/fragment timer to remove; duration is carried by its voice assets.
All 696 existing companions, the frozen corpus/fingerprint, canonical record
manifest and SkyrimTogether.esp remain unchanged. No Papyrus compilation or
other INFO asset generation is required. C runtime skip/silence/timing/menu
criteria remain **NOT TESTED**; no Skyrim launch or CI is authorized in this pass.

The explicit CK importer copied exactly six FUZ from the audited private snapshot
to the repository; Data contains the same six new files. Packaging adds only
these six paths: 703 managed files, comprising the ESP and 702 companions.
The canonical record manifest remains 393 identities. Ten targeted audits pass,
including typography/graph/22 class contexts/100 survivor combinations and the
native voice prototype; seven Python presentation tests and 22 PowerShell checks
(13 collector, nine presentation guards) pass. `git diff --check` passes.
The actual DEV collector finds 701 owned files and 691 automatic imports, all
byte-identical between Data and the repository, with no Vortex backup selected.
The historical misplaced PEX in Data/Scripts/Source is ignored by the collector
and is not imported. Protected tools and the original 697 managed files retain
their before-import hashes. Local commits are retained without push to avoid CI.

The first C build/deploy on `658a1b90bbc45845e6fb660365624a3a3b3b1a32`
returned exit 0 but is **not accepted as a clean deployment**: the local collector
appended six duplicate manifest entries because the new paths used `/` while its
paths use `\`. The 691 imports were identical and all deployed assets match, but
the repository was dirty. Its log/receipt are preserved. Only the six new entries
are normalized to the collector's existing spelling; the original 697 entries
and all assets are unchanged. The second preflight additionally runs the actual
manifest helper on a private copy and requires byte-identical output before the
workflow may be repeated. No build-script or importer change is needed.

**Accepted C DEV build/deploy SUCCESS** on local content HEAD
`26c6f334e93b07b4d51b53c75f4a386ff8ae84b3` (exit 0,
2026-10-16 14:56:57–14:58:42 UTC). Independent post-deploy verification passes:
701 internal snapshot files; 691 IDENTICAL automatic imports, zero copied,
zero Vortex backups; 703 managed files match installed Data, repository, local
Vortex and Pc-vero. The six FUZ additionally match private source, asset snapshot
and indexed Git blobs. The ESP still matches B's reviewed/validated snapshot and
indexed blob. All original companions, protected tools and SkyrimTogether.esp
remain unchanged. The manifest is unchanged by this build and Git is clean.
Client/server/STServer.dll match distribution, Data and both deployment targets;
the client SHA256 remains
`C4FF4BAADD05D4CCF24D1371EBA3D9B4E3E35759CEBFD9D329F03765F8F540A3`.
Accepted build log SHA256:
`390AD53BA6E66C954A1BD99E36BB1A483B96E0ECC4327A415258760FA4CC4734`.
The accepted execution/snapshot/final-gate receipts are in `build-attempt-2/`
under the C evidence root; `deployment-verification.json` records all deployed
hash checks. The following documentation commit changes no deployed content.
No push, CI or Skyrim launch was performed. **STOP for the maintainer's C smoke**:
visible subtitles, silence, sufficient idle duration, immediate vanilla skip,
clean progression/menu return and no double advance remain unvalidated in game.

## Historical human-reviewed Valen matrix import (2026-10-16)

The complete reviewed matrix is imported into `GameFiles/Skyrim` on
`feat/valen-dialogue-matrix-v1`, PR #97 DRAFT. Session
`63c81f61da724c01a6b561181457d3f1` is `Imported`. The ESP SHA256 remains exactly
`2ECEA346231CF6F6453205948FEF4E6C5CE5882C19C1F9141D9873A41DEE355E`.
No reviewed candidate, graph, script, PEX or WAV was regenerated or edited.

The maintainer attested Sort Masters, Check for Errors, save/close, reopen and a
second Check for Errors with no errors. Warnings were not separately specified.
`xedit-validation.log` is a transcript of that statement, not raw xEdit output;
its SHA256 is `B99FA865F3D939D912635F91A08D698F778D704E103C9AC95B8EBEEC7F8D2E31`.
The receipt identifies xEdit SHA256
`CCB7CA9E6D826868B186B0B9A0FC012AFFB3FDC9688250583353E6D84E6A2545`.

Preparation provenance remains `6a1694861af408c1a7c9d0eb690b69859714dbdf`.
The first import on `24c981a958b789e0542886e6f319cc2cbe4e45c8` followed successful
Windows/Linux CI and copied all 673 approved files, then failed when the existing
importer's final native `git status` exceeded Windows' command-line limit.
That `ImportFailed` result and the backups remain preserved. The original
protected importer is unchanged; the tracked adapter now bounds each call to
64 paths / 16000 characters, including the local build wrapper's existing call.

Explicit no-copy completion succeeded on
`9e18afcab5f8fc519c454a921b684ac72a0dc8d0`, after
[Windows](https://github.com/matthieuAlbertelli/Skyrim-Together-Reborn-Enhanced/actions/runs/38888661712)
and [Linux](https://github.com/matthieuAlbertelli/Skyrim-Together-Reborn-Enhanced/actions/runs/38888658121)
CI SUCCESS on that exact HEAD. It revalidated all frozen evidence and copied-file
hashes with current binary/asset auditors, required no staged/unrelated changes,
and promoted the manifests without recopying content. PreparationHead, ImportHead
and CompletionHead are distinct; `preparation-session.json` preserves the original
session bytes. ImportValidated cannot replay a failed or imported session.

The 300-record plan produced **293 additions, five changed existing payloads and
two unchanged planned updates**. The plugin contains **549 records including TES4**;
the canonical records manifest contains **393 identities**. It preserves the
reviewed proposal's exact records and constraints; only its last explanatory note
was updated to reflect completed import. The private proposal remains unchanged.
The full graph, **22 class/fallback projections** and all **100 survivor cases**
reproduce the historical audit exactly: **32 automatic / 17 explicit / 51
incoherent / zero uncovered**.

The imported delta is **672 companions plus the ESP**: two existing QF PSC/PEX
replacements, observer PSC/PEX and **668 new timing WAVs**. The validated snapshot
contains exactly those **673 files**. Packaging has **697 managed files** (ESP +
696 companions). Snapshot, installed Data, repository and staged blob hashes match
for the entire promoted delta; historical evidence and out-of-scope protected
files remain unchanged. SkyrimTogether.esp retains SHA256
`3196266AA29855574BDA48335DA9AB2F18AD33A80EEE116400DA7C5C81C93D7E`.
No master, private audit/cache or `.vortex_backup` was imported.

Post-import validation: packaging, strict records with reject-unexpected, MQ101
structure/generated invariants, catalogue, markers, Valen binary graph/classes,
generated corpus and survivor coverage all PASS. **208 Python tests**, **66 general
PowerShell safeguards**, **21 preparation cases**, **21 import/completion cases**,
**13 local collector cases**, and **TPTests: 433 cases / 45,406 assertions** PASS.
`git diff --check` PASS. The 673-file regression exercises real native Git status;
completion fixtures prove no recopy, preserved failure evidence and rejection of
missing/red CI, staged/unrelated changes, drift and replay.

Deployment requires clean content-HEAD Windows/Linux SUCCESS before the existing
`build-and-deploy-dev.ps1`. The maintainer separately authorized only the installed
`Source/Scripts/TIF_030D6B1A.psc` LF-to-CRLF alignment to its unchanged reviewed/Git
bytes, after import and audits, with backup, normalized comparison and complete
Data hash comparison. It is not a functional #96 change and requires no compilation
or xEdit. Its execution receipt and the build/deployment receipts belong to this
session; subsequent deployment results are reported in PR #97. Runtime validation
remains pending, issue #96 stays open, and Skyrim must not be launched by this flow.

## Historical matrix preparation evidence (2026-10-16)

This section records the earlier private preparation checkpoints. Current import
and validation state is described above; historical claims below retain their
original checkpoint scope.

[#96](https://github.com/matthieuAlbertelli/Skyrim-Together-Reborn-Enhanced/issues/96)
tracks the complete matrix under #20/#19 on `feat/valen-dialogue-matrix-v1`, based
on main `bdedb163aa516dc28917b292caf9b869afda983c`. The canonical JSON, formal
schema, linter, generated review and exhaustive survivor condition table are
prepared. The source comparison accounts for all 140 original approved response
blocks. The maintainer additionally approved SINGLE_KNOWN_FREED and
SINGLE_KNOWN_DEPARTED on October 16; the two existing #95 response blocks preserve
the actual ESP strings. The editorial graph has 133 semantic nodes, 144 response
blocks including the common recruitment introduction, and 21 classes plus
fallback. These counts
are not CK record or allocated FormKey counts.

The maintainer corrected discovery to two independent shared facts, distinct
from the existing Hadvar/Ralof physical states. The
[implementation proposal](../features/alternate-start/VALEN_DIALOGUE_MATRIX_V1.md)
uses completed stages 11/12 on the existing shared narrative quest, after arrival
stage 20, with ordinary QuestService propagation and per-player native saves
under ADR-0018. Its alias script and QF template are implemented in the bounded
private preparation toolchain and compile successfully. They have not been
imported, deployed or runtime-validated.

The exhaustive 100-row table finds **32 generic-reply cases**, 17 separate explicit
report/navigation cases, 51 inconsistent/transient cases rejected without speech,
and **0 uncovered cases**. The two approved additions cover all twelve former
gaps symmetrically: one known survivor Freed or Departed while the other is
undiscovered in state 0, 1 or 2. They remain repeatable generic-topic variants,
with no shared effect, new first report or `Reported` fact. The corpus authoring
gate now passes; negative tests still reject missing rules or overlaps.
Technical implementation work is not an editorial approval gate. This is an
intermediate preparation, not delivery of the whole feature.

Local Preflight, read-only Inspect and PapyrusStatus succeeded. Baselines cover
all 27 managed files in both Git/Data, both protected plugins, the canonical
record manifest and tool executables. Production Git/Data content remains
unchanged. Official xEdit and direct PapyrusCompiler were executed on invented
private fixtures only; no import, build/deploy or game process was launched.
No human xEdit, Solo, roster-2, native save or recovery PASS is claimed for #96.

The bounded `AddValenDialogueMatrixV1` plan concerns 300 records: two QUST,
17 DLBR, 121 DIAL and 160 INFO. Official synthetic output has exactly **293 new
records**, **five changed existing payloads**, and **two planned updates whose
payloads remain identical**. All seven #95 identities are preserved. The
independent reader verifies the full graph, field order/multiplicity, VMAD,
French strings, all 100 actual serialized survivor-condition cases and all
22 class/fallback projections. The official positive fixture and the rejected
sorted-array regression are versioned under `Tools/Scripts/valen/matrix/fixtures`.

The class bridge runs only after validated Character Creation completion and
projects local completed stages on ValenDialogue; it adds no protocol or native
Papyrus API. The observer detects actual proximity/LOS in wounded or captured
locations, fences campaign recovery, and uses independent completed stages 11/12
after shared arrival stage 20. Freed and Departed remain distinct defensive
future cases. Existing QuestService and technical Helgen implementation are
unchanged. Runtime discovery/save/recovery remains pending.

The generated synthetic QF and observer compile directly with PapyrusCompiler.exe
with **zero errors and zero warnings**. **668 new silent PCM WAVs** total
309,373,392 bytes; their names, timing, sample format, silence and hashes pass
read-back verification. Existing synthetic TIF, SEQ and three WAVs are preserved.
This is asset-pipeline evidence, not audible/runtime acceptance.

Real preparation requires a clean committed feature HEAD, current origin/main
ancestry and successful Windows/Linux CI on that exact HEAD. It runs private
DryRun/Prepare, independent reread, compilation and asset verification, then
stops at **HUMAN_XEDIT_REQUIRED**. Matrix ImportValidated is explicitly refused
at this preparation boundary. The canonical CK manifests remain unchanged.

Implementation verification: **195 Python tests**, **66 PowerShell toolchain
cases**, **21 matrix preparation guards**, **13 CK collector cases**, and the
six read-only CK audits pass. The rebuilt TPTests target passes **417 cases /
45,082 assertions**, including the class projection checks. `git diff --check`
passes. This source checkpoint records preparation evidence; it grants neither
human xEdit approval nor permission to import/deploy a private candidate.

The first real preparation attempt stopped before xEdit because the two existing
Valen QF/TIF PSC files use CRLF in the Git checkout and LF in Data, already present
in the initial baseline. The source text and all 60 baseline hashes were unchanged.
The guard now permits only that byte-preserving PSC comparison, records the two
raw hashes separately, and still rejects every binary, BOM, text or final-newline
change. It copies repository bytes privately without normalizing any original.
Regression fixtures cover those rejections; exact-HEAD CI must pass again before
a fresh real preparation session can start.

The next private session completed official DryRun/Prepare and the independent
ESP audit, then stopped because the safe copier refuses an existing destination.
The wrapper now archives the unchanged private writer input as `writer-input.esp`
before copying the audited output to the private candidate path. Integration
fixtures verify input/output preservation and reject changed input or an existing
archive. Failed sessions remain terminal; originals and production content are
unchanged, and a new exact-HEAD CI gate precedes the next session.

Initial preparation at 414556a9 passed 160 Python tests, 66 PowerShell toolchain
cases, 13 collector fixture cases and all six existing read-only CK audits. Its
source comparison verified all 140 original replies against the mission file.
The approved additions are separately fingerprinted; a regression test proves
the original editorial fingerprint is unchanged even if that external attachment
is no longer available. Discovery order/idempotency evidence remains a
completed-stage reference model plus source-path inspection; compiled Papyrus
is not native save/recovery proof.

After integrating the two approved replies, all 164 Python tests pass (25 for
this matrix), generated views agree with the corpus, the authoring input gate
passes with 32/17/51/0 coverage, and `git diff --check` passes. All 60 protected
Git/Data/tool baseline hashes remain unchanged. The external mission attachment
is now absent; its original comparison remains historical evidence, with the
original editorial fingerprint independently preserved by the regression test.

## Valen's shared Helgen lead: Solo and both roster-2 directions accepted (2026-10-15)

For [#94](https://github.com/matthieuAlbertelli/Skyrim-Together-Reborn-Enhanced/issues/94),
the dedicated `AddValenHelgen` adapter and official xEdit script author the
seven-record content described in CK_IMPLEMENTATION, now imported into the
versioned ESP with its eight companion files. The player's acceptance
starts the separate narrative `STRE_QUEST_ValenHelgen`, stage/objective 10,
through bound TIF/QF fragments. The narrative quest now follows standard
QuestService Start/StageUpdate/Stop synchronization: acceptance by either party
member must give the other members stage/objective 10 without separate acceptance.
Only `STRE_QUEST_ValenDialogue` stays excluded alongside AlternateStart and
HelgenInvestigation. Side Quest type 8 does not require the experimental
Misc/None setting. Existing technical Helgen
projection, its post-character-creation trigger, T+4 and ADR-0018 stay unchanged.

**Executed source/fixture evidence:** 134 Python tests (including 15 Valen
candidate-audit cases), 66 PowerShell safeguard cases and all six baseline CK
audits pass. Official xEdit 4.1.5f successfully executes DryRun then Prepare on
fresh synthetic masters/plugin; independent binary auditing confirms seven
records, ownership/parentage, CTDA, French strings, VMAD and unchanged original
record payloads. Its synthetic input and masters retain their hashes. Both
generated fixture fragments compile with the installed Papyrus compiler and
local CK/SKSE imports: **0 errors, 0 warnings**. This evidence is authoring and
static validation, not a human xEdit verdict or gameplay PASS.

**Human xEdit evidence:** the user explicitly attested PASS for Sort Masters
(`Skyrim.esm / Update.esm / Dragonborn.esm`), both error checks, QUST,
DIAL/DLBR/INFO, conditions, QF/TIF VMAD, French text, save, reopen and final review.
The private session `926a8ab42da345219dba588ef604c7e3` retains this human review in
`xedit-validation.log` and `validation.json`; the log is a faithful transcript
of the user's attestation, not a raw xEdit Messages export. Its SHA256 is
`48A80BC620C349D17A58237864B7AF20B8B72E7C81403BE246339231488DA344`.

**Real import and post-import evidence:** after a successful fetch, the clean
preparation branch/HEAD, origin/main ancestry, tool identities and all original
Data/Git hashes were rechecked. `ImportValidated` completed through the existing
CK importer. The saved candidate, frozen validated copy, installed Data ESP and
repository ESP all have SHA256
`0058D2BD28C5BBEF5CCC14D09767EB2CB1BB8C22D81F3604C1D02187E01DB97F`.
All eight companions match across these locations. The canonical records
manifest adds exactly the seven real records at local IDs `0D6B14` through
`0D6B1A`; independent binary auditing confirms unchanged existing record payloads.
`SkyrimTogether.esp` is unchanged in both Data and Git. The existing packaging
manifest helper adds the eight companions, bringing its count from 19 to 27,
without executing the build/deploy entry point.

The six post-import audits pass: packaging, strict records with
`--reject-unexpected`, MQ101 structure, generated invariants, character-build
catalog and seat markers. The post-import suites pass **134 Python tests** and
**66 PowerShell safeguard cases**; `git diff --check` also passes. Both real
QF/TIF fragments were compiled directly with `PapyrusCompiler.exe` during
preparation, with **0 errors, 0 warnings**; there is no SSEScript dependency.

**Build/deploy and human Solo acceptance:** the existing local workflow completed
on `8398037112757318c12537dbefa7dbca105a6657`, with 27 managed files verified,
unchanged CK hashes, clean Git state and no agent game launch. Windows run
38664757655 and Linux run 38664757637 passed on that exact revision. The maintainer
then explicitly attested **complete Solo PASS, no error/crash**: Helgen dialogue,
readable Valen text, exact player choice and final response, journal title,
objective 10 correct/active, read-only `getstage = 10` and `sqv = running`,
non-repeatable acceptance and no technical Helgen regression.

**Scope correction after that Solo test:** remove only
`STRE_QUEST_ValenHelgen` from the native non-syncable EditorID array. The previous
per-player acceptance assumption is superseded by standard Skyrim Together
party quest synchronization. No generic QuestService behavior, server relay,
message/schema, CK content or generated companion changes are needed. Source
contracts cover narrative eligibility, preserved technical exclusions, the
Side Quest/experimental-setting boundary and existing send/receive paths.
The existing human xEdit and Solo content evidence remains tied to identical CK
bytes; it is not evidence of multiplayer propagation on the corrected native
policy. The separate human roster-2 acceptance below now covers both directions.
A general Shared / PlayerLocal / TechnicalProjection classification is a future
evolution only. ADR-0018 recovery remains unchanged; stage synchronization does
not reconstruct a native save. Broader Valen presentation and later narrative
stages remain deferred.

**Correction verification:** all **139 Python tests** pass, including five new
quest-sync source contracts; Windows/Linux CI now includes those contracts.
The **66 PowerShell safeguard cases**, all six CK audits and `git diff --check`
pass. The existing unchanged TPTests target passes **432 cases / 45,334
assertions** using a writable isolated temporary directory. Comparison against
`83980371` confirms exactly one native line removed, no protocol changes and
byte-identical ESP/eight companions in Git, Data and validated snapshots. No new
human xEdit review is needed for this native-only change.

**Local collector regression:** a read-only preflight detected three
`.wav.vortex_backup` files that the workstation's broad voice collector would
have imported. It stopped before the workflow ran, preserving the files.
The authorized correction adds only two case-insensitive exact-suffix guards:
`Add-OwnedFile` excludes them from ownership and `Test-CkAutoImportPath` excludes
them from import eligibility. The ignored local script remains unversioned;
the runbook and tracked `Test-StreCkCollector.ps1` preserve the contract.
The fixture reproduces the old failure and passes **13 checks** after correction,
including real WAV retention, mixed case, future voice extensions, manifest
exclusion and backup preservation. This is tooling evidence, not roster-2 PASS.

**Human roster-2 acceptance:** on October 15, 2026, the maintainer explicitly attested:

> J’ai effectivement testé A→B puis B→A sur le build e6a18213. Les deux directions sont PASS, aucune erreur.

This explicit statement is the roster-2 evidence. The earlier statement
"tout est PASS, aucune erreur" concerned the Solo smoke; its erroneous
attribution to roster-2 is withdrawn. The build actually tested is
`e6a182138a17d8d143fbfd0976a76b34734ddfb3`, client SHA256
`D4CF33B57472EB51D6BE7C5A052D67E61B7C80A809252228B91716D67C44DA3F`.
For each direction, the initiator receives the quest at stage 10/running;
the recipient receives it automatically without accepting, with objective 10
visible/active and stage 10/running. The recipient's acceptance option then
disappears. No duplicate, technical Helgen regression, crash or error was
reported. The checklist requires fresh processes and a separate clean campaign
for the reverse direction, with only read-only quest console commands.

This is a **maintainer human attestation**, not an agent-executed smoke or an
independent analysis of gameplay traces. The [dated runtime report](../audit/history/VALEN_HELGEN_ROSTER2_20261015.md)
records the two-direction matrix and evidence limits. The existing build/deploy
on that revision passed with unchanged ESP/eight companions and protected
SkyrimTogether.esp; Windows run 38719181490 and Linux run 38719181617 also passed.
Recording this acceptance changes documentation only; the tested binaries and
CK content are not rebuilt, re-imported or re-authored. This smoke does not
validate later quest stages, Stop propagation or campaign recovery behavior.

## Codex / AutoMod / official xEdit infrastructure (2026-10-15)

Implemented for [#92](https://github.com/matthieuAlbertelli/Skyrim-Together-Reborn-Enhanced/issues/92):
the Windows PowerShell adapter under Tools/Scripts prepares isolated copies,
enforces clean feature branches containing the fetched origin/main and the
single-plugin write policy, verifies
AutoMod JSON/dry-run/record results, and blocks import pending an explicit human
xEdit attestation. The runbook owns the workflow and its limits:
[MODDING_TOOLCHAIN](../development/MODDING_TOOLCHAIN.md).
There is no unattended xEdit validation/save implementation. The shared process
guard covers game/CK/Vortex and six xEdit names; the ignored local importer and
build/deploy scripts were connected to it without publishing their machine paths.

**Automated local evidence:** `Test-StreModding.ps1 -RealTools` passes **61
cases**: 59 synthetic safeguard cases plus real AutoMod GLOB dry-run and
write/reread cases in a disposable synthetic Git/ESP fixture. The real write
stops at `HUMAN_XEDIT_REQUIRED`; no quest was created. Synthetic import tests use
a fixture importer and explicitly simulated receipts, not xEdit validation.
The tests verify all four repository/installed STRE_AlternateStart.esp and
SkyrimTogether.esp hashes unchanged. Windows CI is configured for the default
59-case fixture suite; this statement does not claim a remote CI result.

**PR #93 review follow-up:** rebased onto main
`f9e550ccc469546875cb4ca9d54707ad6e9471b6`, preserving all of its status evidence
below. The mutation guard rejects missing/invalid origin/main and any failed
ancestry check, including stale and diverged feature histories. The runbook
requires a successful `git fetch origin` before each mutation action because a
local tracking ref alone cannot prove remote freshness. Four added tests exercise
an accepted descendant, a stale branch, a diverged branch and missing origin/main
even with a local main present. Both the default 59-case and real-tool 61-case
suites were rerun successfully. Rebuilt Debug TPTests passes **432 cases / 45,334
assertions**; all **119 Python tests** and the six CK audits pass. PowerShell
parsing, local documentation links and `git diff --check` also pass. No new
gameplay or xEdit runtime validation is inferred from these checks.

Real read-only inspection passes for the current ESP info, the exact three
masters and four QUST. `papyrus status` reports compiler and decompiler available;
no Papyrus compilation was performed. SkyrimSE.exe reports **1.6.1170.0**;
official xTESEdit.exe reports **4.1.5.0**, SHA256
`CCB7CA9E6D826868B186B0B9A0FC012AFFB3FDC9688250583353E6D84E6A2545`.
The tested AutoMod CLI DLL SHA256 is
`D7969A1AE8A664474C24645D511074A1128028AA151A21FD4552C412A517B846`;
the local toolkit extraction has no Git metadata, so no source commit is claimed.
Real-tool testing found a French-locale float parsing failure; child-only
invariant globalization corrects it and the 7.25 GLOB case now passes.

No production ESP/PSC/PEX, gameplay, campaign authority, protocol or persistence
changed. Actual candidate Sort Masters, Check for Errors, save/reopen review,
production CK import, build/deploy and in-game acceptance remain human/future
content-mission steps. Neither a real xEdit validation run nor a production
quest-creation smoke is claimed by this infrastructure evidence.

## Automatic Helgen integration with the cell-transition correction (2026-10-14)

The integrated candidate combines main
`1a997c3037cf55a3127bb323458e4ea16f9fec7f` (the squash of PR #90) with the previous
Helgen candidate `5a66e9413f26aa48e862fb1f1e6f447a69e163e8` using a merge commit,
without rewriting the published branch. There were no conflicts: all 21
non-documentation files changed by PR #90 match main exactly, and all 26
non-documentation files changed by PR #88 match its previous head exactly.
The only additional edits record integration evidence and the next runtime
procedure in this document and TEST_PLAN.

The bounded seating lifetime, cell-scoped Furniture invalidation, native
binding/target/cell validation before ForceAction and value-only diagnostics are
preserved. So are the full-roster Applied barrier, local authorization/event,
campaign-required lifecycle, running cleanup-controller projection owner,
startup/retry/no-replay guards, sole Papyrus MQ101 sequence, T0-after-projection,
quest-sync exclusions, duplicate idempotence and existing T+4 behavior.

**Integrated automated evidence (Windows x64 Debug):** TPTests builds and passes
**432 cases / 45,334 assertions**. Filters pass: [helgen] 10/182, [seating] 8/94,
[animation] 7/73, [appearance-apply] 11/297, [final-respawn] 17/34,089 and
[animation-replay] 12/340. All **119 Python tests** pass. Client, server and
immersive launcher builds pass with `-j 2`, including the production UI build.
Existing ignored -fPIC, signed/unsigned and macro-redefinition warnings remain.
The six audits pass: CK packaging (19 managed files, no misplaced PEX), strict
record manifest, MQ101 structure, generated invariants, build catalog and ten
creation markers. ESP/PSC/PEX did not change during integration; all four PEX
from PR #88 match their retained compiler outputs byte-for-byte. No recompilation
of unchanged PSCs was needed. `git diff --check` and documentation links pass.

**Previously executed automated validation on `d9501d57`:**
[Windows CI](https://github.com/matthieuAlbertelli/Skyrim-Together-Reborn-Enhanced/actions/runs/38618138860)
and [Linux CI](https://github.com/matthieuAlbertelli/Skyrim-Together-Reborn-Enhanced/actions/runs/38618138963)
are SUCCESS. The tests/builds/audits above belong to that candidate and were not
rerun for this documentation-only attestation. Automated evidence is separate
from the maintainer's gameplay results below.

**Integrated candidate runtime: PARTIALLY VALIDATED BY MAINTAINER** on
`d9501d57c7472e276b2ad8397d203b093a16762c`, attested on 2026-10-14.
**Runtime gameplay: NOT RUN by agent.**

The tested runtime was restored to **Skyrim SE 1.6.1170**, with the matching
Address Library. Steam's accidental update to **1.7.104** was downgraded before
validation; this evidence does **not** establish Skyrim 1.7.x support.

| Maintainer-tested scenario | Result and exact scope |
|---|---|
| A-first | PASS: A finishes while B remains in creation; no early Helgen start. Collective automatic triggering occurs after B finishes. |
| Automatic state after final Applied | PASS without manual setstage: MQ101=1000, cleanup=40, investigation stage=10, InvestigationState=1, InvestigationStartGameTime/T0>=0. |
| Post-attack Helgen | PASS: automatically projected post-attack state and no occupation bandits before T+4. `coc HelgenExterior` was only an observation shortcut, not the trigger. |
| B-first | PASS: no early start before the last player; automatic collective triggering after the final Applied, with expected stages/states. |
| Cell transitions / prior crash | PASS only for the tested exits/cell changes; the critical crash did not recur. This is not recovery-matrix acceptance. |
| T+4 with the full roster outside Helgen | PASS: after four elapsed game days from T0, occupation triggers and bandits appear. |

Console commands `getstage MQ101`, `getstage STRE_QUEST_HelgenNPCCleanup`,
`getstage STRE_QUEST_HelgenInvestigation` and `sqv STRE_QUEST_HelgenInvestigation`
were read-only observations. No manual `setstage` was used.

**Pending runtime matrix (not PASS):**

- T+4 with one player still in Helgen: expect Pending and no physical occupation
  until the last player exits;
- inverse player roles for that Pending case;
- presence in HelgenKeep01 at T+4;
- disconnect/reconnect before the final Applied;
- recovery/checkpoint after authorization;
- duplicate/repeated readiness in manual runtime;
- complete Solo regression on the integrated candidate;
- same-process New Game regression;
- all remaining recovery variants in TEST_PLAN.

Seating remains **KNOWN ISSUE / DEFERRED / NON-BLOCKING BY MAINTAINER DECISION**,
tracked in [issue #91](https://github.com/matthieuAlbertelli/Skyrim-Together-Reborn-Enhanced/issues/91).
It is **not PASS**; Helgen runs do not validate seating. Neither PR #88 nor the
complete T+4/recovery matrix is exhaustively validated. The remaining procedure is
in [TEST_PLAN](../features/alternate-start/TEST_PLAN.md#integrated-pr-88-candidate-after-pr-90-2026-10-14).
This follow-up changes documentation only; no code/ESP/PSC/PEX change, local build,
Skyrim/CK launch, deployment, merge, tag or release was performed.

## Multiplayer loading-door regression candidate (2026-10-11)

[Issue #89](https://github.com/matthieuAlbertelli/Skyrim-Together-Reborn-Enhanced/issues/89)
tracks the fatal roster-2 transition after Character Creation, corrected in
[PR #90](https://github.com/matthieuAlbertelli/Skyrim-Together-Reborn-Enhanced/pull/90).
This is separate from PR #88. The correction is based on main
`cba7569c74140b4ffa9a180aad94ebd2f1a865e3`, in an isolated worktree.

**Human runtime acceptance (maintainer attestation, 2026-10-14):** the critical
issue #89 reproducer is **PASS** on
`974b12fc9285831443d34f32f18b1d40eb100393`.

- Fresh sealed roster 2: the normal inn interior-to-exterior loading door works.
- Cell changes and campaign continuation work; no new crash or recovery lock
  related to this regression was observed.
- General behavior outside seating conforms within the tested scope.
- Seating still has functional bugs: **KNOWN ISSUE / DEFERRED / NON-BLOCKING BY
  MAINTAINER DECISION** for PR #90, tracked separately in
  [issue #91](https://github.com/matthieuAlbertelli/Skyrim-Together-Reborn-Enhanced/issues/91).
  Seating is **not PASS**. The maintainer explicitly deferred its correction;
  accepting the crash fix does not establish functional seating quality.

No new logs/captures characterize the seating symptoms in this attestation, so
no more specific symptom or cause is asserted. The original crash dump/log ZIP
are lost. This clears the issue #89 reproducer blocker for continuing PR #88
runtime validation on the validated correction; it does not validate PR #88
itself or imply that the correction has been merged. Exit-order permutations,
re-entry/re-exit, recovery and Helgen checks not individually attested in this
report remain unconfirmed; the entire test matrix is not marked PASS.

Both [Windows CI](https://github.com/matthieuAlbertelli/Skyrim-Together-Reborn-Enhanced/actions/runs/38171040331)
and [Linux CI](https://github.com/matthieuAlbertelli/Skyrim-Together-Reborn-Enhanced/actions/runs/38171040414)
are **PASS** on `974b12fc`. The 2026-10-14 follow-up changes documentation only;
runtime and native-test evidence remain tied to that exact candidate, not to a
new gameplay run of the documentation commit.

**Historical runtime evidence before the correction:** Solo and roster 1 passed;
roster 2 failed when a player took the ordinary inn loading door after reciprocal seating.
`9108727b` passed; `605ef668ce9a9311fe6df713064038e9d09643c3` passed with CK artifacts
explicitly aligned. `6dfba1594e35852d106be0b4785dd5280a58fa8d` and later failed in
older runs, but auto-import prevents treating their artifact fidelity as proven.
The exact interval is the seating PR #84. Both historical launcher/server builds
have now passed in isolated worktrees using Git content, without CK import or
local deployment. Their matched packages/hashes support a new human comparison;
no new historical gameplay result is claimed.

**Source findings and implemented correction:** remote intents previously kept
querying the final committed binding; diagnostic callbacks traversed borrowed
actor/process/package/furniture data, and binding replay had no cell scope.
These are demonstrated source-level lifetime gaps. The new final binding signal
is only emitted by the appearance commit, so replay on every ordinary cell
transition was not established. Remote observation is now bounded and terminal;
callback diagnostics use values rather than native pointer walks. Furniture
history/queued entries are scoped to cell continuity and fresh compatible targets,
with stale-generation and newer-exit fences, while ordinary animations survive.
Discarding the last invalid furniture action also clears its deferred graph reset.
The local seat request, final materializer and appearance contract remain.
Details belong to [the feature contract](../features/alternate-start/CHARACTER_APPEARANCE_SYNC.md#creation-cell-lifetime-and-transition-regression-2026-10-11).
No protocol, persistence, authority, CK, PSC, PEX or manifest change.

**Automated evidence:** Windows x64 debug TPTests passes 45,176 assertions in
425 cases; [seating] 8/94, [animation] 7/73, [appearance-apply] 11/297,
[final-respawn] 17/34,089 and [animation-replay] 12/340. Requested [respawn]
matches no test in this repository; the explicit final-respawn run supplies that
coverage. All 109 Python tests pass. The client, server, immersive launcher and server runner builds pass. One
MSVC C1090/PDB RPC failure during an overlapping build cleared on the isolated
retry; the final sequential build set passed. Existing warnings remain
ignored -fPIC and the TESObjectREFR signed/unsigned comparison. No CK audit was rerun because no managed
content changed. The first observation-only commit also passed the full C++/Python
suites and the client build independently.

**Evidence limits:** no gameplay was run by the agent; runtime acceptance above
is the maintainer's report. The reported historical dump had C0000409 / parameter
7 (FAST_FAIL_FATAL_APP_EXIT), and an E06D7363 breakpoint caught no C++ exception.
The matching PDB alone cannot establish a faulting stack without the lost dump.
The critical reproducer now passes on `974b12fc`; the exact historical good/bad
boundary and native faulting instruction remain unproven. Remaining scenarios
belong to the [human loading-door matrix](../features/alternate-start/TEST_PLAN.md#loading-door-regression-acceptance-issue-89-2026-10-11).
No buffer-overflow, C++ exception, Helgen, PR #88 or Main Menu cause is asserted.
Roster 2 uniquely exercises remote observation/replay; this explains the audit
focus, not a proven instruction-level cause. Historical in-inn seating acceptance
below does not validate seating on this candidate or replace the dated door evidence.

## Startup and Main Menu presentation (2026-09-23)

Implemented for [#86](https://github.com/matthieuAlbertelli/Skyrim-Together-Reborn-Enhanced/issues/86).
The [feature contract](../features/main-menu/README.md) owns behavior and asset
provenance; its [test plan](../features/main-menu/TEST_PLAN.md) owns acceptance.
This consolidated checkpoint replaces the earlier preparation/UX snapshots;
their exact intermediate diffs and evidence remain in Git history.

The existing native client, MainMenuRuntime, ImGui/DX11 pass and Windows Media
Foundation decoder implement once-per-process intro, configurable keyboard/gamepad
skip, held-input suppression, intro-only cursor/music gating and bounded vanilla
fallback. Installed Skyrim key art/font are rendered by an inert Scaleform host;
the hint is not ImGui text. Independent center-left PNG branding and localized
subtitle retain their staggered reveal; the backdrop follows the emblem fade.
No new SKSE plugin, SWF, runtime decoder dependency, campaign state or ADR.
Runtime-sensitive hooks remain fenced to exactly 1.6.1170.0 / SKSE 2.2.6 target;
no 1.7.x support is claimed.

The distribution source now contains the maintainer's silent **forward + reverse**
background export and three independent PNGs. The maintainer selected shipped
`BackdropOpacity=1.0`; compiled fallback remains 0.35. Background visual authorship
is confirmed as Matthieu Albertelli. The final source/export identities and rights
record live only in [asset provenance](../features/main-menu/README.md#asset-provenance-gate).
The maintainer withdrew the local intro from distribution while awaiting Lawrence
James's music permission. Its original bytes remain untouched and untracked.
The package therefore opens directly on background/branding when no separately
supplied intro exists. Useful forward/ping-pong intermediates were preserved
outside GameFiles in the ignored local audit source directory, not distributed.

**Human evidence (maintainer attestation):**

- 2026-09-22 first smoke: trailer, audio, Escape skip, transition to Main Menu /
  background, and general behavior after intro: PASS.
- 2026-09-23: current in-game behavior described as satisfactory, specifically
  including the continuous ping-pong background: PASS at the reported scope.
  The maintainer also explicitly selected backdrop opacity 1.0 for distribution.
- 2026-09-23 final smoke of the distributed package **without `intro.mp4`**:
  PASS, explicitly confirmed by the maintainer on Skyrim 1.6.1170 / SKSE 2.2.6.
  This closes the final-package smoke requirement at that reported scope;
  intro music permission and the remaining human matrix are not covered.
- 2026-09-23 return from gameplay: PASS; the intro did not replay, and the
  background and branding were correct.
- 2026-09-23 Alt-Tab / focus loss and regain during both intro and Main Menu:
  PASS; cursor, input and menu behavior were correct.
- 2026-09-23 temporary absence of `background.mp4`: PASS; clean fallback to
  the vanilla menu, without crash or blocked interaction.

These reports do not identify exact executable hashes, resolution/aspect,
controller model or logs/captures. The three final checks validate only the
reported scenarios, not the entirety of their combined test-plan cases. They
do not establish exhaustive native hint/cursor-exit or vanilla/campaign-action
coverage. Repeated round trips, exit during partial branding reveal, remaining
failure modes (including corrupt media), 21:9, all resolutions/languages/controllers
and 1.7.x remain unconfirmed. No agent-operated in-game run is claimed.

**Automated evidence:** native code is unchanged since validated head
`5da99f8aeebe56074080270d24927d5726ee672d`; no redundant native rebuild was run for
this media/configuration/documentation consolidation and Python packaging checks.

- At that head, Debug and releasedbg builds of TPTests and
  SkyrimImmersiveLauncher (including production Angular): PASS. Per mode,
  `TPTests.exe "[main-menu]"`: 1,277 assertions / 27 cases; default suite:
  45,082 / 417; `TPTests.exe "[main-menu-branding-assets]"`: 13 / 1, all PASS.
  Windows and Linux CI for that head also passed. Debug configuration retained.
- Earlier unchanged-decoder smoke: generated H.264, WARP frame transfer, EOS,
  loop, missing/corrupt media and cancellation during load: PASS in Debug and
  releasedbg. The native-resource address audit is static evidence only.
- Current `python -m unittest discover -s Tools/Scripts -p 'test_*.py'`:
  PASS, 108 tests. `python Tools/Scripts/test_main_menu.py --package
  _audit/main-menu-final/package/Data`: PASS, 16 checks. New checks reject
  intermediate exports, excluded intro distribution, oversized files and a
  background hash/size differing from the canonical provenance.
- Current `python Tools/Scripts/audit_ck_packaging.py`: PASS, 19 managed files,
  zero compiled PEX under Scripts/Source. Full tracked Data payload assembled
  from the Git index, matching clean-checkout CI inputs: 236 files,
  107,721,486 bytes; every copied file hash verified. Expected MainMenu PNG/INI
  and background are present; intro, po3 DLL/INI and MainMenuVideo prototype
  directory are absent. This checks Data, not a new complete binary release.
- Both local MP4s inspected with `ffprobe -v error -show_format -show_streams
  -of json <file>`: H.264/MP4, 1080p60; intro has AAC stereo, final background
  has no audio stream. `ffmpeg -v error -xerror -i <background> -f null NUL`:
  full decode PASS. Frame-sequence comparison confirms 186 forward frames plus
  186 reversed frames (372 total); sampled low-resolution luminance differences
  remain below 0.37/255 per-frame mean against that expected order. This supports
  the transformation, not a universal visual-quality claim.
- Targeted code review found no required native change: input/cursor/music leases,
  close/reset/skip/EOS/error exits, DX11/MF/Scaleform ownership, fixed Data paths,
  bounded localization, runtime fence and po3 independence retain their contracts.
  `dumpbin /dependents build/windows/x64/releasedbg/SkyrimTogether.exe` has no
  Main Menu Video or mandatory mfplat/mfreadwrite import.
- Changed Markdown targets and `git diff --check`: PASS. ROADMAP, acceptance index,
  WBS, Dependency Map and Brief Catalog were reviewed and retain their canonical
  links without duplicated status. GPL attribution remains in NOTICE and Data.

No build/deploy/import script or CK manifest was changed. No deployment or new
branch/worktree was performed during consolidation. CI and merge history belong
to PR #87. Intro music permission and the unconfirmed human matrix
remain acceptance limits; the intro stays excluded from distribution.

## STRE 0.4.0-alpha.1 published release checkpoint — PASS (2026-09-19)

- Product version: `0.4.0-alpha.1`.
- Tag: `stre-v0.4.0-alpha.1`.
- Release commit: `6dfba1594e35852d106be0b4785dd5280a58fa8d`.
- Annotated tag object: `d6c8b8a356838e03007a67ffbd6f74857d451544`.
- Published: `2026-09-19T10:57:15Z`.
- [GitHub Release](https://github.com/matthieuAlbertelli/Skyrim-Together-Reborn-Enhanced/releases/tag/stre-v0.4.0-alpha.1):
  prerelease (`prerelease=true`, `draft=false`).
- [Playable workflow 35437245976](https://github.com/matthieuAlbertelli/Skyrim-Together-Reborn-Enhanced/actions/runs/35437245976):
  completed/success on the release commit, triggered by the tag push.
- Published ZIP: `STRE-v0.4.0-alpha.1-windows-x64.zip`.
- ZIP SHA256: `57dcc435ab6728782a4e4242e28e7286dff7fc75bbb1e5b7ecc6e4c55f66bf3c`.

**Tagged clean-install human smoke: PASS**, attested by the maintainer on this
exact published ZIP, identified by the checksum above. The observed checks were:

- clean installation / dedicated profile: PASS;
- client startup and UI/F2: PASS;
- New Game / Ilinalta's Vigil: PASS;
- separately installed external EEK resource without Embers HD: PASS;
- no missing/purple textures; carvings, embers and wood rendered correctly: PASS;
- fireplace collision and circulation: PASS;
- RaceMenu / Character Creation, final appearance and local seating: PASS.

The prior roster-2 acceptance in both completion orders remains separate
functional evidence, recorded below. It is not claimed to have been repeated
with this public ZIP. This smoke does not establish ranks 2..9 seating, a complete
race/sex matrix, MASTER in-game validation, recovery/reconnect or native
retirement/memory completion. These limits remain unchanged.

The immutable annotated tag and published GitHub Release identify this player
release. Later documentation commits on main do not change that identity or its
assets. This entry records publication and human acceptance; it does not claim
new agent-executed gameplay, TPTests or structural-test runs.

## EEK Vanilla Textured fireplace runtime checkpoint — PASS (2026-09-19)

The maintainer validated the exact ESP candidate in the development environment:
no missing/purple textures; carvings, embers and wood rendered correctly; size
and origin/placement visually unchanged; collision unchanged; circulation
unaffected. This is human runtime evidence, not an agent-executed runtime test.

Only `STRE_STAT_IlinaltaFireplace01` (local `0xCAF31`) MODL changed from
`EEKs Fireplace Resources\HDEmbers Textured\EEK_DragonsReach_Firepit_Kitchen_HDEmbers.nif`
to `EEKs Fireplace Resources\Vanilla Textured\EEK_DragonsReach_Firepit_Kitchen.nif`.
Both models have 106 BSTriShape, matching hierarchy/transforms, vertex positions,
triangles and bounds, and five byte-identical collision blocks. Two shapes have
UV differences; shader/controller differences provide the normal fire textures.
The installed, candidate and imported repository ESP share SHA256
`299dafea9b6a7fa272b2319daa5c66866194cd6b1963064f57145e8294d5d9de`.

EEK remains a separately installed external prerequisite, credited to EvilEyedKyo
/ EEK: [resource and files](https://www.nexusmods.com/skyrimspecialedition/mods/31562?tab=files).
STRE redistributes no EEK or Embers HD assets. Embers HD is no longer a direct
dependency of the selected fireplace.

The exact original provenance of `textures\eeks whiterun interiors\smim\wrcastlecarvings.dds`
and `wrcastlecarvings_n.dds` has not been independently established. Their runtime
use was accepted by the maintainer for `0.4.0-alpha.1` after successful visual
validation. STRE does not redistribute these files; this acceptance is not
evidence of standalone redistribution permission or vanilla origin. The tagged
`0.4.0-alpha.1` clean-install smoke verified the external EEK configuration and
complete fireplace rendering without Embers HD for the published package. That
runtime result does not resolve the two textures' exact original provenance.

Promotion checks rerun: six CK/ESP audits PASS (strict records with
`--reject-unexpected`, ten marker/seat pairs, packaging of 19 managed files,
MQ101 structure, generated invariants, 41 catalog references); nine structural
suites / 92 tests PASS; git diff --check PASS. The 249-record comparison finds
only the fireplace MODL change; aliases, quests, navmesh, markers/seats, script
properties and master overrides are unchanged. PSC/PEX are unchanged. No C++
change, full TPTests rerun, new game launch or clean-install test was performed
during that promotion. The later tagged-package human smoke is recorded above.

## Accepted multiplayer seating checkpoint (2026-09-19)

**Maintainer human acceptance: PASS**, for two fresh roster-2 runs in both
completion orders (A first, then B first). Each player retains correct local
MarkerXX -> SeatXX sitting after their own Applied; both observers see the
other's correct final appearance, without the placeholder/black Viking, and
visible remote sitting after final rematerialization. No all-Applied barrier.
The native furniture interaction belongs only to the owning PlayerCharacter;
remote presentation uses STR actions and binding replay, never remote Activate
or remote MoveTo. This supersedes the pending visual acceptance statements in
historical preparation entries below, for this tested slice only.

The 19 September A-client log documents A Applied at 02:47:46.615 and local
activation at .695, before B Applied at 02:48:13.135. B/serverId 3/revision 2
waits in weapon 4 at 02:48:13.206, returns to safe weapon 0 at .570 (363 ms
reported), then commits at .655. Old FF000834/token8F590DB0 becomes
FF00083B/token8FB44500. At .655, one consumed IdleChairRightEnter is refined
to IdleChairEnterInstant and prepended before two pending actions (queue 2 -> 3).
At .656, ForceAction returns true on the new token, matchesNewBinding=true.
The server records B Applied at .129, appearance owner acceptance/relay to A
at .150. These timestamps establish the A-first trace, not the reverse run.
The B-first order and reciprocal visual results are the maintainer's explicit
attestation; no exact second-run timestamp or B-client trace is inferred.

The installed client inspected during closure matches the prepared candidate:
SHA256 932E650C4B0FD3CB6684E6B707D00AC3A8E54CF28983F8C42B9FF96FD6272A5B.
This identifies the inspected binary; the logs do not themselves contain a
cryptographic executable fingerprint for each historical process. Source and
runtime evidence copies remain local under _audit/seating-final-checkpoint/.

Final closure reruns PASS: full TPTests 43905 assertions / 400 cases; seating
1529 / 16; final rematerialization/admission 34089 / 17; animation binding replay
340 / 12; nine structural suites, 92 tests. Windows Debug client/launcher and
server builds PASS (native targets up to date; Angular rebuilt). Six CK audits
PASS: ten marker/seat pairs, strict records with reject-unexpected, packaging
(19 managed files), MQ101 plugin structure, generated source invariants and
catalog references (41). git diff --check PASS. No CI run or new MASTER build
was performed during closure; prior isolated MASTER compile evidence below is
not a full executable/runtime validation. No functional change or Papyrus recompilation is
part of closure. ESP/PEX are byte-identical to HEAD; PSC is identical after Git
line-ending normalization. No asset will be added by this checkpoint.

Scope limits remain: no runtime validation of ranks 2..9, complete race/sex
matrix, MASTER executable, recovery/reconnect or repeated lifetime cycles is
inferred. All ten existing marker/seat bindings are statically audited, not all
ten poses tested. The origin of transient weapon 4/5 remains unresolved; the
bounded wait preserves every safety predicate and requires safe admission.
Native retirement/memory completion, Valen and collective progression remain
outside this checkpoint. No agent Skyrim/CK launch or deployment; no push.

## Animation replay continuity at final binding replacement (2026-09-19)

Implemented and accepted for the roster-2 checkpoint above: [ADR-0027](../architecture/ADRs/ADR-0027-animation-replay-native-binding-continuity.md).
ActionReplayCache and AnimationEventLists now have one shared implementation in
SkyrimEncoding. Server refinement semantics/mappings and ActionReplayChain wire
format remain unchanged. Each remote animation component keeps up to 32 consumed
actions; final binding notification prepends their refined chain before existing
pending actions. Pending actions are not duplicated. Consumed/pending exits
invalidate stale history; an exit arriving during graph wait cancels only the
synthetic prefix. Duplicate generation notifications cannot replay twice. Fresh
Actor FormID/token comparison, disconnect/new-session/recovery invalidation and
normal graph readiness remain. No borrowed engine pointer is retained.

RemoteRespawnLab adds only a value-only notification after candidate-commit.
Its admission, weapon 4/5 wait, transaction, FaceGen and retirement logic are
unchanged. Appearance publication, natural-join projection, Discovery, individual
local MarkerXX -> SeatXX seating and ESP/PSC/PEX match the mission baseline hashes.
No remote Activate/MoveTo, new ActorState assignment, animation primitive, packet,
schema, collective barrier, race-specific handling or lifetime investigation.
Functional replay is MASTER/non-MASTER; local seating remains non-MASTER.

Evidence establishing the discontinuity, from the **previous diagnostic candidate**
SHA256 18177A4540CB46E5B1BE911A50D36A141ED5197D06831CE995A1211C58D65E2C:
on observer A at 02:12:08.318, B/serverId 3/revision 2 Applied is received. Final
snapshot arrives at .366, admission becomes Ready at .752, candidate-create .753,
candidate-commit .843. At .844 the saved real ForceAction result identifies
IdleChairRightEnter (actionTick 51862521, idle 3B070) returned 1 on old
FF000826/token9365ADF0, not final FF000831/token930DFE60. Execution is bounded
between .320 and .820; native diagnostic saturation hides its exact wall time.
Four actions received/two executed before commit, two pending preserved, followed
by moveStart/TurnRight/moveStop/turnStop on the final actor. At +40 s the counts are
six received, zero filtered, six executed; no chair entry was replayed on final.
Server traces confirm Applied/appearance ownership/relay, not individual action
timestamps. No B-client chronology is claimed.

The maintainer attests final appearances correct in both directions and local
sitting correct for both players on that diagnostic run; remote sitting FAIL in
both directions. This supersedes earlier pending/not-deployed statements for the
diagnostic candidate below, but is **not** runtime acceptance of the new replay
correction or proof of both completion orders/native lifetime.

Executed for this correction: targeted TPTests **340 assertions / 12 cases PASS**;
full TPTests **43905 assertions / 400 cases PASS**; all nine structural suites
**92 tests PASS**. Windows Debug client/launcher and server builds PASS. Isolated
MASTER/IS_MASTER=1 compilation of AnimationSystem, CharacterService,
RemoteRespawnLab and CreationSeating PASS; not a linked MASTER executable or game
test. git diff --check PASS. Existing compiler narrowing/deprecation warnings and
ignored -fPIC remain, including the pre-existing AddActionsForReplay narrowing.
Initial attempts exposed a test-only const mismatch, a structural assertion that
mistook interpolation TimePoints for animation mutation, an invalid build target
name, and a missing forced PCH in the isolated compile command; corrected before
the passing executions. Sandboxed xmake also emitted Git ownership metadata errors;
final builds ran with the normal Windows build environment and correct branch.

Dedicated [AnimationReplay] logs record source/refined chain, binding/generation,
bounded graph wait and actual ForceAction token/result independently of the native
probe budget. The prepared candidate was subsequently installed and tested by the
maintainer, with final acceptance recorded above. ForceAction return alone remains
insufficient for visual acceptance. Procedure:
[TEST_PLAN](../features/alternate-start/TEST_PLAN.md#animation-replay-continuity-after-final-binding-2026-09-19).

## Remote seating action-stream diagnosis (2026-09-19; historical preparation)

Implemented on the existing arrival/seating worktree: CreationSeating no longer
calls Activate for remote actors, in any build. Only the native local
PlayerCharacter reaches the existing activation path. The non-MASTER individual
MarkerXX -> SeatXX approach is unchanged. ADR-0026 supersedes ADR-0024's invalid
observer primitive; this is neutralization plus diagnostics, NOT a remote pose fix.

Native evidence on 1.6.1170: ActivateRef AE19796 dispatches through TESFurniture's
vtable slot 0x1B8 to RVA 0x269C00 (RTTI verified). The comparison at 0x269C31 is
against the PlayerCharacter singleton (AE401069/RVA 0x3137698); a different actor
returns false at 0x269EAB before that furniture method's package creation.
This proves a deterministic native veto, not the first executed instruction of
the outer wrapper in a runtime trace. No hooks or native guards were changed.

Newly analyzed evidence supersedes the old "not yet installed/tested" statement
in the admission preparation below: the installed client hash matches candidate
076ADAFCE13808F31A5F3EAD39EE4A36D1D47657464E2E18A3C58FF7DCA4D558.
On 19 September at 01:18, A receives B's Applied (serverId 3, revision 2), then
the final snapshot at 01:18:43.352. Weapon 4 -> 0 reaches Ready after 398 ms;
one transaction commits FF000846/token9374E2F0 at 01:18:43.881. The maintainer
attests B's correct final appearance on A. Seating then selects this final actor
and Seat02 080BF3DC, but the old Activate path returns false. The supplied A and
server logs do NOT trace B's chair action receipt or execution. No B logs are
requested. This is not a completed reciprocal/two-order appearance test matrix.

Added observer-only non-MASTER RemoteProbe: before/after Applied action receipt,
missing-view filtering, actual actor/token on dequeue and existing ForceAction
return, tick/graph wait, queue reset, committed binding selection, queue/last-action
evidence at binding change, bounded native graph/process/transform samples and
furniture/action callbacks. Identity metadata is immutable/atomic for callbacks;
no borrowed engine pointer or action payload is retained for replay. Ten identities,
40-second phase windows (first Enter +5 s), bounded sampling/action logs and dropped
line counters prevent missing logs being presented as proof of absence.

Static audit: final commit preserves RemoteAnimationComponent, but an action
already popped on the old actor is not automatically requeued on the new one.
STR's existing server spawn replay cache recognizes chair entry events. This is
an available existing mechanism, not proof that this run lost an action. No new
replay/buffering, protocol, ActorState write, forced animation, remote movement,
appearance publication/materializer/fence or server behavior change is implemented.

Executed in this mission: full TPTests PASS, 43565 assertions / 388 cases;
all eight structural suites PASS, 86 tests (including eight new observer checks).
Windows debug client/launcher and server builds PASS (server up to date).
Isolated CreationSeating and RemoteSeatingProbe translation units compile with
MASTER/IS_MASTER=1; this is not a linked MASTER executable or runtime test.
Initial build attempts hit Node sandbox EPERM, then a missing include corrected
before success; one new structural expectation typo was corrected before PASS.
Build warnings: existing ignored -fPIC and existing ReplayCount size narrowing.

No new Skyrim runtime was executed. No B chair action chronology or remote visible
sitting is validated yet. The prepared diagnostic client is not deployed; no
commit/push. The remaining test is observer A action/actor chronology in both
completion orders, plus human reciprocal final-appearance and local/remote sitting
acceptance. Missing source/relay evidence remains explicitly unknown, not inferred
from an empty observer action trace. Native lifetime remains deferred. Procedure:
[TEST_PLAN](../features/alternate-start/TEST_PLAN.md#remote-seating-through-str-actions---observer-diagnosis-2026-09-19).

## Bounded final-rematerialization admission (2026-09-19; preparation evidence)

Implemented on the existing arrival/seating worktree: official MASTER/non-MASTER
final rematerialization now separates Ready, Pending and Rejected. Only weapon
4 (WantToSheathe) / 5 (Sheathing) can wait, with all other guards passing. The
ActorState safety policy is unchanged: replacement still requires weapon 0 and
all existing predicates. All identity/provenance/runtime/data/geometry-readiness
guards formerly after the weapon veto are evaluated before Pending. Current
binding observations are fresh each update; value-only session/entity/version,
player/server, Actor/Base IDs and tokens fence the wait. No partial native capture,
transaction, candidate, retirement or remote seating is initiated while Pending.

The first final snapshot/revision is frozen. Applied matching and weapon settling
share a ten-second steady-clock deadline, never extended by duplicate delivery.
Conflict, lost binding, disconnect or recovery terminates admission. Ready captures
fresh placement/values and uses the existing single transaction. Post-reservation
and precommit guards, natural-join materializer, Discovery and seating code remain
unchanged. Diagnostics distinguish wait begin, weapon transitions, Ready, terminal
rejection and timeout, with a bounded transition count and unconditional outcome.

Evidence preceding this correction: local observer A received B's Applied and
final revision, but Capture rejected weapon=4 before reservation; Advance then
made the job terminal. Other guards after that veto had not been executed. The
origin of WantToSheathe is UNKNOWN; missing B-side logs prevent attribution to
markers, furniture or an equipment call. The maintainer separately attests visible
local MarkerXX -> SeatXX seating PASS in Solo, connected roster 1, and on both
local players of roster 2. That is seating evidence, not final-appearance proof.

Executed for this correction: TPTests targeted 34067 assertions / 17 cases PASS;
full TPTests 43543 assertions / 388 cases PASS; seven structural suites 78/78 PASS.
Client/launcher and server debug builds PASS. Isolated RemoteRespawnLab compilation
with IS_MASTER=1 PASS; this is not a linked MASTER executable or a MASTER game run.
The unchanged safety helper and native post-reservation checks remain covered.
The initial structural run found one stale Capture signature in a test, updated
before the passing run. git diff --check PASS; pre-existing seating sources and
ESP/PSC/PEX match their pre-mission SHA256 hashes. Existing ignored -fPIC build
warning remains.

Follow-up preparation extends both 4 -> 0 and 4 -> 5 -> 0 tests through the
existing pure lifecycle's Discovery/readiness/commit, rejecting premature and
duplicate commit and checking one old-retirement intent. Rebuilt TPTests and
reran targeted 34089 assertions / 17 cases and full 43565 assertions / 388 cases:
PASS. This is model-level commit evidence, not native projection or visual proof.
No further functional source or guard changes; client/server/launcher/MASTER and
structural results above were not rerun for this test/documentation-only follow-up.
A hash-identified copy of the built client is prepared under
`_audit/final-admission-wait/validation-candidate/`; it is not deployed. The local
installed client still has the previous hash and local logs still end before this
correction, so there is no new native admission/timeout/commit result to report.

At preparation time human validation was pending; the accepted checkpoint above now covers both directions: A observing B
and B observing A in each of two fresh runs (A finishes first, then B finishes
first). All four observations must reach candidate-commit and visually correct
final appearance while individual local seating stays correct. Persistent 4/5
must expire with no replacement and is NOT a fixed placeholder; upstream producer
and resolution then remain the next diagnostic question. Native memory retirement
completion remains deferred. No Skyrim/CK launch, deployment, commit or push.
No ESP/PSC/PEX changes or Papyrus compilation. Existing seating source/assets are
preserved. Procedure and interpretation: [TEST_PLAN](../features/alternate-start/TEST_PLAN.md#final-rematerialization-pre-reservation-weapon-wait-2026-09-19).

## Existing CK markers for local seating approach (2026-09-19; preparation evidence)

Implemented non-MASTER per the maintainer's corrected instruction: reuse the
existing STRE_REFR_PlayerCreationMarker01..10 and Seat01..10 by durable PlayerId
rank. No new SeatApproach CK references. C++ copies the exact marker position and
orientation; the Seat01/Seat02 furniture-offset and obstacle models are removed.
All ten local ranks share MoveTo -> measured arrival -> marker facing -> later
UpdateEvent -> Activate. Individual Applied, native local-player-only scope,
session/recovery/identity/occupancy fences, one request per token, 2-unit arrival,
0.05-radian facing and 5-second preparation bounds remain. No remote MoveTo,
new message, collective barrier, forced animation or ActorState write. Existing
remote projection and MASTER behavior are unchanged.

The read-only audit confirms all ten existing references, bases, persistent/
enabled flags, same cell and C++ pair mappings. Their physical validity as chair
entries is NOT established: the maintainer will reposition/orient markers in CK.
This also changes initial creation placement because those references are shared.
Old numeric spacing assumptions are replaced by distinct-reference/position
checks and human layout acceptance, not another C++ obstacle calculation.

Executed in this mission: targeted TPTests 1529 assertions / 16 cases PASS;
full TPTests 43160 assertions / 381 cases PASS; seven structural suites 75/75
PASS. Windows debug client/launcher build PASS; server target PASS (up to date).
git diff --check PASS. Read-only CK record/pair audit PASS for all ten markers.
The only build warning is the existing ignored -fPIC flag. No MASTER build/runtime
claim. Asset audit PASS is record integrity, not human pose/pathing evidence.
ESP/PSC/PEX preserved; no Papyrus recompilation required, since scripts are
unchanged. No Skyrim/CK launch, deployment, commit or push. Runtime acceptance
for this marker-based roster-2 flow is now accepted in the checkpoint above;
previous Seat01 visual attestations below apply to the superseded prototype only. ADR-0025 owns the revised contract;
CK_IMPLEMENTATION and TEST_PLAN describe manual placement and acceptance.

## Historical Seat02 geometry experiment (superseded by ADR-0025)

New maintainer runtime attestations: offline Solo rank 0 / Seat01 PASS visually;
one-member network campaign, own local rank 0 / Seat01 PASS visually; two-member
campaign A/rank 0 sits while B/rank 1 does not. The last result matches the old
intentional Seat01-only scope. These are human attestations, not new agent-run
Skyrim executions or a validation of remote seating projection.

Implemented non-MASTER extension: native local rank 1 now gets its own audited
Seat02 staging point. Seat01's geometry and the shared preparation state machine
are unchanged. Individual authoritative Applied, local finalization, durable
PlayerId/rank, session/recovery and native identity fences still precede work.
Ranks 2..9 reject. The adapter still has one MoveTo and one Activate site, with
no remote MoveTo, collective Applied barrier, new packet, ActorState write or
forced animation. Existing remote projection is untouched.

Seat02 uses the permitted right-side gap to Seat05, which is only a read-only
obstacle. The two neighboring tables and chair collision were decoded from
installed BSA NIFs; static floor/architecture collision was checked too. Point:
midpoint of the two chair X positions, 38 units behind the nearer table's
conservative Y-min, using Marker02 floor Z. In the unchanged ESP this is
(-4017.247803, -2568.495850, 0), local (+57.936035, -8.519043), facing -1.424800
radians. Conservative chair clearance is 34.936 units, table clearance >=38;
both retain >=32 after the 2-unit arrival tolerance. Model details, assumptions
and narrower Seat02 layout-alignment guard belong to CK_IMPLEMENTATION.

Executed in this mission: targeted TPTests 1485 assertions / 18 cases PASS;
full TPTests 43116 assertions / 383 cases PASS; seven structural suites 75/75
PASS. Windows debug client/launcher build PASS; server target PASS (up to date);
git diff --check PASS. ESP/PSC/PEX hashes are unchanged, and the installed ESP
matches the audited repository ESP. Seat01 geometry and the shared preparation
state machine compare unchanged against the mission baseline. A first client
compile failed on Windows' `near` macro; the helper was renamed, with subsequent
compilation successful. Only the existing ignored -fPIC warning remains.
No MASTER runtime test, Skyrim/CK launch, asset edit, deployment, commit or push.
The next acceptance is B's own client in a fresh full two-member campaign:
local rank 1, assigned Seat02, full pipeline, Enter and a visibly seated body.
Seat02 visual acceptance is pending; its visibility from A is explicitly deferred.

## Earlier local Seat01 extension evidence (2026-09-18)

Human evidence supplied by the maintainer: the approach prototype now produces
visible approach, facing and true sitting in offline Solo. The old symptom
persists in a one-player network campaign. This is a visual attestation, not a
new agent-run test or a trace identifying the original package cancellation.
Static inspection confirms that the old `intent.Solo` gate bypassed preparation
for the connected local player and reached Activate directly.

Implemented at the maintainer's request, non-MASTER only: the native local
PlayerCharacter uses the same Seat01 approach implementation in offline Solo
and a network session. Authoritative individual Applied and local finalization
remain prerequisites, with campaign/sealed durable rank, recovery, transport
PlayerId and authenticated durable PlayerId fences checked before advancing.
Only rank 0 is supported by the experiment; other ranks reject explicitly,
without being reassigned Seat01. Remote actors never enter its MoveTo adapter;
their pre-existing projection is unchanged. There is no all-Applied barrier.

The shared pipeline still reserves one MoveTo, proves real arrival, applies
facing, waits for a later UpdateEvent and issues the same single Activate per
token. Geometry, tolerances and timeout are unchanged. Connected local engine
completion now requires the same furniture-enter/root/graph evidence as Solo;
logical furniture state alone is insufficient. LocalProbe traces carry session
connection state, local-player status, durable PlayerId, rank and assigned Seat
through movement, facing, activation, Furniture Enter and passive pose samples.
Callback metadata is published immutably; callbacks do not read the intent map.

Executed for this extension: targeted TPTests 1446 assertions / 15 cases PASS;
full TPTests 43077 assertions / 380 cases PASS; seven structural suites 73/73
PASS. Windows debug client/launcher build PASS; server build target PASS (up to
date); git diff --check PASS. ESP/PSC/PEX SHA-256 values are unchanged from this
mission's baseline. Logs are under `_audit/seating-local-network-*` (local only).
The final launcher build has no new compiler warning from this extension;
the existing ignored `-fPIC` warning remains. No MASTER executable/runtime test.
Network visual acceptance is still pending: use a sealed one-member campaign
and its own local player, not a partial roster.
No new network message, ActorState write, forced animation or CK/PSC/PEX change.
No game launch, deployment, commit or push; no remote or ten-seat generalization.

## Earlier Solo seating diagnostic evidence (2026-09-18)

Earlier evidence: the hands-off 22:50:55.278 Solo activation creates run-once
FF00081F at +1 ms; it is absent at +43 ms. Through +40.013 s the actor stays
95.79255 units from Seat01, with no furniture entry/occupation or sitting graph
state. This was checked in tp_client.log; no-input is the maintainer's attestation.
The exact cancellation branch remains unisolated. The previous +30.750 s Enter
is not reproduced in the hands-off run and is not evidence of automatic pathing.

Static comparison confirms ordinary input reaches the same ActivateRef/furniture
constructor through additional picked-reference interaction code. AI_SYNC hooks
do not cancel packages in this build. Initial Marker01 is behind CommonChair01,
whose model entry mask disallows rear entry; straight front approach intersects
the table. This supports a geometry/context experiment, not a proven root cause.

Implemented at explicit maintainer request: controlled non-MASTER offline Solo
Seat01 approach, local (-88,+32) relative to the chair at Marker01 floor Z.
Audited neighboring table/profile and native bindings are checked. One MoveTo,
measured arrival (2 units), facing, then a later UpdateEvent and fresh position/
yaw check precede the unchanged single Activate. Preparation timeout is 5 s,
without retry or fallback. All approach stages use the passive native snapshots;
the full 40 s / Enter + 5 s window starts again at activation. No multi behavior,
ActorState/animation forcing, CK, network or authority change. This experiment
did not isolate the native cancellation cause. Subsequent offline visual
success is the maintainer's attestation recorded above.

Executed for the approach prototype: targeted TPTests 1424 assertions / 13 cases
PASS; full TPTests 43055 assertions / 378 cases PASS; seven structural suites
71/71 PASS. Windows debug client/launcher build PASS; server target PASS (already
up to date); git diff --check PASS. ESP/PSC/PEX hashes match the start of this
mission. The PSC/PEX already matched the validated pair at mission start, and
the installed PEX now matches it too; the previous obsolete-pair failure below
is historical. No asset edits or Papyrus compile, MASTER runtime test,
Skyrim/CK launch, commit, push or deployment during that earlier agent mission.
The later human Solo result is recorded separately above.

Earlier evidence: the instrumented 22:12:56.505 Solo request stays at occupied=0,
sitState=0 and false furniture graph variables throughout the first ten seconds.
Actor/seat distance is 95.793 initially, reaches 60.389, then 313.008 at timeout.
The matching furniture Enter arrives at 22:13:27.255 (+30.750 s). These facts were
rechecked in tp_client.log; the standing pose after late Enter is the maintainer's
visual report. Turn/move actions do not identify physical input. The prior trace
does not isolate why native package/pathing entry takes this long.

The follow-up now captures at 2 Hz through max(Activate + 40 s, first Enter + 5 s),
including after projection completion and native pending guards. First Enter can
reopen an expired capture and snapshots state synchronously at callback entry.
Animation action budgets renew each second; omissions are counted. Read-only
current/run-once package, target/procedure, raw path point, process, distance,
transforms and PlayerControls observations were added. Navmesh solver state and
physical input provenance are explicitly unavailable. Native Activate, ActorState,
movement, seating authorization and mapping are unchanged. The cause remains
unisolated pending a strict Solo run with no keyboard/mouse input for 40 seconds
minimum and until five seconds after entry. No multiplayer work or native fix.

Earlier 40-second observer checks: targeted TPTests 1392 assertions / 10 cases PASS; full TPTests
43023 assertions / 375 cases PASS. Seating structural tests 11/11 PASS; all seven
suites 69/70 PASS, with the same pre-existing obsolete PSC bootstrap failure.
Windows debug client/launcher and server build commands PASS (server already
up to date); ESP/PSC/PEX SHA-256 match the pre-mission baseline. The final diff
whitespace check passes. No Papyrus compilation, Skyrim/CK launch, deployment,
new runtime acceptance, commit or push. A first sandboxed server build could
not read Git metadata (ownership context); the owner-context rerun passed.

The maintainer's Solo runtime on 0e421a99 invalidates the seating-success
assumption below: the player remains visibly standing with furniture camera
constraints although the log says seated. Manual interaction with Seat01 works
according to the maintainer. This is a human report, not a runtime reproduced
by the agent. The reference tp_client.log records activation at 21:31:16.846,
pending-entry at 21:31:16.891, and seated at 21:31:24.846; no animation evidence
was captured. A later attempt reaches the ten-second timeout.

The local diagnostic follow-up prevents Solo from completing on furniture
identity/GetSitState alone. It separately observes matching TESFurnitureEvent
enter/exit, root/graph availability, isInFurniture/isIdleSitting, movement graph
variables, transforms, and bounded native action results. Entry-animation-observed
means those engine signals agree, **not** that a rendered seated pose is proven.
Missing/false graph reads remain pending without another activation. Individual
Applied authorization and PlayerId-to-SeatXX mapping are preserved. Multiplayer
native behavior is unchanged and its logical completion log is now explicitly
furniture-state-confirmed; no multiplayer acceptance work was performed.

Static inspection of the installed 1.6.1170 binary confirms that the vanilla
Papyrus Activate callback calls the same ActivateRef with the same arguments.
Its furniture branch already calls the native player entry/package path.
No replacement primitive, animation forcing, teleport, ActorState write or asset
change was introduced. The precise cause of the missing pose remains blocked
on the new Solo runtime observations; this is not a validated visual fix.

Earlier ten-second diagnostic checks: targeted TPTests 46 assertions / 7 cases PASS;
full TPTests 41677 assertions / 372 cases PASS; Windows debug client, server and
launcher builds PASS; seating structural suite 9/9 PASS. All seven structural
suites together: 67/68 PASS, one failure in the pre-existing working-tree PSC
  bootstrap (GetSitState/seat placement before stage 20 instead of the validated
BeginCharacterCreation helper). That PSC/PEX pair and the ESP were preserved
byte-for-byte. No Papyrus compilation, Skyrim/CK launch, deployment, new Solo
runtime acceptance, commit or push. The failed initial client compile was
corrected (GamePtr lacks const accessors); an invalid multi-target xmake command
was replaced by successful separate server/launcher builds.

At the earlier diagnostic mission, the installed Skyrim Data/scripts quest PEX hashed to
9202ac5c645f5207d74de05375d2a7e04e1d6c4816e5aa550f9cc92f386e3d16,
the obsolete working-tree pair, rather than the validated committed bootstrap.
This identifies a current installation mismatch, not proof of which script
instance ran in the earlier save or of the cause of the missing seated pose.
The next clean Solo acceptance must record matching binary/asset versions.

## Individual Applied seating implementation (2026-09-18; visual success superseded above)

Implemented individual seating projection after each player's Applied, without
waiting for another player's build (ADR-0024). Local finalization removes the
1.6-second confirmation delay, unlocks controls/stops the quest and requests
seating from game update. Solo uses Seat01. Durable roster rank selects SeatXX,
matching MarkerXX. Observers wait for a read-only final-revision/current-binding
fence on the committed remote representation.

NotifyCharacterBuildState carries a version-1 optional seating identity tail
(campaign/durable PlayerId, each <=128 bytes), populated from server admission.
Revision/inventory/spell hash checks and appearance opcodes/payloads are unchanged.
Matching updated peers are required for seating; absent/invalid identity cannot
authorize it. No new packet type, per-player quest stage or collective phase.

Native projection checks occupancy/reservations before the existing RealActivate
wrapper. The original completion criterion used furniture/seated state only;
the Solo runtime above disproves it as visual confirmation. Duplicate notifications
do not rearm; token changes permit a new projection. Missing actors/functions or
occupation conflicts remain pending; no teleport/ejection. Recovery lock suspends,
disconnect clears session intentions. Fresh canonical replay is required after
reconnection; seat-intention persistence is not claimed.

Executed on Windows debug: targeted TPTests 23 assertions / 5 cases PASS; full
TPTests 41654 assertions / 370 cases PASS; seven Python structural suites 66
checks PASS; client and server builds PASS. CK packaging 19 files PASS, strict
manifest 93 STRE records CONFORME, diff --check PASS. No Skyrim/CK launch or
deployment. Native seating, approach/animation, observer interpolation, callback
availability and runtime offset use still require in-game acceptance on 1.6.1170.

ESP unchanged from f5ee7e88. The pre-existing obsolete PSC/PEX pair was backed
up under _audit/seating-preexisting-papyrus and restored from f5ee7e88 with explicit
maintainer authorization; both restored hashes match that validated commit.
No new PSC/PEX compilation or asset change belongs to this implementation.
Valen, collective ready/departure and native lifetime remain outside this slice.

## Explicit initial creation markers (2026-09-18)

Initial placement now resolves the sealed durable PlayerId's lexical rank to ten
explicit CK XMarkerHeading references in STRE_CELL_AlternateStart. Solo selects
Marker01. The adapter resolves plugin-local IDs, validates marker/current cell
and finite position/rotation, and copies the marker transform without a chair
offset. Existing bounded MoveTo observation and posture independence remain.
Papyrus, campaign authority and final rematerialization are unchanged.
Post-creation collective seating is not implemented.

Validation on Windows debug: targeted TPTests 220 assertions / 9 cases PASS;
full TPTests 41631 assertions / 365 cases PASS; six Python structural suites
59 checks PASS; client build PASS (initial
sandbox attempt failed at Node EPERM, then succeeded outside sandbox).
Strict CK manifest: 93 expected STRE records, zero anomalies; packaging: 19
managed files PASS; MQ101 structural audit CONFORME. PSC/PEX hashes are unchanged.
The supplied ESP is preserved byte-for-byte (SHA256
3ffb7fb72d67b93518a122f080606132b21da4dde6f615822f572e891072f0b4).
Its ten added references are XMarkerHeading records in the intended cell.
Relative to the previous commit it also contains pre-existing CK payload changes
in 19 records (including TES4 and the start marker), so this is not claimed to
be a binary delta limited to ten additions. The manifest audit is not an
exhaustive behavioral validation of those existing CK changes.

No Skyrim/CK launch or deployment. Physical clearance, final facing and
Solo/two/ten-player runtime acceptance remain to be confirmed.

## Cumulative Character Creation review validation (2026-09-18)

The cumulative source at `1e9ddb02174bb0cf8113e2be923152a2f90ad51a`, compared
with `9108727b5d828afc4de2a90e1f4eece7dd811351`, was revalidated locally on
Windows during PR preparation. These are new executions, distinct from the
activation evidence below: TPTests **41574 assertions / 364 cases PASS**;
the six appearance/materialization/placement Python suites **58 checks PASS**;
debug client, server and launcher builds PASS. Six functional translation units
compiled again in isolation with `IS_MASTER=1`; this is still not a complete
linked MASTER executable or a MASTER game run.

The exact tracked Alternate Start PSC compiled in a separate audit directory
with zero errors/warnings. Its disassembled output and the tracked PEX match
after normalizing compiler metadata, declaration order and label names, while
preserving instruction order and operands. Both contain the same six functions,
including BeginCharacterCreation. Tracked PSC/PEX/ESP hashes remained unchanged.
The ESP is also unchanged across both commits relative to the base. CK packaging,
MQ101 Quickstart-5 structure and strict plugin-manifest audits PASS.

Review corrected the canonical wire reference from the obsolete schema 1 to
schema 2 / FinalBuildRevision and documented the existing server final freeze;
no functional source or asset change was needed for that correction. Historical
hot appliers remain unreachable from Character Creation. Other appearance
diagnostic hooks/traces remain compiled in MASTER; only the F11/Shift+F11 passive
lifetime controls and the extra retirement-window sample are non-MASTER.

No new game, visual, network-process integration or lifetime validation was
performed. Earlier human attestations retain their stated scope; repeated cycles,
recovery/reconnect, exhaustive appearance coverage and native retirement remain
unvalidated. CI results and PR delivery state belong to GitHub, not this entry.

## Automatic final Character Creation rematerialization (2026-09-18)

Initial Character Creation now uses the final rematerialization flow automatically,
including MASTER (ADR-0023). There is no live remote appearance sync; the matching
pending authoritative Applied build publishes one canonical final after sealing.
Observers rebuild only the local native Actor/private TESNPC representation through
the unchanged natural-join materializer, preserving logical ECS/network identity.
No user/debug activation is required. Ctrl+F11 and its functional menu toggle are
removed; F11/Shift+F11 passive probes remain non-MASTER and default OFF.

Connection, exact 1.6.1170 runtime, canonical/revision, binding, ActorState,
transaction, readiness and recovery guards are retained. Rejection fails closed
without hot apply, SwitchRace or Reset3D. Solo has no appearance transport/remote
transaction. Intermediate RaceMenu closes and later manual showracemenu do not
publish. Post-creation appearance edits and collective seating remain out of scope.
Papyrus, creation placement, network schema, roster and Discovery policy are unchanged.

Activation verification: TPTests **41574 assertions / 364 cases PASS**; all
**58 structural checks PASS**; client/server/launcher debug builds PASS. Six
functional translation units also compile in isolation with **IS_MASTER=1**,
PCH reuse disabled and the normal common headers included. This verifies the
MASTER code path, not a complete linked MASTER executable or MASTER game run.
Papyrus is untouched: PSC/PEX hashes match the verified checkpoint and the PEX
is byte-identical to its retained compiler output. CK packaging/MQ101 audits and
git diff --check PASS. Source comparisons preserve the materializer, natural spawn
callers, placement and native safety predicates; network/Discovery sources are unchanged.

**HUMAN VALIDATED:** the maintainer confirms that automatic final remote
rematerialization succeeds without debug activation on the tested multiplayer
Character Creation run (2026-09-18 commit handoff). No Ctrl+F11 was required:
normal creation, final build sealing, canonical final snapshot and automatic
natural-join rematerialization produced the correct remote appearance. Initial
creation retains no live remote appearance synchronization. This attestation does
not identify an exact race/sex pair, MASTER build or complete test matrix.

No game was launched by the agent. General production readiness, native retirement,
repeated cycles, recovery/reconnect, the full race/sex matrix and future collective
seating remain open. Post-creation appearance editing remains out of scope.
Final commit checks validate the approved source snapshot, retaining the verified
checkpoint Papyrus; unrelated working-copy Papyrus changes are excluded.

## First final-rematerialization runtime checkpoint (2026-09-18)

Character Creation live appearance sync = **superseded** (ADR-0020/0021/0022).
At this checkpoint, final local natural-join rematerialization was a non-MASTER,
default-OFF LAB (activation now superseded by ADR-0023 above).
Runtime = **human validated on the tested scenario only**.
This checkpoint does not complete Alternate Start or Character Creation (#9).

**HUMAN VALIDATED**, according to the maintainer's 2026-09-18 finalization handoff,
corroborated by the observer's tp_client.log at 16:53:56:

- The final Character Creation snapshot triggered local natural-join rematerialization.
- The same logical remote/serverId/ECS identity was retained.
- A new native Actor/private TESNPC pair was created.
- The candidate committed successfully after readiness and inventory restoration.
- Observer A saw remote B's final appearance correctly (human visual attestation).
- Character Creation finalization used no SwitchRace/Reset3D/hot-apply path;
  source contract checks also verify that bypass.

The trace has finalRevision=2, serverId=3, entityVersioned=3, session=1 and
materialization generation=1. The initial receipt log has no bound entity yet;
the retained ECS identity is verified from gate acceptance through completion.

| Representation | Actor | Private TESNPC |
| --- | --- | --- |
| Before | FF000826 | FF00081E |
| After | FF00083D | FF000839 |

The sequence is final-snapshot-received, actor-state-accepted (life=9 / dont-move),
gate-accepted, transaction-reserved, old-binding-captured, candidate-create,
inventory restoration, candidate-ready, candidate-commit, old-retirement-requested
and complete. At 16:53:56.597, old-retirement-observed reports Discovery absence
only. At 16:54:26.605 (+30 s), actorLookupPresent=true and baseLookupPresent=true:
**native retirement completion remains unresolved and inconclusive**, neither
leak proof nor destruction proof. Do not mark it validated.

No exact race/sex pair or reciprocal direction is established by this handoff;
no universal race/sex validation is inferred. Repeated cycles/native lifetime,
recovery/reconnect, the full race/sex matrix and future collective seating remain
pending. This is not production-ready or a seating validation. No new Skyrim run
was launched by the agent.

Before this checkpoint, tracked Fragment_0/4 had drifted back to a seated gate.
They now share BeginCharacterCreation: resolve the existing non-furniture marker
by plugin/local ID, validate player/cell/position and enter stage 20 without posture
checks. The PEX tracked with that PSC was rebuilt from this exact source, compared
byte-for-byte with the compiler output, and passes the bootstrap/packaging checks.
The runtime observation above does not by itself prove which PSC/PEX build the
maintainer installed; restored repository consistency is validated separately.

Final automated verification: TPTests 41575 assertions / 364 cases PASS;
client/server/launcher debug builds PASS; all 57 relevant structural checks PASS,
including the previously failing PSC bootstrap test; Papyrus compile 0 errors /
0 warnings; CK packaging and MQ101 audits PASS; git diff --check PASS.
Sources/tests/docs and the intentional tracked PEX form the checkpoint scope.
Ignored audit files, runtime logs and generated build outputs are excluded.

See the [appearance contract](../features/alternate-start/CHARACTER_APPEARANCE_SYNC.md)
and [test plan](../features/alternate-start/TEST_PLAN.md). The earlier diagnostic
entries below retain their original, narrower validation context.

## LAB native state veto: DontMove identified and admitted (2026-09-18)

Historical diagnostic entry: the native policy remains; ADR-0023 supersedes its
optional activation. Current automatic behavior and validation are recorded above.

The subsequent supplied run isolates unsafe-actor-state with flags 01200041 /
00001008. The only failed state field is life=9; knock, attack, fly, weapon,
recoil, stagger, sprint and swim are idle. Offline RE of the matching installed
1.6.1170 image proves Actor.SetDontMove(true) sets life 9: native registration
9EFCD8/9EFCE6 -> callback 9EA930 -> 6750D0 -> life setter 680740. Creation already
uses SetDontMove via SetPlayerActorLock. This is movement inhibition, not proof
of death or ragdoll. The specific writer of the remote's word is not traced.

The non-MASTER, default-off LAB now admits only this additional life value.
Named read-only predicates replace the combined opaque mask. Every other veto
and the independent dead/disabled/combat/mount guards remain. Conservative
fly/weapon/sprint/swim restrictions are identified as LAB limits, not asserted
to be inherently dangerous. No state mutation or posture condition was added.

Capture logs the tested words' decoded fields, lifeName, stateVeto and every
predicate result. State rejection retains unsafe-actor-state with its precise
substate; later old-binding/precommit/post-placement vetoes also log before abort.
For the reported pair, actor-state-accepted now reports lifeName=dont-move and
stateVeto=none. The existing gate-accepted -> transaction-reserved ->
candidate-create-enter sequence follows if all other guards pass. Native success
is NOT claimed: no Skyrim run or deployment was performed.

Validation: TPTests PASS (41575 assertions / 364 cases), debug client/server/
launcher builds PASS; structural checks 56/57 PASS with the same pre-existing
posture-independent PSC bootstrap failure, neither skipped nor weakened.
Papyrus audit-only compile: 0 errors/warnings; CK packaging and MQ101 audits PASS;
git diff --check PASS. Tests exhaust the former 14-bit combined mask and admit
only DontMove, including mixed-hazard rejection and all posture values.
Natural-join/materializer/projection, network, roster, race handling, Discovery,
placement and PSC/PEX remain unchanged. No commit, push or installation.
Policy/evidence: [appearance contract](../features/alternate-start/CHARACTER_APPEARANCE_SYNC.md#lab-actorstate-policy-161170-only).
Runtime procedure: [test plan](../features/alternate-start/TEST_PLAN.md#lab-dontmove-veto-correction---current-mission).

## Runtime placement and current-binding rejection diagnostics (2026-09-18)

The supplied new run proves two independent failures: placement resolves index 0
for a two-member roster/alias 1 in cell 080012D1 but rejects move validation; the
LAB receives serverId 3's final and rejects its current binding. The older
player-not-standing log is not evidence for this run. No new game run was launched.

Offline analysis of the matching installed 1.6.1170 binary confirms that the
PlayerCharacter branch of MoveTo (ID 56626, RVA A447F0) copies a destination into
pending game state through ID 40442 (RVA 7312B0), rather than synchronously setting
the player coordinates. Native placement now issues one MoveTo and observes real
cell/position on updates, with a five-second monotonic deadline. No fixed sleep,
repeated MoveTo, posture logic or ActorState write is added. Orientation is applied
after arrival so queued movement cannot overwrite it. Identity/anchor/cell,
campaign/PlayerId, connection, quest and recovery guards cancel a stale wait.

standing-move-observation logs target, pre-move and current coordinates, distance,
target/current rotation, cells, actor token, sample and elapsed time. The first
observation is immediate; pending logs are capped at four per second, with final
success/failure always logged. A final move-validation-failed now has an exact
detail such as position-timeout, cell-timeout or player-identity-lost.

LAB Capture now reports the first exact failed guard and the same observed
component/native values used for evaluation. Missing ECS components, assignment,
WaitingFor3D, cached binding, native/base lookup, actor safety and alias collisions
are distinct. Recovery/disconnect rejection also names the exact boundary.
At that diagnostic step, all eligibility predicates were retained without a
speculative repair. Natural-join materializer/projection, network, roster and ECS
identity stayed unchanged. The subsequent run isolated life=9; its targeted
correction and current validation are recorded above.

Validation: TPTests **8386 assertions / 361 cases PASS**; client/server/launcher
and TPTests debug builds PASS. Python **55/56 PASS**. Papyrus verification compile
(to audit output only), CK/MQ101 audits and git diff --check PASS. Scope comparison
confirms 12 intended changed files, with natural-join/network/roster/ECS and
tracked PSC/PEX unchanged. Evidence: `_audit/runtime-rejection-report.md`.
The full Python run has
one pre-existing bootstrap contract failure: the mission-start PSC already contains
the old seated Fragment_0/4. Its hash is unchanged in this mission; it is recorded
separately, not repaired as part of these two native rejection diagnostics.

## Posture-independent Character Creation and respawn LAB (2026-09-18)

Implemented locally under ADR-0022; runtime acceptance pending. Native placement
no longer reads ActorState or rejects player-not-standing. It validates durable
PlayerId/roster, existing anchor/cell, native player identity and finite position
within 32 units of the requested target before RaceMenu. The same ten anchors
and PlayerId rank mapping remain. No WantToStand wait, forced furniture exit,
activation or ActorState write is introduced.

LAB safety ignores the entire sit/sleep field and no longer rejects furniture
Interaction extra data. Capture, staging and commit retain independent actor,
cell, binding, assignment, ownership, generation, recovery and geometry guards.
The natural-join materializer/projection, transport, roster and ECS identity are
unchanged. Seating after collective completion remains a separate future phase.

At mission start, the on-disk PSC again contained the old seated Fragment_0/4.
Both now call BeginCharacterCreation: existing non-furniture marker, player/cell
validation, movement and position check, then stage 20 without GetSitState.
The tracked PEX is rebuilt; ESP/aliases/stages are unchanged. Preserve this
source/compiled pair during later CK exports.

Validation: TPTests **8379 assertions / 360 cases PASS**; **52** Python
source/model checks PASS. TPTests/client/server/launcher debug builds, Papyrus
(0 errors, 0 warnings), CK packaging/MQ101 audits and git diff --check PASS.
All 16 sit/sleep values are covered with independent safety guards retained;
position tolerance and non-finite coordinates are tested. Baseline hash review
confirms 19 intended changed/new files and no natural-join/network/roster/ECS/ESP
changes. Existing C++ warnings remain. Evidence: `_audit/posture-independent-report.md`.
No game launch, installation/deploy, commit or push. Native runtime remains untested.

## Standing placement by durable PlayerId (2026-09-18, temporary LAB unblock)

Implemented locally; no new game runtime acceptance. Standing placement now
sorts a local copy of the sealed roster's durable PlayerIds and selects the local
authenticated PlayerId's rank as creationPositionIndex. Solo uses index 0; two
players use 0/1; up to ten use 0..9. Existing anchors are selected through quest
aliases 1..10, with the same coordinate calculation and current-cell validation.

SlotId is not read or validated by this placement step: empty, duplicate or
malformed SlotId metadata cannot reject its selection. Missing/duplicate PlayerIds
or a local player absent from the sealed roster still reject. The current sealed
CharacterCreation/ACTIVE/full-roster checks remain. Admission, wire validation,
campaign SlotId semantics, ownership, recovery and the respawn/natural-join LAB
are unchanged; this is only a temporary local visual placement rule.

The source of local identity is CampaignService::GetDurablePlayerIdForAuthentication,
not the transient numeric transport player number. Success logs now include
phase=standing-position playerId=... creationPositionIndex=... rosterCount=....
Native alias/cell diagnostics remain; ADR-0022 removes posture failures;
identity failures now name PlayerId. No multiplayer identity failure falls back
to the Solo position.

Validation: TPTests **8025 assertions / 359 cases PASS**; **51** relevant Python
checks PASS. Create/Join A+B reaches the common selector after seal/authorization;
empty/duplicate/invalid SlotId metadata preserves ranks, reordered ten-player
rosters produce 0..9, and missing/duplicate/absent PlayerIds reject. TPTests,
client/server/launcher debug builds, Papyrus verification compilation (0 errors,
0 warnings), CK packaging/MQ101 audits and git diff --check PASS. Papyrus was
compiled into the audit directory; repository PSC/PEX and ESP remain unchanged.
Existing C++ warnings remain. No launch, installation/deploy, commit or push.

Earlier placement failures and the restored standing bootstrap are documented
in `_audit/standing-placement-diagnostic-fix.md`; their old SlotId selection is
superseded for this step. Current evidence: `_audit/standing-player-rank-report.md`.

## Character Creation Slice 3 natural-join correction (2026-09-18)

Historical implementation entry: its activation and absence of runtime evidence
are superseded by the automatic flow and bounded runtime checkpoint above.

EXPERIMENTAL NON-MASTER LAB IMPLEMENTED, DEFAULT OFF; NO SLICE 3 GAME RUNTIME
VALIDATION. ADR-0020 and ADR-0021 govern the final-only, race-agnostic local
representation replacement. All RaceMenu closes remain local; ONE final is
captured after the matching authoritative Applied build notification seals the
entire creation. Later manual showracemenu remains out of scope.

The LAB now reuses natural-join private materialization AND shared local
projection: explicit initial native placement, remote/player flags, actor values,
root-based WaitingFor3D readiness, inventory/equipment, factions and animation
variables. The same canonical AppearanceBuffer/ChangeFlags/FaceTints path accepts
Nord, Orc, Khajiit and Argonian; no race-family gate remains in the LAB. Descriptor
race/sex resolve and validate the created result. Headpart entries, when present,
must resolve; a nonzero headpart count is not an eligibility requirement.

The new Actor/private TESNPC starts at the old remote position/cell/worldspace,
not the observer position. Existing ECS entity/version, serverId, PlayerId,
Remote.Id, ownership and network animation/interpolation components remain.
Candidate tints stay isolated until commit, canonical inventory/equipment are
checked live, and Slice 2A generation/token fences intercept Discovery before
legacy subscribers. Retirement uses the existing engine-managed Actor Delete.
No new server lifecycle, assignment, SwitchRace, Reset3D or manual TESNPC free.

The CK stage-10 and stage-11 fragments share placement at the existing
non-furniture NewGameStartMarker, cell/position check, then stage 20. ADR-0022
supersedes the initial standing check; posture cannot reject this flow. Before
RaceMenu, native bootstrap assigns a distinct location from the immutable sealed
roster PlayerId rank (Solo: first anchor), 96 units behind one of the ten existing seat
anchors, without targeting/activating furniture. PSC and PEX are updated; ESP,
aliases and stage numbers are unchanged. These new standing positions and their
physical clearance have NOT been tested in game. Seating after collective
completion is a separate future slice, not a respawn requirement.

Automated validation: TPTests 7931 assertions / 356 cases PASS; 46 relevant Python
source/model checks PASS; TPTests, SkyrimTogetherClient, SkyrimTogetherServer and
SkyrimImmersiveLauncher debug builds PASS. Papyrus compilation: 0 errors, 0
warnings. CK packaging, MQ101 structural audits and git diff --check PASS. Existing C++ build
warnings remain. MASTER exclusion is source-checked, not a separate MASTER build.
The corrective T1-T14 coverage and human acceptance matrix are in TEST_PLAN.

Runtime limits: world-exposed candidate staging can briefly overlap the old
representation; native rendering, state/equipment and actual network silence
need human evidence. Private-base lifetime remains UNKNOWN; a 30-second registry
observation proves neither destruction nor a leak. Source proves placement
arguments, not visual spawn atomicity. Provenance remains clearable; capture uses
fresh binding evidence. Recovery preserves valid old binding and defers candidate
cleanup while locked. Tombstones survive disconnect (ten server identities per
process); restart clients between LAB runs. Historical Slice 2B observations do
not validate this corrected flow. No game launch, installation, commit or push.

Evidence: `_audit/natural-join-correction-baseline.json`, build/test logs prefixed
`natural-join-`, and
`_audit/character-creation-natural-join-rematerialization-correction.md` (A-K).
The previous Slice 3 A-Q report remains historical evidence.

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

### Final-only same-race appearance application (phase 4E)

The final-only, owner-validated appearance path applies to proven STRE-created
private player NPCs on 1.6.1170.0. Race/sex eligibility is checked before native
Deserialize; fifteen identity, provenance, race, sex and marker postconditions
remain fatal. Weight is compared and logged but is no longer fatal. The applier
neither owns nor directly writes weight and adds no weight network channel.

The user's phase 4E handoff reports phase 4D runtime evidence: all fifteen fatal
fields matched; only Weight differed (pre=50, descriptor=75, actual=50), while
headparts/body/hair color changed. Live remote weight changes during RaceMenu
are human-observed; their exact runtime propagation path remains unidentified.
This evidence supports the narrower postcondition policy, not a general native
serialization contract or proof of successful FaceGen application.

After postcheck success, one guarded QueueUpdate/DoReset3D(true) request starts
a maximum 120-tick face/head transition and tint-generation cycle. One immutable
active snapshot plus one latest follow-up is retained. New diagnostics capture
weight before reset/at transition/at applied; tint count/hash, geometry tokens,
Generated before/after Setup/Update, body colors and identity observations.
Generated completion is rejected if Update's geometry changed underneath it.

The user's phase 5A handoff reports human runtime validation of phase 4E for
Nord/Nord, same sex: full final-only pipeline reaches applied, morphs/headparts,
hair, body/head colors and weight are correct, actor remains stable and no visible
state loss occurs. Exact live weight propagation remains unidentified. This is
human-provided evidence, not an assistant-executed runtime test.

Phase 5A classification and phase 5A.1 passive SwitchRace instrumentation remain
available. The revised 5A.2 handoff supplies human-captured player RaceMenu and
loaded non-player setrace evidence on 1.6.1170.0: both pass false, change runtime
and base race before return, and rebuild 3D/head later. Player completion can
precede head readiness. These are supplied observations, not new assistant-run
runtime tests; they do not validate the remote pipeline.

The phase 5A.3 handoff supplies a real multiplayer 5A.2 failure: remote male Nord
-> High Elf, serverId3, Actor FF000815/base FF000812. SwitchRace and Deserialize
pass target runtime/base and private identity checks; body/race data converge.
The Nordic head remains black, with no race-head-ready, FaceGen, applied or
correlated completion event before the 120-tick timeout. Phase 5A.2 is NOT validated.
The earlier player/NPC probes observed surrounding engine paths, so they did not
prove that a direct remote SwitchRace alone schedules the required head rebuild.

Phase 5A.3 adds one existing QueueUpdate/DoReset3D(true) after successful target
Deserialize/postchecks in RaceChangeSameSex only. ResetRequested is one-shot and
cannot precede successful Deserialize; reset failure stops. Pre-reset root/face/head
are captured immediately before the call. Readiness requires all three nonnull and
a changed root plus a changed face or head, with separate transition logs; completion event is optional.
The 120-tick wait/FaceGen budget starts at reset. No second reset, retry, respawn or
shader fallback is added. The mapped Nord/High Elf same-sex skeleton/runtime gates,
fifteen fatal invariants, diagnostic-only weight, inventory aggregate checks and
immutable active plus latest follow-up remain. Same-race 4E is unchanged.

TPTests passes 5245 assertions in 293 cases. Tests, server, client and launcher Debug builds pass; git diff --check passes. The 5B.1 handoff reports human runtime validation of controlled male Nord -> male High Elf: correct body/head/ears/hair/tints, no black Nordic head and stable behavior. Reverse direction is not required before continuing, by user decision. This does not validate sex changes. No game deployment, commit, push, PR or issue was performed. Combined race/sex changes, beast/custom race switches, empty-tint clearing and 2 Hz publication stay disabled.
Native failure can leave a partially mutated private NPC without rollback. Geometry
and aggregate counts cannot prove rendered or per-item correctness; ownership epoch
is unavailable in existing logs. Campaign authority/recovery is unchanged.
Phase 5B.1 passive diagnostics remain available. The 5B.2 handoff supplies human
S1/S2 Player Nord evidence on 1.6.1170: sex changes first, head becomes null, race
and overlay remain stable, then DoReset3D(true) at caller RVA95499B renews root/face/head.
S1 (male->female) reset follows about5 ticks; S2 follows about2 ticks. No Player
SwitchRace invocation was observed in those windows. Male/female Nord model paths
differ (character assets/skeleton.nif vs character assets female/skeleton_female.nif).
Appearance buffer/hash, flags and descriptor sex change. These local observations
do not prove remote Deserialize behavior. Optional NPC S3 is not required by the user.

Phase 5B.2 implements an experimental dedicated SameRaceSexChange path, restricted
to mapped Skyrim.esm Nord and nonempty bounded male/female model paths (equality is
not required). Existing private/identity/runtime/loaded/unmounted/INI guards precede
one Deserialize into the same private NPC. Actual sex must equal descriptor sex;
mismatch logs deserialize-did-not-apply-sex nativeReset=false and stops without setter,
retry or reset. Success permits one existing QueueUpdate/DoReset3D(true), then up to
120 ticks for new nonnull root AND changed nonnull face/head and existing FaceGen.
Fifteen fatal postconditions include unchanged target races and target sex; weight
remains diagnostic. Logical-loss aggregate checks and latest-wins persist. SameRaceSameSex
4E and RaceChangeSameSex5A.3 retain their existing cycles; combined race+sex is rejected.
No new packet, SwitchRace sex call, direct sex-bit write or destructive fallback.
TPTests passes 5623 assertions in 301 cases. Tests/client/server/launcher Debug builds and git diff --check pass.
At completion of that phase, remote sex runtime evidence was still pending. The later
generic-domain handoff supplies human validation of remote Nord male -> Nord female
with the 5B.2 pipeline. This validates that scope only; no new assistant-run game test
or deployment is claimed.

The generic vanilla humanoid domain now admits the eight audited base Skyrim.esm
playable races through one resolved-record/model gate. The installed master was read
without modification to verify EditorIDs, relative IDs, playable flags and both gender
models. Runtime checks resolve the actual forms, require playable/nonempty bounded
models and exact 1.6.1170.0. Argonian/Khajiit, custom, vampire/transform and nonplayable
records remain excluded without a separate algorithm.
`ApplyAppearanceSnapshots` classifies all four valid race/sex transitions generically.
The original combined implementation used one SwitchRace, final Deserialize and one
reset. The supplied R2 handoff reports that it FAILED in multiplayer: final race/sex,
headparts/hash and colors matched A, but head remained null through120 ticks; FaceGen
and Applied were never reached. Correct feminine body alone is not acceptance.
SameRaceSexChange no longer has a Nord gate; race-only no longer has a pair/direction
or matching-skeleton gate. 4E readiness, 120-tick budgets, nonfatal weight, provenance
and latest-wins behavior remain. No live publisher or packet/spawn change is introduced.
Pure sequence tests cover both editing orders and nonhistorical race pairs; these prove
classification/state-machine closure, not general native rendering convergence.
TPTests passes 5956 assertions in 309 cases. Tests/client/server/launcher Debug builds
and git diff --check passed for that prior build. R1 HighElf sex-only and R3 Breton
female -> Orc female remain unvalidated. R2 now has the supplied failure evidence.
Live publication stays disabled pending that validation. No deployment/Git publication.

The combined-only adapter now implements an experimental two-phase native cycle:
SwitchRace -> target race/source-sex checks -> reset1 without Deserialize -> strict
race/source-sex geometry -> final Deserialize -> target race/target-sex checks -> reset2
-> strict final geometry -> one final tint cycle. Phase1 failure never applies final bytes.
Each phase has a120-tick wait budget, phase2 including FaceGen. Active/Latest spans both
phases; native reentry is guarded. One SwitchRace, one Deserialize, at most two resets
(combined only), one final Setup; Update can poll material/Generated within that cycle.
The three existing noncombined pipelines, eight-race domain, transport, capture/spawn,
nonfatal weight and live-disabled behavior are preserved.
The handoff supplies A's chronology: target race/source sex after SwitchRace, race reset
about15 service ticks later; sex change seconds later, sex reset about13 ticks after it.
These observations justify the experiment, not copied delays or proof that reset1 without
Deserialize can prepare the remote head. No new remote runtime success is claimed.
TPTests passes7112 assertions in319 cases. Tests/client/server/launcher Debug builds and git diff --check pass.
After review, retry R2 first in CharacterCreation RaceMenu on fresh A/B. If phase1 fails,
retain logs and investigate native preparation separately; no additional workaround.
Resume R1/R3 only after R2 is validated. No post-creation showracemenu acceptance while
live publication is disabled. No deployment or Git publication performed.

The R2 provenance handoff reports two identical visual reproductions (feminine target
body, stale Nordic/male head) but no pertinent hot/combined markers in the supplied B
log after the two-phase build. The prior executable hash CDA455456896ACCE17A371A9E800C31BB8FBEC04C1ECAD6055BFFDE329EFA734
was verified locally in both build and distrib before linking this diagnostic build.
The loaded-process identity is human-supplied evidence; no new process inspection or
game execution is claimed. This contradiction does not prove that the combined path ran.
A diagnostics-only AppearanceTrace now records every final publisher attempt/rejection,
pending/send outcome, server receipt/owner result/actual selected recipient, receiver
entry/queue/ignore, creation/recreation/assignment paths and detailed combined guards.
Remote-player-only fingerprints record changed identity/appearance/geometry fields by
serverId across service ticks and known creation/combined observations. Last valid
identity is retained across missing-actor samples until disconnect. Native hooks add
local/remote/NPC category and base pointer without changing their native calls.
No reset/order/domain/transport/wire/capture semantics or publisher activation changed.
TPTests passes7145 assertions in321 cases. Tests/server/client/launcher Debug builds and git diff --check pass.
One clean R2 after review must establish the real mutation path; no final appearance
fix or new runtime success is claimed. Unknown engine changes can be localized to an
observation interval; temporal proximity alone cannot identify an uninstrumented writer.

The subsequent 2026-09-08 service-barrier handoff resolves that diagnostic uncertainty
for R2 on A observing B/serverId3: notify/entity/queue/classification/combined begin
occur at17:03:40.189-.191 with all preguards passing. Actor FF000835/pointer8F5FD3F0
and Base FF000833/pointer8F5BEF40 stay stable: IN_PLACE_MUTATION, not recreation.
SwitchRace(HighElf,false) followed by reset1 without Deserialize yields renewed,
nonnull race/source-male geometry at17:03:40.215. That remote phase is human-proven
for this case. The final female Deserialize previously ran immediately in the same
callback, followed by reset2; head stayed null for120 ticks and stopped at17:03:42.258,
without FaceGen. Native payload correctness did not validate the stale visible head.
These are supplied handoff observations, not a new assistant-run game test.

The combined-only adapter now returns immediately after observing RaceStageReady,
then resumes in a later CharacterService update before the waiting-state filter.
A saved existing service update identity rejects same-update reentry; no new timer,
arbitrary delay, native state, third reset or barrier between final Deserialize/reset2.
Resume rechecks Active, identity/private provenance, cached/server IDs, remote/player
markers, target race/SOURCE sex, inventory/equipped totals, WaitingFor3D=false and strict
race geometry before any final write. Existing final target-sex checks and120-tick
budgets remain. SameRaceSameSex4E, race-only5A.3, sex-only5B.2, domain/transport/spawn,
nonfatal weight and live-disabled behavior retain their prior source unchanged.
TPTests passes7380 assertions in323 cases for the barrier, immutable Active/Latest,
post-barrier invariant/geometry loss and one-shot bounds. TPTests/server/client/launcher
Debug builds and git diff --check pass. The immediate fallthrough was confirmed in source; its causal
role in the missing final head remains a hypothesis until one reviewed R2 runtime.
No deployment or Git publication. If reset2 still leaves head null, stop without fallback.


The subsequent native-sex-rebuild handoff reports a failed R2 despite the real service
barrier: ready at17:36:50.224/update4649, resume .240/update4650, final head still0 after
reset2 through120 ticks. Local sex rebuild later succeeds after14 observed creation
updates. This validates the barrier's separation, not the final appearance; no arbitrary
delay follows from that observation. Existing5B.2 same-race sex success is retained.

Read-only RE of the installed1.6.1170 image now establishes that return0x95499B follows
CALL0x954996 into40255/DoReset3D from unnamed52391 at0x954960. It uses the global player,
marks its NPC0x800, then after reset iterates animation graphs and sets byte+0x242
(meaning unknown). RaceSexMenu vtable callers ProcessMessage/AdvanceMovie precede it
with menu-specific preparation52409; AdvanceMovie also has conditional state gates.
Return0x40B115CE matches the STRE ThisCall trampoline in the launcher, not a second
native Skyrim reset implementation. The loader's shared image/path alias explains why
callerModule may still say SkyrimSE.exe. Both paths reach the same native reset.
A complete Actor-generic equivalent of the menu preparation remains unproven;52391
cannot target a remote argument. No native code, barrier or probes changed. TPTests
passes7380 assertions/323cases; TPTests/client builds and git diff --check pass.
No game test, deployment, commit or push. Next work is targeted diagnostic comparison
of native branch/process/biped state, not a speculative appearance correction.

Passive sex-rebuild differential instrumentation is now implemented for1.6.1170:
the existing40255 hook records entry/return state; narrow36947 and39395 probes provide
positive queued-op1C/inline-AIProcess evidence correlated by thread, actor and callSeq.
The52391 entry/return probe brackets local NPC change flags, with after-reset emitted
by40255 (no mid-function hook). Globals, validated biped operands, INI, base change
flags, process fields and creation-probe sex timing are read only; unknown values
carry availability markers. Diagnostic server IDs use a bounded service-fed registry.
No appearance application, reset count, timeout, barrier, transport or weight behavior
changed. TPTests passes7395 assertions/325cases and the client build passes. These are
helper/build checks, not engine validation. New probe activation and all differential
A/B values remain runtime-pending. The next reviewed run observes local RaceMenu and
remote combined together; do not request/repeat5B.2 control C before analyzing A/B.
No fix, deployment, commit or push is included.

The subsequent user-supplied differential control now reports a fresh successful remote
Nord male->Nord female5B.2 head/body and failed Nord male->HighElf female combined.
Both reset entries have baseChangeFlags0, INI0, process level0/byte311=3B and non-global
biped selection. These values alone therefore do not explain the failure. The supplied
SwitchRace brackets show ActorState41/1088 ->0/1008; raw new-run logs were not independently
replayed in this RE mission. This supersedes the earlier pending-control planning above.

Exact-image RE identifies ActorState fields at Actor+C8/+CC (not Actor flags+E8/+204).
SwitchRace37925 calls38989 at69AF29 with Actor+C0: flags1=0 and
flags2=(flags2 & FFFF9008)|1008. This exactly explains the observed change and matches
constructor defaults. The candidate bits belong to movement and the multi-bit weapon
state, not three independent geometry-ready indicators. Ordinary movement/weapon update
paths can set them; no safe native head-readiness restoration primitive is established.
DoReset3D has no direct test of these masks. Its AIProcess helper tests movement400
and can clear low14 movement bits; inspected load/face/biped paths do not establish
a requirement for41/80. Indirect dependency through unexpanded calls remains unknown,
so no blanket causal exclusion or repair is claimed. No C++/probe changes were needed.
TPTests remains7395 assertions/325cases; tests/client builds, raw-byte verification and
git diff --check pass. No runtime launch/deployment/commit/push. A separately reviewed
same-race HighElf male->female control is proposed to separate target-race effects from
SwitchRace/overlay history; it is not performed or required as an immediate retry here.

Controlled local rematerialization audit (2026-09-17): the supplied human comparison
reports correct HighElf head after fresh STR TESNPC/Actor creation, with the same
serverId and changed Actor/Base pointers, while the matching hot payload left head
null. Raw new-run logs were not independently replayed. This supports a candidate
architecture, not a validated transactional replacement. The current priority is
now this lifecycle audit rather than the preceding proposed HighElf sex control.

Source inspection supports keeping the same ECS entity/serverId/player identity
and reusing a shared private TESNPC + FaceGen + Actor creation path for race changes.
Functional rematerialization is NOT implemented or enabled. Activation remains
blocked on staged/retiring discovery handling, private-base/handle lifetime,
current inventory/equipment and other state reconciliation, seated furniture
behavior, and a proven remote Character Creation eligibility gate. Actor::Create
already spawns into the world; current deletion and deferred creation are not a
transactional replacement API. Same-race hot behavior is unchanged. Existing
race-changing hot branches remain present and are not considered robust by the
latest supplied observations; this audit does not reroute them.

The candidate contract and future tests are in the appearance contract/test plan;
the local detailed source audit is
`_audit/character-appearance-controlled-rematerialization-audit.md`.
TPTests passes 7395 assertions in 325 cases; TPTests/client/server/launcher builds
and git diff --check pass. Only audit/documentation changed in this mission; no
functional source change, native test, deployment or Git publication.

Slice 1 shared private remote materializer extraction is IMPLEMENTED (2026-09-17).
OnCharacterSpawn and CreateCharacterForEntity now call one file-local creation
helper only for custom private players. It preserves TESNPC::Create -> FaceGen
Setup -> Actor::Create, existing trace order and temporary GamePtr release. The
callers still bind provenance and own all network/ECS and post-creation setup.
Existing-NPC, placed-actor and non-player creation paths retain their behavior.
FaceGen still mutates the supplied entity and empty tints still do not clear it.

NOT IMPLEMENTED: controlled rematerialization, transaction, seating restore or
native retirement safety. No AppearanceApply routing changed and the prior audit's
activation blockers remain. Seven source-contract checks pass; TPTests passes
7395 assertions in 325 cases. TPTests/client/server/launcher builds and git diff
--check pass. No Skyrim run, runtime acceptance, deployment or Git publication.
The local review is `_audit/character-appearance-materializer-extraction-report.md`.

Slice 2A lifecycle evidence and a dormant pure contract model are IMPLEMENTED
(2026-09-17). RemoteMaterializationLifecycle is consumed only by TPTests; production
client/server/encoding and Slice 1 remain unchanged. The model tests same-identity
candidate routing, full session/entity/server/generation fences, abort isolation,
authoritative invalidation and one-shot retirement intents. It has no native
Completed state: discovery absence does not prove Actor or private-base release.

Source inspection establishes that ActorAdded/Removed carry only FormID, removal
means loss from the loaded/high-process/root scan, and ActorValueService is another
removal subscriber. A future router must preserve event-time correlation across
FormID reuse and protect all affected subscribers. Native pre-discovery registration,
Delete completion and private TESNPC lifetime remain UNKNOWN. The existing destructor
hook definition is not installed as a completion observer in inspected source.

NOT IMPLEMENTED: runtime transaction/routing, native candidate staging, retirement
safety, candidate-local FaceGen, state replay or seating restoration. No replacement
LAB is approved by the evidence. Recommended Slice 2B is a narrowly scoped passive
lifetime/correlation observation after verified ABI/observation boundaries; no probe
was installed or game launched here. Details are in
`_audit/character-appearance-rematerialization-lifecycle-audit.md`.
TPTests passes 7645 assertions in 337 cases; two dormant-model source checks and
Slice 1's seven source checks pass. Required TPTests/client/server/launcher builds
and git diff --check pass. No deployment, commit, push or PR.


Slice 2B passive native lifetime instrumentation is IMPLEMENTED, DEFAULT OFF
(2026-09-18). One private remote player is recorded: next natural creation via
F11, or the unique currently bound remote via Shift+F11 (both non-MASTER).
The existing Debuggers menu exposes both diagnostic controls. No candidate,
replacement, extra spawn/Delete, network write or retained GamePtr is added.
The one-pair recorder correlates packed ECS entity/version, serverId, FormIDs,
address tokens, the existing Spawn handle and a process-local probe session.
Fresh lookups are throttled to 100 ms, ending at 180 s after observation starts or 30 s
after the first removal signal, whichever occurs first. Timeout is NOT OBSERVED
WITHIN WINDOW, never proof of a leak or completed native retirement.

Static 1.6.1170 RE identifies Character and TESNPC scalar deleting destructors
(AE40288 and AE24888), verifies complete-object/flags/return ABI, and traces
TESForm registry removal. Diagnostic detours are attempted only on explicit
enable, with runtime/address/prologue checks. Actual hook entry has NOT been
observed. Source boundaries record ordinary creation/Delete and unchanged
Discovery dispatch. GetByHandle's temporary reference is released inside the
existing helper before the recorder compares its returned address token.
No later native pointer dereference occurs; destructor entry checks FormID only
through its valid live argument before forwarding. No read follows destruction.

NOT VALIDATED: natural R1-R4 timelines, destructor hook operation in game,
Discovery ordering relative to Actor::Create return, private TESNPC release,
bounded retirement across repetitions, or a terminal replacement fence.
SetBaseForm and reference cleanup inspection does not prove private-base
ownership/release. Discovery loss, form absence, handle invalidation and destructor
entry/return remain distinct facts. Verdict: NO-GO replacement LAB.

TPTests passes 7677 assertions in 343 cases (six new window tests). Nine new
passive-probe source checks plus the prior seven/two slice checks pass.
TPTests/client/server/launcher builds and git diff --check pass. Runtime scenarios
require human execution; no game launch, deployment, commit, push or PR occurred.
The local static evidence and pending-runtime report is
`_audit/character-appearance-native-lifetime-observation-report.md`.

Slice 2B keyboard access is IMPLEMENTED (2026-09-18): non-MASTER F11 toggles
the exact same setter/state as the checkbox, with foreground gating and one
toggle per sampled key-down edge. Menu visibility and mouse input are not
required. Probe implementation, logs and native/network behavior are unchanged.
The added structural check covers shared logic and MASTER exclusion; ten probe
checks plus seven/two prior slice checks pass. TPTests still passes 7677 assertions
in 343 cases. Client/server/launcher builds and git diff --check pass.
In-game keyboard acceptance remains pending; no deployment or game launch.

Slice 2B observe-current mode is IMPLEMENTED (2026-09-18), non-MASTER only.
Shift+F11 / the new Debuggers action submit one selection attempt on the next
game update. Zero/multiple remote players, contradictory private provenance,
incomplete/aliased ECS binding, inconsistent native Actor/Base binding,
unsupported runtime or an already-enabled
probe reject the request without arming or waiting for a future creation.
Successful selection captures packed entity/version, serverId, private FormIDs
and comparison-only tokens, then uses the SAME Record, polling, destructor
observers and observation-window policy. The 180 s clock starts at selection;
the first removal still bounds it to 30 s. Existing published handles can be
resolved temporarily; no GetHandle, retained GamePtr or synthetic creation log.
Root/discovery history not observed remains unknown. F11 disables either mode;
the next-private mode remains available for R1. No native mutation/network change.
Current-binding fallback is IMPLEMENTED (2026-09-18), passive/non-MASTER only.
The supplied installed-client log confirms that recovery lock clears appearance
provenance through ApplyAppearanceSnapshots, even after REMOTE_RECREATED, while
the native Actor/Base can remain bound. This is the existing intentional appearance
invalidation contract, not evidence of native deletion. It is left unchanged.
Current selection now permits an absent marker only with exactly one Remote+Player,
no Local/WaitingForAssignment/WaitingFor3D, matching FormId/CachedRefId, fresh
temporary native Actor/TESNPC identities, native remote-player flags, and no known
ECS actor/server/cache alias or shared bound base. Present contradictory provenance
still rejects. The evidence log distinguishes durable-provenance (present matching
allocation marker, NOT a promise of durability) from validated-current-binding-fallback.
No allocation history, exclusive native ownership or materialization generation is
inferred; root/face/head presence is not an identity prerequisite for passive capture.

This reveals a REAL BLOCKER for future controlled-rematerialization runtime wiring
that assumes RemotePlayerAppearanceBaseComponent is a durable materialization
registry across recovery. Earlier audit rebind proposals did not establish that
guarantee. Slice 2A's separate pure lifecycle keys/model remain dormant and valid;
their runtime identity source/retention/invalidation contract is still unimplemented.
The diagnostic fallback does not resolve that blocker or authorize a replacement LAB.
Production appearance/lifecycle/network sources retain their pre-task hashes.
Fourteen probe structural checks plus seven/two prior slice checks pass; TPTests
passes 7703 assertions in 346 cases. TPTests/client/server/launcher builds and
git diff --check pass. The earlier current-selection rejection was observed in the
18 September log; fallback acceptance and natural R1-R4 retirement remain NOT VALIDATED.
No deployment or game launch. The focused evidence is
`_audit/character-appearance-current-binding-fallback-report.md`.

See [the appearance contract](../features/alternate-start/CHARACTER_APPEARANCE_SYNC.md).

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
  standalone T+4 projection path, with a local stage-10 entry and
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
- the automatic Helgen increment (7 October 2026) connects validated Character
  Build Applied to the existing server start latch for the exact sealed ACTIVE
  roster. Individual seating remains immediate. Client authorization dispatches
  a local event; after local finalization, investigation stage 10 starts the
  cleanup quest, then obtains its `STRE_HelgenContinuityController` and calls
  `EnsurePostHelgenProjection()` before initializing T0 once. This running owner
  holds projection idempotence, the sole MQ101 sequence, cleanup 10/20/30,
  physical projection and cleanup 40. AlternateStart stage 30/Fragment_6 is only
  a compatibility adapter; the normal path never obtains/calls its generated
  script. Stage 0 has no fragment, and the stopped quest is never restarted by
  this adapter (startup stage 10 would re-enter creation). The existing VMAD
  attachment suffices: no ESP change or fourth quest;
- **RUNTIME FAIL (maintainer Solo smoke, 7 October 2026):** the previous candidate
  left MQ101 at 0, InvestigationState at 0 and InvestigationStartGameTime at -1.0;
  HadvarState/RalofState were both 1. After `coc HelgenExterior`, Helgen remained
  vanilla pre-attack with starting NPCs. No manual `setstage` was used. Projection
  did not execute and T0 was correctly withheld. Exact deployed artifact hashes
  were not supplied. This diagnostic FAIL remains recorded; the replacement
  cleanup-owner boundary still requires a fresh Solo smoke. The later integrated
  roster-2 partial PASS is recorded above and does not validate Solo;
- duplicate Applied acknowledgements no longer reset level or repeat the start
  mutation. Native/Papyrus latches, admission/recovery gates and the saved campaign
  flag fence duplicates/disconnects. Existing T+4 spatial and survivor rules are
  retained. Readiness reconstructs only full-roster checkpoint sessions with no
  volatile builds, never pending/mixed evidence. No durable build/Helgen state,
  phase change, host authority, new protocol or MQ101 C++ projection was added;
- targeted lifecycle corrections clear the native campaign-required latch on
  Main Menu/EndRuntimeSession (also after admission was lost) and accepted Leave,
  while preserving it across a simple disconnect. Saved Papyrus campaign fencing
  and successful readmission remain intact. Cleanup Start failure no longer
  claims the projection latch. In the corrective runtime candidate, both entry
  adapters finish latent `Start()` before casting; the cleanup owner's guard runs
  afterward. Missing MQ101 also remains retryable before the latch is claimed;
  a partial mutation still forbids replay. The installed Bethesda `Quest.psc`
  documents that `Start()` waits for startup and returns its success. Two C++
  lifecycle cases and updated structural tests preserve these boundaries;
- automated verification for this increment: Debug TPTests passes 424 cases /
  45,240 assertions (Helgen filter: 10 cases / 182 assertions), all 118 Python
  structural tests pass, and the six CK audits (packaging, strict record manifest,
  MQ101 structure/generated fragments, build catalog and ten creation markers)
  pass. The corrective AlternateStart QF, continuity and investigation PSCs
  compile with 0 errors / 0 warnings using the installed SKSE/Bethesda imports;
  each packaged PEX has the same SHA256 as its fresh compiler output. Structural
  tests verify startup-before-cast, the existing cleanup VMAD attachment, sole
  MQ101 owner/exact order, no-replay/completion guards and T0 after success.
  Fragment_0, Fragment_4, BeginCharacterCreation and the physical projection
  implementation are unchanged by this correction. Debug client, server and
  SkyrimImmersiveLauncher builds pass with `-j 2`; `git diff --check` passes.
  Windows secure-store tests and compilers require access outside the sandbox.
  Skyrim/CK were not launched and no deployment was performed;
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
- the first physical v1 headquarters checkpoint is implemented and runtime-
  smoke-tested in `STRE_CELL_AlternateStart`, now player-facing as
  `Ilinaltaâ€™s Vigil`: the main inn shell and circulation space, exterior
  placement by Lake Ilinalta, the working interior/Tamriel load-door pair,
  tavern music, the initial warm lighting pass, the STRE-owned fireplace/light
  records, and ten stable STRE starting-seat references are present;
- the current Ilinalta's Vigil geometry has an implemented interior navmesh; a
  temporary vanilla NPC successfully runtime-tested normal circulation,
  obstacle avoidance, stairs, and passages before the test reference was
  removed;
- a Messire Valen full-body prototype is integrated into the headquarters with
  versioned NIF/DDS assets; it uses a prototype Skyrim-skeleton rig, hides the
  overlapping vanilla geometry through its biped-slot setup, and was validated
  in game for locomotion and general animations;
- the headquarters exterior spans Tamriel cells `(-9, -16)` and `(-9, -17)`;
  the xEdit audit removed unintended overrides, while two vanilla rocks and the
  nearby two-reference forest-predator encounter are intentionally disabled to
  keep the headquarters footprint clear without deleting master references;
- the explicit CK import for the navmesh checkpoint changed only
  `GameFiles/Skyrim/STRE_AlternateStart.esp`, so no expected `NAVM` entry was
  added; after the Valen checkpoint, the strict CK manifest covers 83 expected
  STRE-owned records and the audit is green with no new unexpected master
  override;
- `build-and-deploy-dev.ps1` completed successfully, and the post-deployment
  runtime smoke test passed entry, normal traversal, the interior/exterior
  load-door transition, stairs and passages, collision and pathing while
  preserving the existing fireplace and lighting presentation.

The current catalog uses `BuildVersion = 5`.

### Limitations

- the New Game bootstrap and MQ101/post-Helgen world-state projection are
  implemented, but the neutral MQ102/MQ103 vanilla main-quest handoff remains
  unfinished;
- automatic post-creation Helgen entry's previous Solo candidate is runtime FAIL
  (7 October 2026). The replacement cleanup-controller owner is implemented and
  automated/build-tested, with the integrated two-PC flow partially accepted on
  d9501d57 (dated checkpoint above). Full Solo and the listed recovery/negative
  cases remain pending. Valen does not yet start it;
- the multiplayer T+4 vertical slice and its final occupied projection are
  runtime-validated in a multiplayer campaign, but the complete permutation
  matrix remains pending. The d9501d57 all-outside case passes; occupied-at-deadline
  Pending, inverse roles, HelgenKeep01, disconnect, mixed survivor states,
  save/load and cell reset are not inferred from that result;
- the new normal entry opens after the last exact-roster validated Applied.
  Numeric local T0 equality and the actual cross-client projection latency have
  not been measured; STR does not rebase local GameDaysPassed. Checkpoint-only
  readiness reconstruction is retained, but pre-checkpoint creation/build state
  cannot be reconstructed after server restart;
- coordinated checkpoint creation is implemented and automated/build-tested;
  its nominal sealed-roster Candidate/ACK/commit path is runtime-validated with
  two real Skyrim clients. Issue #72 is complete: deterministic ordering,
  replay, persistence, and transactional evidence covers the narrow partial-ACK
  races, and a real abrupt server interruption after a fresh committed
  checkpoint preserved the exact `LastCommittedCheckpoint` across restart.
  The millisecond mid-ACK disconnect, first-ACK packet-loss, and pre-commit
  force-kill races were not manually reproduced and are not claimed as live;
  `RECOVERY_LOCK` restore is implemented, automated/build-tested, and live
  validated by #56 with two clients across nominal, successive, and
  restart-rehydrated recovery. Helgen
  deliberately adds no parallel reconnect/persistence mechanism and fences its
  local progression after a campaign disconnect;
- rescue/liberation and physical `Freed`/`Departed` projections remain
  unimplemented; mixed-state and save/load/cell-reset regressions for the new
  occupation flow are still required;
- the physical Ilinalta's Vigil checkpoint completes neither headquarters issue
  #23 nor room issue #24: the architecture remains provisional, decoration is a
  minimal and incomplete first pass, and the ten player-room spaces are empty
  and doorless rather than usable rooms;
- the exterior stair/access path still has to reach the road, and the exterior
  still needs a Skyrim-appropriate sign or signpost for Ilinalta's Vigil;
- Room Bounds and Portals were deliberately skipped because profiling and
  runtime validation have not demonstrated a concrete visibility or
  performance need. They are conditional optimizations, while acceptable
  runtime performance and the remaining Valen, ready/departure, housing, and
  ten-player validation work are still required;
- the fireplace uses the separately installed EEK Vanilla Textured resource,
  without EEK/Embers HD redistribution or a direct Embers HD dependency. The two
  carving textures retain the accepted alpha provenance limitation documented
  above; tagged-package clean-install texture completeness passed for the exact
  published `0.4.0-alpha.1` ZIP, without resolving those textures' provenance;
- Messire Valen remains a full-body prototype: the head and body are still one
  mesh rather than a production FaceGen/dialogue head, finger weighting is
  imperfect, the material/shader pass is provisional, and the temporary
  Sandbox package changes furniture too often; final AI, dialogue, scene, and
  narrative-departure work is not complete;
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
path and the meaningful post-commit server-restart boundary are live validated.
Issue #72 is complete through that live evidence plus deterministic
partial-Candidate, exact replay/no-overwrite, restart, and commit-order coverage;
the narrow non-deterministic packet/timing races remain explicitly non-live.
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
remains explicitly untrusted as completion proof. Issue #72 completed the
resilience evidence with deterministic failure/disconnect, exact no-overwrite
replay, persistence, and ordering coverage plus a live abrupt post-commit server
restart. The exact lost-first-ACK and millisecond pre-commit races were not
manually reproduced.
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

Issue #72 is complete. Automated evidence proves a failed/disconnected partial
Candidate cannot replace the previous commit, exact replay selects and hashes
the existing artifact without a new native save or overwrite, and both
checkpoint/disconnect orderings preserve the correct rollback point. A real
force-kill after a new commit preserved that exact checkpoint and both slot
artifacts across server restart. The mid-ACK disconnect, first-ACK packet-loss,
and pre-commit force-kill races were not manually reproduced live. See
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
