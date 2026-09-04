# Alternate Start — Creation Kit implementation

> **Status: New Game/CK bootstrap, MQ101/post-Helgen projection, the wounded-survivor investigation slice, and standalone T+4 are runtime-tested; the native campaign bootstrap and the multiplayer T+4 gate are automated-tested, while two-client Helgen validation, rescue/liberation, negative runtime coverage, MQ102/MQ103 handoff, introduction, and Departure remain**

## Versioned files

```text
GameFiles/Skyrim/STRE_AlternateStart.esp
GameFiles/Skyrim/Source/Scripts/QF_STRE_QUEST_AlternateStart_02001AF9.psc
GameFiles/Skyrim/scripts/QF_STRE_QUEST_AlternateStart_02001AF9.pex
GameFiles/Skyrim/Source/Scripts/QF_MQ101_0003372B.psc
GameFiles/Skyrim/scripts/QF_MQ101_0003372B.pex
GameFiles/Skyrim/Source/Scripts/QF_STRE_HelgenCleanup_0302D022.psc
GameFiles/Skyrim/scripts/QF_STRE_HelgenCleanup_0302D022.pex
GameFiles/Skyrim/Source/Scripts/STRE_HelgenContinuityController.psc
GameFiles/Skyrim/scripts/STRE_HelgenContinuityController.pex
GameFiles/Skyrim/Source/Scripts/STRE_HelgenCollapseLoadAlias.psc
GameFiles/Skyrim/scripts/STRE_HelgenCollapseLoadAlias.pex
GameFiles/Skyrim/Source/Scripts/QF_STRE_QUEST_HelgenInvestig_0305BCA5.psc
GameFiles/Skyrim/scripts/QF_STRE_QUEST_HelgenInvestig_0305BCA5.pex
GameFiles/Skyrim/Source/Scripts/STRE_HelgenInvestigationController.psc
GameFiles/Skyrim/scripts/STRE_HelgenInvestigationController.pex
GameFiles/Skyrim/Source/Scripts/STRE_HelgenRubbleSqueezeActivator.psc
GameFiles/Skyrim/scripts/STRE_HelgenRubbleSqueezeActivator.pex
GameFiles/STRE_AlternateStart.manifest.txt
```

PSC files alone are not executed by Skyrim: the compiled PEX must be retrieved and deployed.

## Confirmed primary records

- `STRE_CELL_AlternateStart` (`Ilinaltaâ€™s Vigil`)
- `STRE_CELL_DevSandbox`
- `STRE_QUEST_AlternateStart`
- `STRE_FURN_PlayerSeat01` through `STRE_FURN_PlayerSeat10`
- `STRE_REFR_NewGameStartMarker`
- `STRE_STAT_IlinaltaFireplace01`
- `STRE_LIGH_IlinaltaFireplace01`
- `STRE_LIGH_CandleHornWall01`

Intentional Skyrim master overrides include:

- `MQQuickstart [0004679E]` with value `5`;
- `MQ101 [0003372B]`, adding the STRE stage-0 and continuity branches while
  keeping the audited vanilla branches intact;
- `Tamriel [0000003C]` and `HelgenKeep01 [0005DE24]` structural parents retained
  by the CK for persistent continuity references;
- `MQ101SetStage360 [0005CEE3]`, `MQ101SetStage400 [000BA032]`,
  `MQ101SetStage267 [000BAC16]`, `MQ101SetStage368 [000F778E]`,
  `MQ101SetStage485 [000FDA33]`, and `MQ101SetStage210 [00103AF4]`, promoted as
  required by CK property binding;
- five anonymous approved master-backed records validated by exact signature
  and FormID: `NAVI [00012FB4]`, `CELL [00000D74]`, `REFR [0010FDE3]`, and the
  two captured-survivor jail doors `REFR [00091583]`/`REFR [00091587]`.

The strict manifest is the authority for this allowlist; do not infer approval
for any additional master-backed record from this prose summary.

