# Compatibility matrix

> **Status: Source of truth for known compatibility**
> **Last updated: October 22, 2026**

## Current development target

Steam **1.7.104.0 only**, by maintainer decision. No future 1.6 support is
maintained. The migration branch is separate from main pending validation.
Target selection is not evidence of gameplay compatibility; [STATUS](../project/STATUS.md)
owns executed results. All older gameplay observations below are historical
1.6 evidence unless explicitly dated and identified as 1.7.

## Historical 0.4.0-alpha.1 reference platform

| Component | Observed version/support | State |
|---|---|---|
| Skyrim SE/AE runtime | `1.6.1170` Steam | historical `0.4.0-alpha.1` target; no longer the development target |
| Operating system | Windows x64 | playable client/server target; Linux CI builds shared/server code but does not establish Linux client support |
| Skyrim Together Reborn upstream | historical baseline in `UPSTREAM.md` | fork integrated into STRE |
| STRE | `0.4.0-alpha.1` | reciprocal final appearance and individual local/remote seating validated for roster 2 in both completion orders; tagged-package clean install pending |
| Angular | 16.x | in use |
| xmake | `3.0.9` in CI (`>= 3.0.0` project) | Windows and Linux CI build toolchain |
| Creation Kit | environment compatible with `1.6.1170` | historical tool qualification; 1.7 requalification required before authoring |
| SKSE64 | `2.2.6` for Skyrim `1.6.1170` | alpha installation target |
| Better Grabbing | `1.17` | external plugin required by default for multiplayer World Sync manipulation |
| Address Library | all-in-one package containing database `11` for runtime `1.6.1170` | required external dependency; runtime base for STRE/Better Grabbing |

These are the **`0.4.0-alpha.1` release targets**. Other Skyrim runtimes,
stores, operating systems, SKSE/Address Library combinations, or dependency
versions are not part of this release's supported matrix.

## Steam 1.7.104 migration

This is the sole development target, separate from the historical release above.
It does not inherit historical 1.6 validations or claim full gameplay compatibility.
See [STATUS](../project/STATUS.md) for executed evidence and the
[experimental runbook](../development/EXPERIMENTAL_RUNTIME_1_7.md) for the authorized local DEV procedure.

| Component/domain | Experimental boundary |
|---|---|
| Runtime | Exactly Steam `1.7.104.0`; 1.7.99, 1.6, other builds, GOG/EOS/unknown distributions rejected |
| Bootstrap | All 18 static native prerequisites qualified in lot 3; derived admission open for the exact inspected material; local DEV vanilla-menu startup PASS2026-10-21 (see STATUS) |
| Experimental deployment | Maintainer-operated two-PC workflow has a retained local A/B `INSTALLED_VERIFIED` receipt; candidate and evidence limits are in STATUS. No fresh destination audit or Codex access to Pc-vero |
| Address Library | v13 / exact `versionlib-1-7-104-0.bin`; formats 2/5 parse, but format coexistence is not runtime coexistence |
| SKSE | Official `2.3.1`, DLL `skse64_1_7_104.dll`, file resource `0.2.3.1`; loading/startup hooks do not attest initialized plugins |
| Better Grabbing | Official 2.0: main-PC DLL/provenance, actual SKSE 2.3.1 loading and human Solo smoke PASS; V5 declaration present; multiplayer manipulation/interruption remains untested; required native-plugin policy retained |
| Vanilla main menu | Official launcher, visible entries, keyboard/mouse and normal Quit PASS2026-10-21; maintainer visual/input evidence plus logs/exit0 |
| STRE intro/custom menu | Exact 1.7.104 adapters ported; offline builds/tests PASS and maintainer-attested human PASS for the progressive A-D presentation trial. Deployed candidate and exact scope in STATUS; no inferred controller, resolution/language matrix, per-PC runtime verdict or exhaustive failure coverage |
| Solo gameplay | Bounded maintainer-run New Game/creation, local Thief build, seating, Valen access, quick-save/reload, Continue and checked-character persistence; exact human/technical scope in STATUS; no exhaustive appearance/class/dialogue compatibility claim |
| Reported two-player path | Maintainer-attested PASS: admission/lobby, creation, reciprocal appearances, seating, movement and saving/loading. Logs checked by the maintainer, not Codex; exact candidate and accepted scope are in STATUS |
| World Sync and collective recovery | Not qualified by the reported two-player path. No inferred manipulation/interruption, crash/disconnect/restart or coordinated-checkpoint recovery PASS; presentation has its separate exact-target gate, while unrelated optional dialogue/diagnostic paths remain guarded |
| CK/Papyrus/content | Exact 1.7.104 wrapper/protection tests PASS; real AutoMod GLOB and two Papyrus fixture compilations PASS. Maintainer-attested xEdit 4.1.5f fixture cycle PASS, with bounded log corroboration. CK 1.6.1378.1 human PASS only for startup, Skyrim.esm load without active file, TimeScale inspection and normal exit without saving; reported non-blocking MASTERFILE warning retained. CK fixture creation/write/save/reopen: **NON TESTÉ / dispensé par décision du mainteneur**, not an automatic migration-PR prerequisite. Production authoring/import remain unqualified. Inventory/hash continuity observed, archive/source provenance incomplete. See STATUS and the [toolchain evidence](../audit/2026-10-22-runtime-1.7-toolchain.md). DEV build/deployment remains separate from authoring |
| Campaigns | New isolated test campaign only after capability prerequisites; no 1.6 campaign/save migration guarantee |
| Linux/CI | No Linux execution or manually triggered CI in this mission |

