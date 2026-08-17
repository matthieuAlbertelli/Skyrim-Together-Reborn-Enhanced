# New Game bootstrap spike

> **Status: New Game bootstrap and MQ101/post-Helgen projection implemented and runtime smoke-tested; MQ102/MQ103 handoff and Departure remain open.**
>
> Evidence snapshots: New Game bootstrap — 15 August 2026; MQ101/post-Helgen continuity — 16 August 2026.
> Runtime: Skyrim SE `1.6.1170.0`.

## Decision summary

The validated production bootstrap uses the same early Creation Kit/Papyrus
boundary identified by this spike, with one important refinement from the
prototype: Bethesda's normal `MQQuickstart == 0` stage-0 fragment remains
unchanged.

`STRE_AlternateStart.esp` instead overrides vanilla `MQQuickstart [0004679E]`
to value `5` and adds a sixth MQ101 stage-0 log entry conditioned on
`MQQuickstart == 5`. That STRE branch:

1. sets the game hour;
2. moves the player directly to `STRE_REFR_NewGameStartMarker` in the inn;
3. starts `STRE_QUEST_AlternateStart`;
4. never calls MQ101 `SetStage(10)`.

The Alternate Start quest has stage 10 marked as Start Up Stage. Stage 10 moves
the player to the seat, stage 20 hands off to `CharacterCreationService`, and
RaceMenu/Angular Character Creation continues from there.

No native executable hook is required. Native code participates only in the
existing Character Creation lifecycle. Runtime testing exposed one same-process
re-entry issue: after returning to the main menu, Skyrim can restart the quest
and reach stage 20 without emitting another quest-stage event. The validated
fix observes the Alternate Start quest Start/Stop lifecycle, rearms the native
service on a fresh `Start()`, then recovers stage 20.

The compatibility cost remains unavoidable: the plugin wins the MQ101 quest
record and ships `QF_MQ101_0003372B.pex`. Other mods that change MQ101 or that
fragment script require an explicit patch or are mutually exclusive.

Runtime smoke tests validated first New Game, a second New Game in the same
Skyrim process, and loading an ordinary existing save without retriggering the
bootstrap. A subsequent continuity increment now projects the required
MQ101/post-Helgen state through MQ101 stage 1000 while leaving MQ102/MQ102A/
MQ102B untouched. The neutral MQ102/MQ103 handoff and real Departure remain
separate work.

## Scope and ownership

This spike covers only a genuinely fresh Skyrim **New Game** and the handoff to
the existing Alternate Start flow.

The CK/Papyrus layer owns:

- interception of the Skyrim New Game chargen quest;
- MQ101 and Helgen state;
- quest aliases, markers, furniture, scenes, and `MoveTo`;
- Valen and local Alternate Start progression;
- projection of an authorized campaign phase into Skyrim;
- eventual resumption of vanilla quests.

The native STRE client/server owns durable identity, campaign admission,
roster, readiness, campaign phases, and multiplayer authority. It does not
directly set vanilla quest stages or patch MQ101.