Aliases used:

- `Alias_Player`
- `Alias_PlayerSeat01`

Quest stages:

- `0` — initialization;
- `10` — move and seat the player;
- `20` — trigger Character Creation.

The quest is intentionally excluded from generic quest synchronization.

## Current flow

```text
Main menu: New Game
→ MQQuickstart = 5
→ MQ101 stage 0 selects the STRE branch
→ Player.MoveTo(STRE_REFR_NewGameStartMarker)
→ STRE_QUEST_AlternateStart.Start()
→ Start Up Stage 10
→ MoveTo the seat through its alias
→ wait for the seated state
→ advance to stage 20
→ CharacterCreationService locks controls and opens the campaign-bootstrap CEF gate
→ Solo authorizes locally, or canonical sealed CharacterCreation + full-roster ACTIVE authorizes multiplayer
→ CharacterCreationService opens RaceMenu
→ Angular Character Creation
```

This gate is entirely native/CEF. No ESP, PSC, PEX, quest stage, or generic
quest-synchronization change was required for the lobby slice.

Runtime prerequisite: `Alternate Start - Live Another Life` must be disabled
when `STRE_AlternateStart.esp` is active. Running both was observed to be
incompatible during validation. Compatibility work is outside this slice.

The vanilla `MQQuickstart == 0` fragment remains unchanged and still calls `SetStage(10)`. The STRE branch must never call MQ101 stage 10.

`CharacterCreationService` also observes the Alternate Start quest Start/Stop lifecycle. A fresh quest `Start()` rearms the native flow so a second New Game in the same Skyrim process can recover stage 20 even when Skyrim does not emit a second quest-stage event.

Never hard-code a loaded FormID that depends on load order. CK references use aliases and properties; the native catalog uses plugin name plus local FormID.

## Headquarters v1 implementation boundary

`STRE_CELL_AlternateStart` is the physical headquarters interior cell for v1 and
now carries the player-facing name `Ilinaltaâ€™s Vigil`. Headquarters completion
uses normal Creation Kit instanced-interior mechanics, including Skyrim cell
transitions and load doors where applicable.

The current physical headquarters checkpoint is implemented and runtime-smoke-
tested. It includes the main inn shell and circulation space, the exterior
placement by Lake Ilinalta, a working interior/Tamriel load-door pair, tavern
music, an initial warm lighting pass, the STRE-owned fireplace/light records,
ten stable STRE starting-seat references, and an interior navmesh for the
current geometry. The exterior footprint touches Tamriel cells `(-9, -16)` and
`(-9, -17)`.

The interior architecture remains provisional: geometry, proportions, room
shapes, secondary circulation, composition, and some structural placements may
still evolve. Decoration is only a minimal first pass and does not yet provide
the intended furniture, clutter, functional tavern areas, environmental
storytelling, lived-in character, or final lighting/readability polish. The ten
current room spaces are empty and have no doors; they are not yet the ten usable
v1 player rooms, so issue #24 remains incomplete.

The xEdit pass for this checkpoint removed unintended master overrides. Two
vanilla exterior rock references and the nearby two-reference forest-predator
encounter are intentionally retained as disabled overrides so the headquarters
footprint remains clear without deleting the Skyrim master references. The two
exterior CELL overrides are retained as structural CK parents.

This checkpoint does **not** complete issue #23. The v1 CK work still has to
finish the architecture and substantial decoration pass, continue the exterior
stair/access path down to the road, add a Skyrim-appropriate sign or signpost
for Ilinalta's Vigil, complete Valen integration and ready/departure
circulation, deliver the ten-room housing work owned by #24, and validate the
finished hub through the ten-player target.

Room Bounds and Portals were deliberately not implemented for the current
interior. They are no longer an unconditional v1 implementation technique or
acceptance gate: add them, or another explicit visibility-partitioning
solution, only if profiling or runtime validation demonstrates a concrete
visibility or performance problem. Navmesh, NPC pathing, collision, lighting,
visual readability, and acceptable runtime performance remain required.

