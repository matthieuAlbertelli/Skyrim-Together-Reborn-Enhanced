# Alternate Start — Creation Kit implementation

> **Status: New Game/CK bootstrap, MQ101/post-Helgen projection, the wounded-survivor investigation slice, and standalone T+4 bandit occupation/capture are runtime-tested; the native campaign gate is automated-tested and manually validated on the Solo/two-player happy path; multiplayer Helgen validation, rescue/liberation, negative runtime coverage, MQ102/MQ103 handoff, introduction, and Departure remain**

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

- `STRE_CELL_AlternateStart`
- `STRE_CELL_DevSandbox`
- `STRE_QUEST_AlternateStart`
- `STRE_FURN_PlayerSeat01`
- `STRE_FURN_PlayerSeat02`
- `STRE_REFR_NewGameStartMarker`

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
- three anonymous approved master-backed records validated by exact signature
  and FormID: `NAVI [00012FB4]`, `CELL [00000D74]`, and `REFR [0010FDE3]`.

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

## M7 records and continuity helper

The `CK_RECORDS_M7_IMPLEMENTED.json` manifest covers 67 expected STRE-owned records:

- cells, quests, and seat references;
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
Skyrim-master records listed in its allowlists. Any additional master-backed
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

`STRE_QUEST_HelgenInvestigation` is explicitly excluded from generic
`QuestService` synchronization. Future multiplayer integration must synchronize
canonical investigation/survivor/world-phase facts through the dedicated STRE
campaign boundary and locally project them into CK/Papyrus.

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

The standalone v1 path is now implemented with `HelgenLocation [00018A4A]`
as the local presence predicate. `STRE_HelgenInvestigationController` stores
`InvestigationStartGameTime`, arms the relative four-day deadline through
`RegisterForSingleUpdateGameTime`, and refuses local campaign authority whenever
`SkyrimTogetherUtils.IsConnected()` reports an active STR connection. If the
standalone player is still inside `HelgenLocation` at the deadline, the
controller enters `BanditOccupationPending` and rechecks presence every five
real-time seconds until the location is clear.

`HelgenWorldPhase` is projected locally as:

```text
0 = RecentPostAttack
1 = BanditOccupationPending
2 = BanditOccupied
```

The `BanditOccupied` projection reuses Bethesda's complete late post-Helgen
phase instead of creating duplicate STRE bandits:

```text
Enable  PostHelgenEncountersMarker       [000F8240]
Disable MQ101CollapsingBridgeAnimRef      [000C8960]
Disable dunCGKeepBridgeDebrisMarker       [0010AB26]
Disable STRE squeeze activator entrance   [local 0x000677C9]
Disable STRE squeeze activator survivor   [local 0x00081CD2]
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

The selected vanilla jail doors are referenced through quest aliases rather than
overridden:

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

- server-authoritative multiplayer ownership of the four-day deadline and
  all-roster Helgen-presence predicate;
- transport/snapshot/recovery of `HelgenWorldPhase` and survivor state through
  the dedicated campaign adapter;
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

The cell contains several navmesh fragments. Avoid relying on complex NPC pathfinding until the cell has received a complete CK audit. Every furniture or door change must be followed by a navigation test.

## Remaining implementation

- server-authoritative four-day deadline and all-roster Helgen presence gate for
  multiplayer, using the dedicated campaign-state boundary rather than CK quest
  stages;
- multiplayer projection/snapshot/recovery for the implemented Helgen world and
  survivor states;
- rescue/liberation and the remaining survivor lifecycle projections;
- neutral MQ102/MQ103 vanilla-continuity handoff and its Riverwood/Alduin/Civil
  War semantics;
- Hadvar/Ralof branch commit without making rescue itself a faction choice;
- Valen, scenes, dialogue, and aliases;
- real Departure/exit flow and main-quest resumption;
- markers and placements for more players;
- dedicated campaign adapter for authoritative shared investigation state;
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
  the pre-occupation bridge/debris state and STRE squeeze traversal are removed,
  and survivors still in `WoundedInCave` move to their locked jail projections;
- the strict CK record audit conforms with 67 expected STRE-owned records and no
  unexpected Skyrim-master override;
- CK packaging passes with 17 managed files and no compiled PEX under
  `Scripts/Source`; client build and TPTests remain green at 1511 assertions in
  112 test cases.

## Local test

```text
resetquest STRE_QUEST_AlternateStart
startquest STRE_QUEST_AlternateStart
setstage STRE_QUEST_AlternateStart 10
```