This spike does not design or implement managed `.ess` checkpoints (#55),
recovery lock (#56), checkpoint fingerprints, or save coordination.

## Evidence and confidence

### Locally verified evidence

The following was inspected against the installed Skyrim SE 1.6.1170 data and
the repository at main `416f88a4093917d926a9779e0fbdfe26c50797a4`:

- `SkyrimSE.exe` contains the engine-facing names `StartNewGame` and
  `CharGenQuest`;
- `Skyrim.esm` contains MQ101 as `[QUST:0003372B]`;
- MQ101's QUST flags are `0x0100` (`Run Once`), not `Start Game Enabled`;
- its generated fragment source is `QF_MQ101_0003372B.psc`;
- the normal stage-0 fragment sets the game hour and immediately calls
  `SetStage(10)`;
- stage 10 performs the cart/chargen setup: it hides first-person geometry,
  enters chargen, changes controls and inventory, moves the player and cart
  actors, and starts the intro setup;
- the vanilla RaceMenu call occurs later in MQ101, after the cart has reached
  the Helgen town-square flow;
- MQ101 hands off to the Imperial or Stormcloak MQ102 branch near the end of
  Helgen Keep;
- leaving the Helgen location through those branches advances their objectives
  and cleans up MQ101 with stage 1000;
- generic MQ102 initializes Civil War background state, Riverwood interaction
  state, and the transition toward MQ103;
- Creation Club's `CCStartAfterCharGenScript` waits for MQ101 stage 1000, so
  that stage is also a broad "chargen finished" signal outside the main quest;
- `Update.esm` does not contain an MQ101 override in the inspected installation.

The installed Bethesda-distributed Papyrus sources used for this audit were:

- `QF_MQ101_0003372B.psc`;
- `MQ101QuestScript.psc`;
- `MQ101PlayerScript.psc`;
- `MQ101StartingCellLoadRegisterScript.psc`;
- the MQ102 branch and generic MQ102 quest fragments/scripts;
- `CCStartAfterCharGenScript.psc`.

The xEdit QUST definition confirms the meaning of the QUST DNAM flags and the
stage/alias structures. The repository's current strict plugin audit passes with
49 expected STRE-owned records and only the explicit named/anonymous
Skyrim-master allowlist recorded in `CK_RECORDS_M7_IMPLEMENTED.json`. Any
additional master-backed record is rejected.

### Inference that requires a runtime trace

The engine selects its `CharGenQuest` when New Game is chosen and MQ101 is the
vanilla quest that receives this special startup. MQ101 is observably not SGE,
and its stage 0 runs at New Game in the documented and established CK behavior.
The precise native call site that resolves `CharGenQuest` is not exposed by the
plugin records or Bethesda Papyrus source. That internal call site is not
needed by the proposed CK design, but the first prototype must log the stage-0
entry to close the timing proof on every supported runtime.

Historical candidate sections below are retained as design evidence. Current
implementation truth is recorded in
[`docs/project/STATUS.md`](../../project/STATUS.md),
[`CK_IMPLEMENTATION.md`](CK_IMPLEMENTATION.md), the strict manifest, and the
versioned runtime sources.

## Verified vanilla startup path

```text
Main menu: New
    |
    v
engine StartNewGame / CharGenQuest initialization
    |
    v
MQ101 [0003372B] starts through the chargen-specific engine path
    |
    v
MQ101 stage 0, normal-start log entry (MQQuickStart == 0)
    |
    +-- set GameHour = 7
    `-- SetStage(10)
            |
            v
        cart actors/player moved and constrained
        chargen/camera/control/inventory setup
        Helgen intro scenes and aliases progress
            |
            v
        RaceMenu later in the town-square sequence
            |
            v
        Helgen Keep -> MQ102A or MQ102B -> generic MQ102
```

The validated STRE interception boundary is MQ101 stage-0 branch selection
before any branch calls `SetStage(10)`. Letting vanilla stage 10 occur and trying
to undo it later is unsafe: actor packages, aliases, controls, inventory
mutations, cell loads, and scenes have already begun.

Validated STRE path:

```text
Main menu: New
    |
    v
MQQuickstart = 5
    |
    v
MQ101 stage 0, STRE sixth log entry
    |
    +-- GameHour = 7
    +-- Player.MoveTo(STRE_REFR_NewGameStartMarker)
    `-- STRE_QUEST_AlternateStart.Start()
            |
            v
        Start Up Stage 10
            |
            v
        seat -> stage 20
            |
            v
        CharacterCreationService
            |
            v
        RaceMenu -> Angular Character Creation
```

The vanilla `MQQuickstart == 0` fragment remains byte-semantically unchanged.

## MQ101 and Helgen dependency map

| Component | Relevant vanilla responsibility | STRE consequence |
|---|---|---|
| MQ101 stage 0 | Selects normal or Bethesda quick-start fragment | Add the STRE `MQQuickstart == 5` branch and leave Bethesda's normal branch unchanged. |
| MQ101 stage 10 | Enters chargen and configures carts, actors, controls, camera, inventory, and initial scenes | Must never run during an STRE start. |
| MQ101 aliases | Player, Hadvar, Ralof, Ulfric, Tullius, Alduin, carts, horses, prisoners, Helgen markers/locations, and scene actors | Do not assume all vanilla aliases were exercised merely because MQ101 started. |
| MQ101 scenes/packages | Cart ride, town square, dragon attack, keep branches | Must remain stopped/unstarted while the player is in the STRE bootstrap. |
| MQ101 stage 900/1000 | Late intro/cleanup and global chargen-completion signals | Advance only through a deliberately validated departure adapter, never as a bootstrap shortcut. |
| MQ101DragonAttack / Helgen world state | Destroyed-Helgen presentation, actors, markers, acoustic/music state | Choose and test one coherent post-Helgen projection; do not partially simulate the attack. |
| MQ102A / MQ102B | Hadvar/Ralof relationship, Riverwood route, and Civil War-side context | Do not select a side implicitly unless product design explicitly requires it. |
| MQ102 | Riverwood/Whiterun progression, Civil War background initialization, Alduin cleanup, transition to MQ103 | Departure must enter one tested canonical route exactly once. |
| MQ103 and later main quest | Bleak Falls Barrow and subsequent vanilla chain | Let vanilla own this chain after the chosen MQ102 handoff. |
| MQ104 and later dragon/shout state | Dragon Rising and the later Dragonborn/shout progression | Do not enable dragons or grant shouts during bootstrap. |
| Creation Club post-chargen starts | Some scripts wait for MQ101 stage 1000 | Keep them deferred until STRE character creation is genuinely complete. |
| Civil War | Generic MQ102 starts its background state; branch quests also assign contextual allegiance | Initialize once at the selected vanilla handoff; never infer allegiance from STRE campaign identity. |

`SetStage(1000)` alone is not a safe substitute for this dependency map. A
stage call can run fragments, but it does not prove that every physical Helgen
reference, scene, alias, package, dialogue condition, or downstream quest is in
the intended state.

## Current STRE plugin audit

`STRE_AlternateStart.esp` now contains 49 manifest-owned STRE records, including
`STRE_REFR_NewGameStartMarker` and `STRE_QUEST_HelgenNPCCleanup`, plus the
explicit Skyrim-master override allowlist in
`CK_RECORDS_M7_IMPLEMENTED.json`. Every master-backed record outside that
allowlist fails `--reject-unexpected`.

`MQQuickstart` is fixed to value `5`; the audit verifies that value.

`STRE_QUEST_AlternateStart`:

- is not Start Game Enabled;
- owns Player and two seat aliases;
- moves the player to an available seat in the existing stage-10 bootstrap;
- advances to stage 20 after seating;
- lets `CharacterCreationService` observe stage 20 and open RaceMenu;
- supports an offline/local build path as well as server-authoritative build
  validation when connected.

The current plugin therefore has the deterministic New Game bridge, destination
cell, furniture, character-creation bootstrap, and validated MQ101/post-Helgen
projection needed before Departure. The remaining continuity problem is the
neutral MQ102/MQ103 vanilla handoff and its later authorized Departure trigger.
The existing plugin should be extended; a second plugin would add load-order
and ownership ambiguity without removing the MQ101 conflict.

## Candidate interception approaches

### A. Independent Start Game Enabled quest

**Rejected as the primary boundary.** An SGE quest can initialize on a new
game, but quest start is latent and menu-mode initialization can be deferred.
There is no ordering guarantee that it runs before MQ101 stage 0 synchronously
advances to stage 10. A race that usually wins is not an interception contract.

### B. Story Manager event

**Rejected.** No audited Story Manager event precedes the chargen-specific
MQ101 start. Location-change or cell-load events occur after the point at which
the cart setup may already have mutated the game.

### C. Minimal MQ101 stage-0 CK override

**Validated with a refinement.** Keep the normal-start log entry unchanged,
override `MQQuickstart` to `5`, and add a sixth stage-0 entry for STRE. The STRE
branch moves the player into the inn and starts the STRE quest without calling
MQ101 stage 10. Bethesda's existing quick-start branches remain untouched.

Benefits:

- deterministic and earlier than the cart setup;
- uses the engine's existing chargen quest boundary;
- keeps Skyrim quest manipulation in CK/Papyrus;
- works before any server connection exists;
- requires no executable-version address or pattern.

Costs:

- winning MQ101 QUST override in the load order;
- generated `QF_MQ101_0003372B.pex` replacement;
- explicit incompatibility/patch policy for every other MQ101 alternate start;
- careful three-way review when Bethesda/USSEP changes the winning record.

### D. Native `StartNewGame` or MQ101 hook

**Rejected unless the CK prototype fails.** It would be runtime-version
sensitive, would move quest manipulation into native code, and would not remove
the need to understand MQ101/MQ102 cleanup. The CK boundary is already early
enough.

### E. Reuse or copy another alternate-start implementation

**Rejected.** Existing mods are useful comparative evidence: their authors
also disclose MQ101/QF conflicts, and some deliberately rewrite MQ102 and later
main-quest behavior. STRE needs a narrower contract and its own tested vanilla
handoff. No third-party record, script, or architecture should be imported
blindly.

## Comparative implementation evidence — Alternate Start: Live Another Life

Inspection of the distributed Alternate Start — Live Another Life archive
provides implementation evidence for the proposed CK boundary.

Relevant observations:

- the mod replaces the MQ101 generated quest fragment;
- New Game takeover occurs from MQ101 before the normal Helgen cart path;
- the alternate chargen/bootstrap is owned by Papyrus/CK rather than a native
  Skyrim executable hook;
- the mod does not treat skipping MQ101 stage 10 as sufficient vanilla cleanup;
- later vanilla continuity work explicitly handles MQ101/Helgen state;
- MQ102 is also adapted for main-quest continuity;
- Civil War, Riverwood/Whiterun progression, Helgen state, Alduin, Hadvar and
  Ralof are treated as separate continuity concerns;
- the main quest can remain deferred while the alternate-start experience
  proceeds.

STRE does not need Live Another Life's alternate-start scenarios.

The relevant architectural pattern is narrower:

1. intercept New Game through the dedicated MQ101 `MQQuickstart == 5` branch before stage 10;
2. transfer control to an STRE-owned chargen/bootstrap quest;
3. keep the vanilla main quest dormant during the STRE Alternate Start;
4. reconstruct only the minimum coherent post-Helgen vanilla state required by
   STRE;
5. perform eventual MQ102/MQ103 handoff through a separately tested continuity
   adapter.

STRE must reproduce the required behavior independently rather than copy
third-party scripts or records verbatim.

## Recommended CK/Papyrus architecture

### 1. Thin vanilla bridge

The implemented bridge is the dedicated sixth MQ101 stage-0 branch selected by
`MQQuickstart == 5`. It must not own class rules, server state, Valen, or the
eventual MQ102 policy.

Implemented responsibility:

```text
MQ101 stage-0 STRE branch
    -> trace New Game interception
    -> set GameHour
    -> Player.MoveTo(STRE_REFR_NewGameStartMarker)
    -> STRE_QUEST_AlternateStart.Start()
    -> do not call MQ101.SetStage(10)
```

Because the fragment script is generated, the full winning QF PEX must be
shipped even though the semantic change is one branch. The source, plugin
override, and compiled PEX must be reviewed as one artifact set.

### 2. STRE-owned bootstrap controller

Prefer a small script attached to the existing `STRE_QUEST_AlternateStart` or a
small STRE-prefixed helper quest in the same plugin if alias-start latency makes
that separation necessary. Its API should be idempotent, for example a single
`BeginFreshGame()` entry point.

It should:

1. distinguish a fresh New Game invocation from a save load;
2. start/reset the existing STRE quest once;
3. establish the minimum tested chargen/control/camera state;
4. move the player through CK properties to the existing bootstrap marker or
   seat flow;
5. invoke the existing stage 10 and stage 20 contract;
6. leave all server interaction optional until after local takeover;
7. retain enough local state to make duplicate stage events harmless.

It must not encode `CampaignId`, `PlayerId`, slot ownership, checkpoint IDs, or
server decisions in quest stages.

### 3. Existing Alternate Start projection

After the controller starts the existing quest, the current CK and native flow
remains the projection mechanism:

```text
STRE quest stage 10
    -> choose/fill seat alias
    -> MoveTo / furniture bootstrap
    -> stage 20
    -> CharacterCreationService
    -> RaceMenu
    -> STRE class/build flow
```

The implementation must reconcile chargen entry/exit with the existing native
RaceMenu lifecycle. In particular, it must test `Game.SetInChargen`, camera,
HUD, control flags, default race abilities, and inventory ordering. It must not
blindly call MQ101's `AddRaceSpells()` because the current STRE build pipeline
deliberately removes and reapplies canonical spells.

### 4. Separate vanilla departure adapter

Do not combine interception with MQ101/MQ102 cleanup in one stage-0 fragment.
The departure adapter should run only after STRE character creation and the inn
flow are complete, and only once.

The production implementation needs one explicit product choice:

- **vanilla branch handoff:** select Hadvar or Ralof and use the corresponding
  MQ102 branch; or
- **neutral post-Helgen handoff:** establish a tested generic MQ102/Whiterun
  state without assigning Civil War allegiance.

The neutral route best matches a class-driven alternate start, but it is only a
proposal. Vanilla's generic MQ102 fragments intertwine Riverwood, Whiterun,
Civil War initialization, Alduin cleanup, and MQ103. The exact stage sequence
must be derived and runtime-tested in a CK prototype rather than guessed from
quest numbers.

Whichever route is approved must:

- finalize MQ101/chargen only after STRE creation is complete;
- produce one coherent destroyed-Helgen state;
- make MQ102, Riverwood, Whiterun, and Civil War conditions coherent;
- leave dragons disabled until the normal main-quest unlock point;
- grant no shout or dragon soul during bootstrap;
- allow the vanilla MQ103+ chain to proceed normally;
- avoid pretending that quest stages reconstruct every world reference.

## Moving before RaceMenu

Moving the player into `STRE_CELL_AlternateStart` before RaceMenu is feasible
at the CK/Papyrus layer. Bethesda's own MQ101 quick-start fragments move the
pre-RaceMenu player, and established alternate-start designs use the same
engine capability.

It is safe for STRE only if the prototype proves all of the following:

- MQ101 stage 10 never ran;
- the target cell and its persistent references are loaded;
- the Player and seat aliases fill without relying on Helgen aliases;
- no cart package or player AI-driven state is active;
- input remains deliberately locked until the existing STRE flow owns it;
- RaceMenu opens and closes correctly from the target cell;
- race, sex, name, appearance, racial abilities, first-person geometry, camera,
  HUD, inventory, and controls are correct after the STRE build is sealed;
- a second New Game in the same process does not reuse stale bootstrap state.

## Bootstrap state distinction

These states are architectural inputs, not additional Skyrim quest phases:

| State | Meaning | New Game behavior |
|---|---|---|
| `UNMANAGED_NEW_GAME` | No durable STRE campaign binding exists. | CK takes over unconditionally; character creation and later local Alternate Start progression use the offline/local path. |
| `CAMPAIGN_BOOTSTRAP` | Campaign/slot/binding exists, but no committed managed Skyrim checkpoint exists. | CK takes over the same way; after local takeover the server may authorize canonical campaign phases and the CK adapter projects them. |
| `MANAGED_CAMPAIGN_SAVE` | A later #55 `CampaignCheckpoint` and member `.ess` exist. | This is a load, not New Game. It must bypass this MQ101 bootstrap and use the managed-load gate designed by #55/#56. |

Network availability never decides whether MQ101 is intercepted. A disconnected
client and a machine with no dedicated server must still enter the inn. A
connection can change authority after takeover, not the existence of the
takeover.

## Solo flow

```text
New Game
  -> MQ101 stage-0 STRE bridge
  -> local STRE bootstrap
  -> inn / seat / stage 20
  -> RaceMenu and local build application
  -> Valen and class flow
  -> approved local departure adapter
  -> coherent vanilla world and main quest
```

No server request is allowed on the critical path into the inn. A failed or
absent network connection must not stall the local quest.

## Multiplayer flow

```text
New Game
  -> same unconditional CK bootstrap
  -> local inn takeover complete
  -> connection and campaign admission (native authority)
  -> canonical CampaignPhase received
  -> CK adapter projects only the authorized local phase
  -> same RaceMenu / class / inn content
  -> coordinated campaign start authorizes departure
```

The server never sends `SetStage(MQ101, ...)`. It sends campaign meaning; the
adapter performs the approved Skyrim projection. This preserves the fixed
roster and authority invariants in ADR-0018 without making the host or CEF a
quest authority.

## Exact artifacts likely to change in production

The bootstrap implementation touches the existing plugin artifact set,
Character Creation lifecycle code, and its documentation/tooling:

| Artifact | Expected change |
|---|---|
| `GameFiles/Skyrim/STRE_AlternateStart.esp` | Win `MQ101`, set `MQQuickstart` to `5`, and add `STRE_REFR_NewGameStartMarker`. |
| `GameFiles/Skyrim/Source/Scripts/QF_MQ101_0003372B.psc` | Track the generated winning fragment source with the narrow STRE delegation. |
| `GameFiles/Skyrim/scripts/QF_MQ101_0003372B.pex` | Ship the CK-compiled fragment used at runtime through the existing game-file packaging path. |
| `QF_STRE_QUEST_AlternateStart_02001AF9.psc` or a small STRE controller script | Add idempotent fresh-game bootstrap entry/projection, keeping CK operations out of native code. |
| `Code/client/Services/CharacterCreationService.*` | Rearm Character Creation on a fresh Alternate Start quest lifecycle so same-process New Game works even when Skyrim does not emit another stage event. |
| `CK_RECORDS_M7_IMPLEMENTED.json` | Track the new STRE marker, approved `MQQuickstart`/`MQ101` master overrides, and the pre-existing anonymous NAVI baseline without weakening STRE prefix checks. |
| `Tools/Scripts/audit_stre_plugin_records.py` | Validate approved master overrides, verify `MQQuickstart = 5`, preserve the explicit NAVI baseline exception, and reject every other master-backed record. |
| feature documentation/tests | Record the final CK record delta, handoff policy, and runtime evidence. |

No native hook, second ESP, database schema, or campaign save format is required
for New Game interception. A small `CharacterCreationService` lifecycle fix is
required for reliable same-process New Game re-entry.

## Compatibility policy

The MQ101 override is a hard conflict surface, not a soft integration point.

- Only one winning `MQ101 [0003372B]` record exists at runtime.
- Only one loose/BSA winner for `QF_MQ101_0003372B.pex` exists.
- A plugin record winning while another mod's QF script wins can be worse than
  either mod winning completely.
- Alternate Start — Live Another Life documents conflicts with MQ101, MQ102,
  and the QF scripts it changes.
- Skyrim Unbound Reborn documents broader changes through MQ102 and later main
  quest/dragon behavior.
- Alternate Perspective publicly describes delaying/reframing the intro and
  returning to vanilla after Helgen; it is a useful test comparison, not an
  implementation source.

STRE should initially declare other MQ101 alternate-start mods incompatible.
Compatibility patches must be explicit, versioned, and tested; load-order
advice alone is insufficient.

USSEP compatibility needs a winning-record comparison against the supported
USSEP version. The STRE override must forward every applicable upstream/USSEP
fix outside the single intentional stage-0 delta.

## Failure and recovery behavior

| Failure | Required behavior |
|---|---|
| STRE loses the MQ101 record or QF script conflict | Detect/report the incompatible winner during support diagnostics; do not add a native fallback hook. |
| STRE quest fails to start or aliases fail to fill | Fail closed before cart stage 10, log the exact property/alias failure, and retain a diagnostic escape path. Do not partially resume MQ101. |
| Target cell or seat unavailable | Keep the player controlled by the bootstrap, report the missing reference, and allow a tested retry/fallback marker inside the same STRE cell. |
| Native client present but disconnected | Continue through the local flow; no server wait. |
| Server disconnects after admission | Follow campaign authority/recovery policy; do not restart MQ101. |
| RaceMenu fails or is cancelled | Preserve the existing character-creation control lock and retry/error UI; do not advance departure. |
| Fresh quest Start after returning to the main menu | Treat it as a new Character Creation lifecycle, reset stale native UI/control/build state, then recover stage 20 once. Duplicate events within the same lifecycle must remain idempotent. |
| Ordinary save is loaded | Do not invoke the New Game bridge. |
| Managed campaign save is loaded | Future #55/#56 flow owns it; never reset MQ101 or the STRE bootstrap. |
| Vanilla handoff fails | Remain in a diagnosable pre-departure state and do not enable an incoherent partial world. |

## Test matrix

### CK/plugin structural checks

- strict manifest audit passes;
- exactly two intentional EditorID-bearing Skyrim overrides exist: `MQQuickstart` and `MQ101`, plus the explicit pre-existing anonymous NAVI baseline record;
- MQ101 winning record differs from the supported master/USSEP record only in
  the approved stage-0 branch and generated fragment metadata;
- the deployed QF PEX matches the reviewed PSC/plugin fragment;
- no hard-coded load-order FormID is added to Papyrus;
- all STRE properties resolve in CK and alias fill failures are logged.

### New Game runtime checks

Run on every supported Skyrim executable and load-order baseline:

1. choose New from the main menu;
2. observe the MQ101 STRE stage-0 trace exactly once;
3. prove MQ101 stage 10 was never set;
4. prove no cart/Helgen scene or package started;
5. prove the player reaches `STRE_CELL_AlternateStart` before RaceMenu;
6. prove seat selection and stage 20 occur once;
7. complete RaceMenu and the STRE build;
8. validate camera, controls, HUD, inventory, spells, race abilities, level,
   name, and appearance;
9. start another New Game without restarting Skyrim and repeat;
10. load an ordinary existing save and prove the bootstrap did not run.

### Solo/network matrix

| Client | Server | Binding | Expected |
|---|---|---|---|
| installed | absent | none | Full local bootstrap and build flow. |
| installed | unreachable | none | Same local flow; network error cannot block entry. |
| installed | connected | none | Local bootstrap; no campaign authority invented. |
| installed | connected | admitted/bootstrap | Same bootstrap; later phases wait for canonical server authorization. |
| installed | disconnect during bootstrap | admitted/bootstrap | Local CK state remains coherent; campaign progression fails closed without touching MQ101. |

### Vanilla handoff regression

The approved departure path must validate:

- MQ101 stops/completes at the intended time, with stage 1000 consumers
  released only after character creation;
- Helgen exterior/interior references match one coherent post-intro state;
- Hadvar, Ralof, Ulfric, Tullius, Alduin, carts, horses, prisoners, gates,
  music, and acoustic state have no stranded active packages/references;
- MQ102/Riverwood/Whiterun dialogue and objectives progress;
- Civil War starts in a coherent neutral or explicitly selected branch;
- MQ103 starts from vanilla dialogue, not from the bootstrap;
- dragons remain gated until vanilla progression authorizes them;
- shouts, dragon souls, and words are not granted early;
- Creation Club post-chargen quests start at the intended time;
- entering Helgen/Helgen Keep later does not expose half-run intro triggers;
- save, reload, fast travel, arrest, death/reload, and a second client in the
  same campaign do not duplicate the adapter.

### Conflict matrix

- supported vanilla + Update baseline;
- supported USSEP baseline;
- known MQ101 record conflict;
- known `QF_MQ101_0003372B.pex` loose-file conflict;
- one representative alternate-start mod, expected to be detected/documented
  as incompatible rather than silently combined;
- Vortex staging/deployed hardlink installation path.

## Implementation status and next milestones

1. **CK timing/bootstrap prototype — complete.** New Game ordering, direct world
   transition, and suppression of MQ101 stage 10 are runtime-validated.
2. **Minimal plugin bridge — complete for bootstrap.** MQ101/QF, `MQQuickstart`,
   marker, strict audit allowlist, and generated PSC/PEX are versioned.
3. **Solo bootstrap acceptance — complete.** Seat, stage 20, RaceMenu, local
   build, same-process second New Game, and ordinary-save non-retrigger are
   smoke-tested.
4. **MQ101/post-Helgen projection — complete.** The required audited MQ101
   continuity stages, skipped-actor cleanup, destroyed-Helgen reference state,
   and Keep-collapse historical projection are runtime-validated while
   MQ102/MQ102A/MQ102B remain untouched.
5. **Campaign bootstrap projection — pending.** Map admitted pre-checkpoint
   campaign phases to the same CK flow without implementing managed saves.
6. **Neutral MQ102/MQ103 departure decision/prototype — pending.** Prove the
   exact generic MQ102/MQ103, Alduin, Riverwood, and Civil War state needed by
   STRE before wiring the real Departure authority path.
7. **Compatibility and regression gate — pending.** Compare winning records,
   execute the supported-load-order matrix, and document explicit conflicts.

Each milestone should be independently reviewable. #55 and #56 remain separate
and must not be pulled into these increments.

## Open decisions before production

The current implementation has resolved two earlier questions: MQ101 now reaches
stage 1000 as part of the validated pre-handoff continuity projection, and
destroyed Helgen is projected before the future MQ102/MQ103 Departure handoff.

Remaining decisions:

1. Prove the exact neutral generic MQ102/MQ103 route without selecting a
   Hadvar/Ralof Civil War allegiance.
2. Which USSEP version is the v1 compatibility baseline for the MQ101 forwarded
   override?
3. What diagnostic escape is allowed if the CK bootstrap fails before stage 20
   without turning that escape into an alternate authority path?

## References

Primary or project-owned evidence:

- Bethesda-distributed Skyrim SE 1.6.1170 plugin and Papyrus sources listed in
  [Evidence and confidence](#evidence-and-confidence);
- [xEdit Skyrim QUST record definitions](https://github.com/TES5Edit/TES5Edit/blob/93cc0bc5a1251936c3c7859eee3150eda12a62d7/Core/wbDefinitionsTES5.pas);
- [Creation Kit `Quest.Start()` behavior](https://ck.uesp.net/wiki/Start_-_Quest);
- [Creation Kit `Quest.SetStage()` behavior](https://ck.uesp.net/wiki/SetStage_-_Quest);
- [current STRE CK implementation](CK_IMPLEMENTATION.md);
- [current STRE state model](STATE_MODEL.md);
- [campaign state and ownership](../../architecture/CAMPAIGN_STATE.md);
- [ADR-0018](../../architecture/ADRs/ADR-0018-fixed-roster-coordinated-checkpoint-recovery.md).

Comparative author documentation, used only to identify public compatibility
patterns and risks:

- [Alternate Start — Live Another Life compatibility notes](https://www.nexusmods.com/skyrimspecialedition/mods/272);
- [Skyrim Unbound Reborn description and change history](https://www.nexusmods.com/skyrimspecialedition/mods/27962);
- [Alternate Perspective source repository and architecture summary](https://github.com/KrisV-777/Alternate-Perspective);
- [MQ101 Resources technical tutorial](https://www.nexusmods.com/skyrim/articles/53783).