The fireplace currently uses the selected EEK fireplace mesh/texture resource
in the development environment. Redistribution permission and final packaging
must be resolved before release; the v1 distribution must not require an
undocumented manual asset dependency.

This physical boundary does not change campaign authority: the future seamless
replacement must preserve the existing server-authoritative campaign contract.
Stable room ownership identities must remain logical and must not be defined by
the cell, a physical mesh, or load-order-dependent FormIDs. Current
implementation and validation remain documented only in
[`STATUS.md`](../../project/STATUS.md).

### Messire Valen prototype checkpoint

Versioned assets:

- `meshes\STRE\Valen\STRE_Valen_Master_test.nif`;
- `textures\STRE\Valen\STRE_Valen_d.dds`.

CK records:

- `STRE_ARMA_ValenFullBody`;
- `STRE_ARMO_ValenFullBody`;
- `STRE_OTFT_Valen`;
- `STRE_NPC_MessireValen`;
- `STRE_PACK_ValenInnSandbox`.

The integrated custom full-body prototype uses the Skyrim skeleton with
transferred prototype weights. Its Armor/ArmorAddon biped-slot setup hides
overlapping vanilla head, body, and hand geometry. Locomotion and general
animations were runtime-tested in game.

This is not the production FaceGen/dialogue head. Finger weighting remains
imperfect, the material/shader pass is provisional, and the temporary Sandbox
behavior moves Valen between chairs or other furniture too frequently. Final
AI, dialogue, scene, and narrative-departure work is deferred.

## M7 records and continuity helper

The legacy-named `CK_RECORDS_M7_IMPLEMENTED.json` strict manifest now covers 83 expected STRE-owned records:

- cells, quests, and the ten headquarters seat references;
- the Ilinalta fireplace static plus the two STRE-owned headquarters light records;
- the Messire Valen prototype's full-body `ARMA`/`ARMO`, outfit, NPC base, and
  provisional inn Sandbox package;
- outfits and boots;
- weak enchantments;
- Destruction and Alteration spells;
- targetable magic effects for ally buffs;
- the `STRE_REFR_NewGameStartMarker` placed reference used only for the initial world transition;
- `STRE_QUEST_HelgenNPCCleanup`, which owns the skipped-Helgen cleanup aliases
  and the continuity controller used by the post-Helgen projection;
- `STRE_QUEST_HelgenInvestigation` and the first pre-deadline investigation
  projection records;
- independent Hadvar/Ralof wounded anchors, wounded furniture references, and
  `SitTarget` packages;
- the shared rubble-squeeze activator, its two placed activators, and its two
  destination markers;
- the dead excavator bandit and placed pickaxe used by the environmental
  storytelling around the rubble opening;
- independent Hadvar/Ralof captured anchors and conditional Sandbox packages
  used by the `CapturedInKeep` projection.

The same strict manifest allows only the explicit named and anonymous
Skyrim-master records listed in its allowlists. For the Ilinalta checkpoint this
includes the two exterior CELL structural parents, two deliberately disabled
rock references, and the two deliberately disabled forest-predator ACHR
references qualified during the xEdit audit. Any additional master-backed
record is rejected by `--reject-unexpected`.

The three ally buffs must retain compatible values in both `SPEL` and `MGEF`:

```text
Casting Type : Fire and Forget
Delivery     : Target Actor
```

`Contact` is not appropriate for these manually cast spells.

## MQ101 / post-Helgen continuity

The current STRE continuity path deliberately stops at a neutral boundary:

```text
MQ101  -> stage 1000 / stopped
MQ102  -> untouched
MQ102A -> untouched
MQ102B -> untouched
```

The Alternate Start generated fragment advances the audited MQ101 continuity
stages, starts `STRE_QUEST_HelgenNPCCleanup`, removes or repositions the skipped
Helgen actors, then delegates complex world-reference projection to
`STRE_HelgenContinuityController`.