## Fireplace dependency and evidence

EEKs Fireplace Resource from [EEKs Resource Repository](https://www.nexusmods.com/skyrimspecialedition/mods/31562?tab=files)
(EvilEyedKyo / EEK) remains a separately installed external prerequisite. The
selected Vanilla Textured model has no direct Embers HD dependency. STRE
redistributes neither EEK nor Embers HD assets.

Maintainer runtime smoke **PASS**, 2026-09-19, in the development environment:
no purple/missing textures; carvings, embers and wood correct; size/origin
visually unchanged; collision unchanged; circulation unaffected. Exact tested
ESP SHA256: `299dafea9b6a7fa272b2319daa5c66866194cd6b1963064f57145e8294d5d9de`.

The exact original provenance of `textures\eeks whiterun interiors\smim\wrcastlecarvings.dds`
and `wrcastlecarvings_n.dds` has not been independently established. Their runtime
use was accepted by the maintainer for `0.4.0-alpha.1` after successful visual
validation. STRE does not redistribute these files; this acceptance is not
evidence of standalone redistribution permission. A tagged-package clean-install
smoke must still verify availability of all external textures.

## Character Creation checkpoint limits

- Ranks 2..9 have static mapping coverage, without seating runtime acceptance.
- MASTER executable/in-game seating is not validated.
- Tagged-package clean-install smoke, including external texture availability,
  remains pending; development-environment visual smoke is not a substitute.
- The two EEK-path carving textures retain the provenance reservation above.
- The complete race/sex/state matrix, later manual appearance edits, recovery/
  reconnect for this seating checkpoint and native retirement/lifetime completion
  remain unvalidated. Earlier campaign and compatibility limitations below still
  apply; the roster-2 result does not supersede them.

## Campaign continuity

| Scenario | Evidence/state |
|---|---|
| Two-player sealed campaign and coordinated checkpoint | Live validated |
| Candidate -> Committed full-roster barrier | Live validated for two players; automated for 2/4/10 slots |
| Collective restore of each player's exact `.ess + .skse` bundle | Live validated for one and two players |
| Successive checkpoint then successive recovery | Live validated with two players |
| Client/server restart recovery rehydration | Live validated for the implemented persisted recovery flow |
| Abrupt server stop after a committed checkpoint | Live validated; exact `LastCommittedCheckpoint` survived |
| Disconnect incident: Stay and recover | Live validated with two players |
| Disconnect incident: Return to Main Menu, then Continue/Resume | Live validated with two players |
| Three- or four-player live recovery | Not yet validated |
| Millisecond mid-ACK disconnect, first-ACK packet loss, pre-commit force-kill | Deterministic ordering/replay/transaction evidence only; not manually reproduced live |
| Complete recovery error/controller/resolution/negative presentation matrix | Incomplete; tracked by issue #57 |

Campaign progression never supports a partial sealed roster, late campaign join,
player replacement, host authority, or continue-without-player fallback.

## Upgrade, persistence, protocol, and saves

- **Client/server mixing:** unsupported and fail-closed. Authentication requires
  exact `BUILD_COMMIT` equality, so `0.2.0-alpha.1`, `0.3.0-alpha.1` and
  `0.4.0-alpha.1` clients and servers cannot be mixed. Use identical artifacts from one release.
- **Campaign database:** SQLite campaign schema v2 was introduced in
  `0.3.0-alpha.1`; this release does not establish an old-save upgrade guarantee. A
  schema-v1 STRE campaign database migrates transactionally to v2; malformed,
  failed, or newer-schema stores fail closed without reset. The public
  `0.2.0-alpha.1` release did not expose this campaign-continuity database as a
  player contract.
- **Downgrade:** not supported for campaign state. Back up
  `state/stre-server.sqlite3` before upgrading. A 0.3 schema-v2 campaign must
  not be opened or inferred by an older build; restore the matching server and
  database backup together if reverting.
- **Native saves:** campaign checkpoints are dedicated local per-player
  `.ess + .skse` bundles with versioned sidecar identity and fingerprints.
  Native payloads are never uploaded. Keep each player's complete save/profile
  data; another roster member's bundle is not a substitute.
- **Older/unmarked saves:** they remain ordinary Skyrim saves outside an
  admitted campaign, but they do not become campaign checkpoints and cannot be
  inferred as a sealed campaign identity. Cold campaign Resume requires a valid
  local marker/binding and server-authoritative admission.
- **Character Build:** catalog `BuildVersion = 5` remains current, but durable
  Character Build binding/restoration through campaign identity is not yet
  complete.

## Mods and load order

- `SkyrimTogether.esp` and `STRE_AlternateStart.esp` must both be enabled for
  the intended STRE campaign/Alternate Start flow.
- `Alternate Start - Live Another Life` must not be active with
  `STRE_AlternateStart.esp`; coexistence is not supported in this alpha.
- SkyUI is not in the validated campaign load/Journal matrix. Live diagnosis on
  AE `1.6.1170` found an older SkyUI `quest_journal.swf` incompatible with the
  current AE native callback contract and capable of crashing the Journal's
  Show All Saves path. Reproduce campaign load issues with the vanilla Journal
  UI before reporting STRE behavior.
- The native Skyrim Gameplay rows are not visually disabled by STRE. The STR
  Settings surface is informational; enforcement remains at engine-safe save
  and load boundaries.

## Existing feature matrix

| Configuration | Trading | World Sync drops | World Sync placed/grab | Character Build single-player | Character Build STRE |
|---|---:|---:|---:|---:|---:|
| 1 offline player | N/A | vanilla/local | Better Grabbing local | Yes | N/A |
| 2 players | Alpha | Validated | Validated on common cases | N/A | Smoke-tested |
| 4 players | To test | To test | To test | N/A | To test |
| SkyUI | Retest required | N/A | N/A | development environment | not validated for campaign load/Journal flows |
| Anniversary content | To test | runtime 1.6.1170 | runtime 1.6.1170 | runtime 1.6.1170 | runtime 1.6.1170 |

## Validated World Sync behavior

- dynamic drop -> remote materialization;
- local Havok plus authoritative settlement;
- remote pickup;
- grab/release of a dropped WorldEntity;
- lazy adoption of a movable placed reference;
- placed-reference grab/release without an observer crash;
- ownership triggering vanilla theft on grab;
- forced release when guard dialogue opens;
- swimming after correcting the STRE regression.

## World Sync scope to extend

- quest objects;
- heavily scripted references;
- complex enable-parent references;
- cell reset;
- custom item names;
- durable persistence after restart or save branching;
- 4 players.

## Mod/dependency test record

For every mod or dependency, record:

- name and version;
- Skyrim runtime;
- load order when relevant;
- single-player result;
- STRE result;
- conflict or workaround;
- logs;
- date;
- STRE SHA.
