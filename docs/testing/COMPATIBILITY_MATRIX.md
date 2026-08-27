# Compatibility matrix

> **Status: Source of truth for known compatibility**
> **Last updated: August 27, 2026**

## Reference platform

| Component | Observed version/support | State |
|---|---|---|
| Skyrim SE/AE runtime | `1.6.1170` Steam | primary development runtime and `0.3.0-alpha.1` public-alpha target |
| Operating system | Windows x64 | playable client/server target; Linux CI builds shared/server code but does not establish Linux client support |
| Skyrim Together Reborn upstream | historical baseline in `UPSTREAM.md` | fork integrated into STRE |
| STRE | `0.3.0-alpha.1` | Campaign Continuity Alpha; Windows native/UI/package gates and two-player live campaign evidence |
| Angular | 16.x | in use |
| xmake | `3.0.9` in CI (`>= 3.0.0` project) | Windows and Linux CI build toolchain |
| Creation Kit | environment compatible with `1.6.1170` | current ESP/Papyrus authoring and strict manifest target |
| SKSE64 | `2.2.6` for Skyrim `1.6.1170` | alpha installation target |
| Better Grabbing | `1.17` | external plugin required by default for multiplayer World Sync manipulation |
| Address Library | all-in-one package containing database `11` for runtime `1.6.1170` | required external dependency; runtime base for STRE/Better Grabbing |

These are the **`0.3.0-alpha.1` release targets**. Other Skyrim runtimes,
stores, operating systems, SKSE/Address Library combinations, or dependency
versions are not part of this release's supported matrix.

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
  exact `BUILD_COMMIT` equality, so `0.2.0-alpha.1` and `0.3.0-alpha.1` clients
  and servers cannot be mixed. Use identical artifacts from one release.
- **Campaign database:** `0.3.0-alpha.1` uses SQLite campaign schema v2. A
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