The controller owns the validated destroyed-Helgen enable/disable projection
and collapse-trigger neutralization. `STRE_HelgenCollapseLoadAlias` applies the
already-collapsed rubble visual when `HelgenKeep01` attaches. The implementation
does not call the vanilla collapse trigger and does not select an Imperial or
Stormcloak MQ102 branch.

This is a local Skyrim projection. The server must never synchronize raw
`MQ101.SetStage` or future `MQ102.SetStage` calls as campaign protocol.

## Helgen investigation and standalone T+4 slice

The first investigation increment is owned by:

```text
STRE_QUEST_HelgenInvestigation
STRE_HelgenInvestigationController
```

The quest is not Start Game Enabled. Its stage `10` is currently a diagnostic
bootstrap used to call `BeginInvestigation()`; it is not the canonical survivor
state machine and will later be replaced at the narrative boundary by Valen.

`BeginInvestigation()` records `Utility.GetCurrentGameTime()` only when the
investigation transitions out of its uninitialized state. The current local
state model reserves independent values for:

```text
InvestigationState
HadvarState
RalofState
MainQuestPath
```

`WoundedInCave` and `CapturedInKeep` projections are implemented. `Freed` and
`Departed` remain reserved logical states without physical projections in this
increment.

Hadvar and Ralof use Specific Reference aliases with `Allow Reserved` because
their vanilla actor references are already reserved elsewhere in the active
quest graph. The investigation quest also owns aliases for the two
STRE wounded-position markers.

The current wounded projection is:

```text
logical survivor state = WoundedInCave
→ MoveTo STRE wounded anchor
→ EvaluatePackage()
→ dedicated SitTarget package
→ vanilla wounded furniture marker
```

Hadvar uses `STRE_PACK_HadvarWounded` with
`STRE_HelgenHadvarWoundedFurniture`; Ralof uses
`STRE_PACK_RalofWounded` with `STRE_HelgenRalofWoundedFurniture`. The packages
own the local wounded posture; the XMarkerHeading references remain logical
position anchors.

The existing collapsed Keep rubble remains intact. No collision or navmesh edit
was introduced. A small visible opening is exposed as a bidirectional local
interaction:

```text
STRE_ACTI_HelgenRubbleSqueeze
→ "Se faufiler"
→ STRE_HelgenRubbleSqueezeActivator
→ GetLinkedRef()
→ short fade
→ local player MoveTo
```

Each side links to the destination marker on the opposite side. The interaction
is per-player local traversal and is not shared campaign state.

`STRE_HelgenRubbleExcavatorCorpse` and
`STRE_HelgenRubbleExcavatorPickaxe` explain the opening through environmental
storytelling. The corpse currently uses a vanilla corpse ActorBase whose
respawn behavior still requires a cell-reset regression test.

`STRE_QUEST_HelgenInvestigation` remains explicitly excluded from generic
`QuestService` synchronization. The multiplayer vertical slice does not make
quest stages or Helgen state persistent server authority. Instead, every local
controller signals that `BeginInvestigation()` has run; once the campaign is
`ACTIVE` and the exact sealed roster has signalled, the server broadcasts an
ephemeral collective start authorization. Each client then records its local
Skyrim time, whose calendar is already resynchronized by STR's
`CalendarService`, and owns the relative T+4 timer in its native save.

The cooperative presence gate is also ephemeral. The server evaluates a generic
group spatial `NONE` condition from the existing `CellIdComponent` values and
pushes a reliable cache update after the collective start and every player cell
update. A missing roster member, an unknown cell, a non-`ACTIVE` campaign, or an
unresolved footprint returns no authorization. Papyrus polls only the cache and
never blocks the VM for a network response.

The exact footprint was audited from Bethesda's installed `Skyrim.esm` by
selecting `CELL` records whose `XLCN` is `HelgenLocation [00018A4A]`:

```text
Exterior: 000097ED HelgenExterior04, 000097EE ChargenExit,
          0000980B HelgenExterior,   0000980C HelgenExterior05,
          0000982A HelgenExterior02, 0000982B HelgenExterior06,
          00009849 HelgenExterior03, 0000984A HelgenExterior07
Interior: 00013A66 HelgenTorolfsMill,
          00013A67 HelgenHomestead,
          0005DE24 HelgenKeep01
```

All entries use stable `Skyrim.esm` plus local FormID identities. Exact cells
cover the location contract, so no approximate exterior radius or grid range is
used.

The v1 post-deadline behavior is now fixed at the product level:

- before four full Skyrim days have elapsed since the investigation starts,
  Helgen remains in the recent-post-attack projection and no occupation bandits
  are introduced;
- when the four-day deadline is reached, the bandit-occupied projection is
  applied only if no campaign player is inside the affected Helgen footprint;
- if at least one player is still inside that footprint at the deadline, the
  physical transition is deferred unchanged: no occupation bandits appear and
  the current survivor projection remains in place;
- when the last player leaves the affected footprint, the deferred transition
  may commit directly to the bandit-occupied Helgen projection;
- at that commit boundary, each survivor still in `WoundedInCave` transitions
  independently to `CapturedInKeep`; survivors already `Freed` or `Departed`
  never regress to captivity.

The standalone v1 path is implemented with `HelgenLocation [00018A4A]`
as the local presence predicate. `STRE_HelgenInvestigationController` stores
`InvestigationStartGameTime`, arms the relative four-day deadline through
`RegisterForSingleUpdateGameTime`, and refuses local campaign authority whenever
`SkyrimTogetherUtils.IsConnected()` reports an active STR connection. If the
standalone player is still inside `HelgenLocation` at the deadline, the
controller enters `BanditOccupationPending` and rechecks presence every five
real-time seconds until the location is clear. While connected, the same
controller instead waits for the collective start cache, keeps T+4 local, and
commits only when the server's full-roster outside-Helgen cache is known and
true. Once a campaign has been observed, disconnect cannot reactivate the solo
authority path.

`HelgenWorldPhase` is projected locally as:

```text
0 = RecentPostAttack
1 = BanditOccupationPending
2 = BanditOccupied
```

The `BanditOccupied` projection reuses Bethesda's complete late post-Helgen
phase instead of creating duplicate STRE bandits:

```text
Disable dunCGPostMajorFXMarker            [000F829B]
Enable  PostHelgenEncountersMarker        [000F8240]
Keep    MQ101CollapsingBridgeAnimRef       [000C8960]
Keep    dunCGKeepBridgeDebrisMarker        [0010AB26]
Disable STRE squeeze activator entrance    [local 0x000677C9]
Disable STRE squeeze activator survivor    [local 0x00081CD2]
```

At the same commit boundary, only survivors still in `WoundedInCave` transition
to `CapturedInKeep`. The capture projection uses STRE-owned jail markers and
conditional Sandbox packages:

```text
STRE_HelgenHadvarCapturedMarker  local 0x00096451
STRE_HelgenRalofCapturedMarker   local 0x00096452
STRE_PACK_HadvarCaptured         local 0x00096453
STRE_PACK_RalofCaptured          local 0x00096454
```

The selected vanilla jail doors are referenced through quest aliases. The CK
promotes those exact references for the bindings, so their master overrides are
explicitly allowlisted by signature and FormID:

```text
Hadvar jail door [00091583]
Ralof jail door  [00091587]
```

`CapturedInKeep` closes and locks the appropriate door, moves the survivor to
the captured marker, and re-evaluates the actor package. Reprojection is
idempotent for the implemented states. `Freed` and `Departed` never regress at
the occupation commit, but their physical projections and liberation gameplay
are not implemented yet.

Not implemented yet:

- two-client runtime validation of the implemented collective start and
  all-roster Helgen-presence gate. The cases where A exits while B remains,
  the inverse order, both are already outside at T+4, and one member remains in
  `HelgenKeep01` remain pending. The merged #71 gameplay bootstrap is now the
  supported way to create, join, and start the sealed campaign for this
  two-PC matrix. No Helgen runtime pass is claimed yet;
- general coordinated checkpoint/recovery infrastructure; Helgen intentionally
  relies on native saves and adds no dedicated persistent adapter state;
- rescue/liberation interaction and physical `Freed`/`Departed` projections;
- Valen-driven quest start;
- neutral/Hadvar/Ralof MQ102 continuity.

## Deferred post-v1 Helgen occupation encounter

A more immersive occupation sequence is deliberately deferred beyond v1. One
possible later enhancement is to play the bandit takeover in real time when one
or more players are present at the four-day boundary: bandits would approach
Helgen, secure the ruins, enter the Keep, and progressively reach any unsaved
survivors. An off-screen fast-forward path would still resolve the same
canonical result when nobody is present.

This is a design candidate only. It is not part of the v1 acceptance criteria,
must not complicate the simple occupancy-deferred transition above, and would
require a separate CK/navmesh/AI and multiplayer-authority design pass before
implementation.

## Navmesh

The current `STRE_CELL_AlternateStart` geometry has an implemented interior
navmesh. A temporary vanilla NPC successfully navigated normal circulation,
obstacles, stairs, and passages in game; the test reference was removed
afterward. Architecture, furniture, or door changes that affect traversal must
be followed by the necessary navmesh update and another NPC navigation test.

## Remaining implementation

- two-client runtime validation for both last-exit orders, the direct
  already-outside-at-T+4 path, and `HelgenKeep01`, using the merged #71
  gameplay bootstrap;
- coordinated campaign checkpoint/recovery validation for the native local
  Helgen projection;
- rescue/liberation and the remaining survivor lifecycle projections;
- neutral MQ102/MQ103 vanilla-continuity handoff and its Riverwood/Alduin/Civil
  War semantics;
- Hadvar/Ralof branch commit without making rescue itself a faction choice;
- final Valen AI, FaceGen/dialogue head, scenes, dialogue, and aliases;
- real Departure/exit flow and main-quest resumption;
- markers and placements for more players;
- automated Papyrus compilation.

## Audits

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

Reports under `_audit/*.tsv` and logs are generated locally and must not be committed.

Ilinalta's Vigil navmesh checkpoint validated on 3 September 2026:

- the Creation Kit interior navmesh was implemented for
  `STRE_CELL_AlternateStart`;
- a temporary vanilla NPC completed in-game pathing checks for normal
  circulation, obstacle avoidance, stairs, and passages, then was removed;
- the explicit CK-to-repository import changed only
  `GameFiles/Skyrim/STRE_AlternateStart.esp`;
- the strict plugin audit remained conforming with 78 expected STRE-owned
  records and no unexpected master override; no new expected `NAVM` manifest
  entry was required;
- `build-and-deploy-dev.ps1` completed successfully;
- the post-deployment runtime smoke test passed entry, normal traversal, the
  interior/exterior load-door transition, stairs and passages, collision and
  pathing, while preserving the existing fireplace and lighting presentation.

Room Bounds and Portals were not implemented in this checkpoint. Their use is
conditional on a demonstrated profiling or runtime need, and this evidence does
not imply that issue #23 or #24 is complete.

## New Game acceptance

Validated on 15 August 2026:

- New Game enters the inn without running the Helgen cart sequence;
- during the character-creation bootstrap, MQ101 remains at stage 0 while
  `STRE_QUEST_AlternateStart` reaches stage 20;
- RaceMenu and Character Creation open normally;
- returning to the main menu and starting a second New Game in the same process works;
- loading an ordinary existing save does not retrigger the bootstrap.

Post-Helgen continuity validated on 16 August 2026 after xEdit Quick Auto Clean:

- MQ101 reaches stage 1000 and stops;
- MQ102, MQ102A, and MQ102B remain untouched;
- skipped Helgen Keep victims are cleaned up;
- Hadvar/Ralof and the residual Imperial guard no longer remain in their skipped
  intro positions;
- Helgen exterior is projected to the destroyed post-attack state;
- Helgen Keep rubble is already collapsed, proximity does not replay the
  collapse, and the vanilla dragon/collapse roar is suppressed;
- the known minor limitation is a brief rubble sound while `HelgenKeep01` loads.

Pre-deadline investigation slice validated on 20 August 2026:

- `STRE_QUEST_HelgenInvestigation` starts successfully with Hadvar, Ralof, and
  both wounded marker aliases filled;
- Hadvar and Ralof move independently to the intended `HelgenKeep01` locations
  and remain in their distinct vanilla wounded poses;
- the `Se faufiler` prompt is available at the rubble opening and traverses the
  blockage in both directions with the expected fade/MoveTo behavior;
- the dead bandit and pickaxe are present without blocking the interaction;
- no navmesh or rubble collision edit is required;
- the strict record audit conforms with 63 expected STRE-owned records and no
  unexpected Skyrim-master override;
- CK packaging audit passes with no compiled PEX under `Scripts/Source`;
- `STRE_QUEST_HelgenInvestigation` is excluded from generic quest-stage
  synchronization, and the client build plus TPTests pass.

Standalone T+4 occupation slice validated on 23 August 2026:

- a clean STRE post-MQ101 baseline shows destroyed/burning Helgen with the
  skipped intro actors removed and no occupation bandits before the deadline;
- after four full game days, remaining inside `HelgenLocation` transitions the
  controller to `BanditOccupationPending` without changing the visible world or
  survivor placement;
- leaving Helgen after the deadline commits `BanditOccupied` on the next
  standalone presence evaluation;
- Bethesda's post-Helgen bandit occupation appears outside and inside the Keep,
  the major post-attack fire/smoke FX and STRE squeeze traversal are removed,
  the collapsed bridge and its debris remain projected, and survivors still in
  `WoundedInCave` move to their locked jail projections;
- the strict CK record audit conforms with 67 expected STRE-owned records and no
  unexpected Skyrim-master override;
- CK packaging passes with 19 managed files and no compiled PEX under
  `Scripts/Source`; client/server builds and TPTests remain green at 1794
  assertions in 130 test cases.

Multiplayer T+4 vertical slice implemented and automated-validated on 23 August
2026:

- `BeginInvestigation()` is not intrinsically collective, so an ephemeral
  all-roster start barrier now establishes the common logical T+4 boundary;
- server calendar resynchronization keeps the local Skyrim calendars aligned;
- the server computes `NONE` over the exact sealed roster and exact Helgen cell
  footprint, then pushes a non-blocking client cache on cell updates;
- the generic spatial evaluator covers one, two, and N members, both exit
  orders, interior/exterior cells, unknown position, incomplete roster, empty
  footprint, and closed campaign gate;
- protocol round trips cover the readiness request and Helgen cache
  notification;
- `SkyrimTogetherClient`, `SkyrimTogetherServer`, and TPTests build; all 1837
  assertions in 137 test cases pass;
- runtime revalidation on 24 August 2026 confirmed the final occupied projection
  in standalone and in a multiplayer campaign: major post-attack FX retire,
  occupation encounters appear, STRE squeeze traversal retires, and the
  collapsed bridge/debris projection remains intact. The additional ordering,
  mixed-state, disconnect, save/load, and cell-reset permutations remain tracked
  in `TEST_PLAN.md` and are not implied by this evidence.

## Local test

```text
resetquest STRE_QUEST_AlternateStart
startquest STRE_QUEST_AlternateStart
setstage STRE_QUEST_AlternateStart 10
```
