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
- `Alias_PlayerSeat01` through `Alias_PlayerSeat10` (future seating only; not creation anchors)

Quest stages:

- `0` — initialization;
- `10` / alternate `11` — place at the start marker without a posture requirement;
- `20` — trigger Character Creation.

The quest is intentionally excluded from generic quest synchronization.

## Explicit creation markers by PlayerId (2026-09-18)

For this local placement step only, use the current sealed roster's stable
PlayerIds. Sort a copy lexicographically; the authenticated local PlayerId's rank
is creationPositionIndex. CampaignService::GetDurablePlayerIdForAuthentication
supplies that durable identity. Transient transport numbers, roster wire order,
SlotIds and Create/Join order do not select the position.

ResolveCampaignStandingPlacement still requires a sealed CharacterCreation/
ACTIVE/full roster, 1..10 members, unique nonempty PlayerIds and local membership.
It never inspects SlotId validity or uniqueness. Solo uses index 0. Multiplayer
identity errors reject; they never fall back to Solo. This does not alter campaign
admission, protocol validation, SlotId persistence/ownership or recovery semantics.

Index 0..9 maps to the ten distinct CK XMarkerHeading references
STRE_REFR_PlayerCreationMarker01..10. ResolvePluginFormId uses the loaded
STRE_AlternateStart.esp and the local IDs below, never a fixed load-order prefix.
The adapter verifies a live reference, STRE_CELL_AlternateStart (local 0x000012D1),
and finite position/rotation. It copies the marker position and full orientation;
there is no chair-derived offset or yaw. The existing quest aliases remain untouched.

| Index | Marker suffix | Local FormID |
| --- | --- | --- |
| 0 | 01 | 0x000D6B08 |
| 1 | 02 | 0x000D6B09 |
| 2 | 03 | 0x000D6B13 |
| 3 | 04 | 0x000D6B12 |
| 4 | 05 | 0x000D6B0A |
| 5 | 06 | 0x000D6B11 |
| 6 | 07 | 0x000D6B0B |
| 7 | 08 | 0x000D6B10 |
| 8 | 09 | 0x000D6B0D |
| 9 | 10 | 0x000D6B0F |

Expected progression: standing-index-resolved source=sealed-roster-player-id,
then standing-position playerId=... creationPositionIndex=... rosterCount=....
Failures retain standing-position-rejected reason=... with identity, revision,
marker local/reference/current-cell IDs. Missing/duplicate PlayerId
and absent local membership remain distinct errors; see TEST_PLAN.

ADR-0022 removes every sit/sleep eligibility check from entry and LAB. The
native placement validates actor identity, current cell and finite distance from
the target (32-unit tolerance), never posture. Player MoveTo is deferred: issue
it once and observe the actual cell/position on updates for at most five seconds
(monotonic clock). Open RaceMenu only on arrival, after applying target rotation.
No fixed sleep or retry move is used by the native placement. It does not request WantToStand,
force a furniture exit or write ActorState. Keep the updated PSC/PEX pair when
exporting from CK; a seated stage-20 gate would violate this contract.

## Current flow

Posture-independent bootstrap contract, 2026-09-18. Finalization repairs the
tracked PSC drift previously recorded by the diagnostic missions, recompiles the
tracked PEX from that source and requires the bootstrap test to pass. Keep this
pair together when exporting from CK. Both entry fragments call BeginCharacterCreation, resolving
STRE_AlternateStart.esp local ID 0x0001B771 (NewGameStartMarker), never a loaded
FormID. Missing player/marker/cell or a wrong-cell/out-of-range move rejects.
The fixed one-second placement settle is not a posture poll. GetSitState,
furniture activation and posture normalization are absent. ESP is unchanged.

Distinct creation positions are selected after lobby authorization: lexical
rank of stable sealed PlayerIds selects Marker01..10. Solo selects Marker01.
MoveTo uses the marker's cell/coordinates and orientation is applied on arrival.
Physical clearance and visual orientation still require in-game acceptance.
Post-creation collective seating is not implemented by this placement slice.

```text
Main menu: New Game
→ MQQuickstart = 5
→ MQ101 stage 0 selects the STRE branch
→ Player.MoveTo(STRE_REFR_NewGameStartMarker)
→ STRE_QUEST_AlternateStart.Start()
→ Start Up Stage 10
→ BeginCharacterCreation: MoveTo the non-furniture NewGameStartMarker
→ validate the start cell and position without a posture condition
→ advance to stage 20
→ CharacterCreationService locks controls and opens the campaign-bootstrap CEF gate
→ Solo authorizes locally, or canonical sealed CharacterCreation + full-roster ACTIVE authorizes multiplayer
→ assign a distinct standing position from sorted sealed PlayerIds (Solo: first anchor)
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

`STRE_STAT_IlinaltaFireplace01` (local `0xCAF31`) now uses
`EEKs Fireplace Resources\Vanilla Textured\EEK_DragonsReach_Firepit_Kitchen.nif`
as its MODL, replacing the HDEmbers variant. EEK remains an external prerequisite,
installed separately; STRE redistributes no EEK or Embers HD assets. Embers HD
is no longer required by this fireplace path. Credit: EvilEyedKyo / EEK,
[EEKs Resource Repository / EEKs Fireplace Resource](https://www.nexusmods.com/skyrimspecialedition/mods/31562?tab=files).

The exact candidate passed maintainer visual smoke on 2026-09-19: no missing or
purple textures, correct carvings/embers/wood, unchanged apparent size/origin,
collision and circulation. Geometry/transforms/bounds and five collision blocks
match the previous model; two shapes have different UVs and shaders/controllers
differ. This development-environment evidence does not validate a clean install.

The exact original provenance of `textures\eeks whiterun interiors\smim\wrcastlecarvings.dds`
and `wrcastlecarvings_n.dds` has not been independently established. Their runtime
use was accepted by the maintainer for `0.4.0-alpha.1` after successful visual
validation. STRE does not redistribute these files; this acceptance is not
evidence of standalone redistribution permission. A tagged-package clean-install
smoke must still verify availability of all external textures.

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

## Individual Applied seating (ADR-0024, 2026-09-18)

Applied is individual: after authoritative build completion, local finalization
unlocks controls and stops the creation quest, then requests SeatXX immediately
on the game update. Solo uses Seat01. No other player's Applied is required.
An observer keeps an intention pending until the matching final revision has
committed a current remote native binding (WaitingFor3D/assignment still reject).
No change is made to the natural-join materializer or its transaction.

The same durable PlayerId rank selects the existing furniture references:
0=0x000BF3DD, 1=0x000BF3DC, 2=0x000C516E, 3=0x000C516F,
4=0x000C5173, 5=0x000C5171, 6=0x000C5174,
7=0x000C5172, 8=0x000C5176, 9=0x000C5178.
All values are plugin-local to STRE_AlternateStart.esp.
The manifest and CreationSeatLocalFormId table are tested for exact concordance.

Native boundary: TESObjectREFR::Activate calls RealActivate under the existing
ScopedActivateOverride, as used by STR ObjectService for remote activation.
This avoids generating another ActivateRequest. There is no supported dedicated
Sit adapter in this repository. Before activation, the registered vanilla
ObjectReference.IsFurnitureInUse(false) includes reservations; Actor.GetSitState
and occupiedFurniture identify the correct occupant. The occupiedFurniture
handle is exposed at MiddleHighProcessData offset 0x208, with a static assertion,
following CommonLibSSE-NG include/RE/M/MiddleHighProcessData.h and
src/RE/A/AIProcess.cpp:
https://github.com/CharmedBaryon/CommonLibSSE-NG/blob/main/include/RE/M/MiddleHighProcessData.h
https://github.com/CharmedBaryon/CommonLibSSE-NG/blob/main/src/RE/A/AIProcess.cpp

Actor/cell/seat, root, current binding, process, exact runtime and registered
Papyrus functions must be available. Dead/disabled/combat/mounted or unsafe
life/knock/attack states wait. Another occupant/reservation is never ejected.
There is one activation per Actor FormID/token; a pending animation is observed,
not repeatedly activated. After ten seconds, log pending-entry-timeout-no-reactivation.
A rejected activation is not retried on that same token. Confirmed seating is
terminal for that token, so voluntary later movement does not force reseating.
A new committed Actor token permits a fresh projection.

No PSC/PEX/ESP change is needed. Posture queries exist only here, after completion.
Collective sequencing, Valen, ready/departure and furniture lifetime are out of scope.
Runtime acceptance for the current roster-2 marker/replay flow is recorded in STATUS; missing native registrations still fail closed.

### Local approach using existing MarkerXX references (ADR-0025)

Non-MASTER uses the existing rank pair MarkerXX / SeatXX for the native local
PlayerCharacter, offline or connected. No new SeatApproach reference is needed.
The complete reference EditorIDs are STRE_REFR_PlayerCreationMarker01..10;
these remain the initial creation markers as well. Repositioning a marker in
CK changes both initial creation placement and the final seating approach.

| Rank | Marker local ID | Seat local ID |
| --- | --- | --- |
| 0 | D6B08 | BF3DD |
| 1 | D6B09 | BF3DC |
| 2 | D6B13 | C516E |
| 3 | D6B12 | C516F |
| 4 | D6B0A | C5173 |
| 5 | D6B11 | C5171 |
| 6 | D6B0B | C5174 |
| 7 | D6B10 | C5172 |
| 8 | D6B0D | C5176 |
| 9 | D6B0F | C5178 |

All IDs are local to STRE_AlternateStart.esp, resolved through the loaded plugin.
There is one shared pipeline for ranks 0..9, without furniture-derived offsets,
heading calculations, table/neighbor models or hard-coded target coordinates.
The old Seat01/Seat02 geometric profiles are retired, not alternative fallbacks.

#### Accepted layout checkpoint (2026-09-19)

The maintainer accepts local and remote sitting for the two-player slice in both
completion orders; STATUS owns the runtime evidence and its limits. This does
not validate physical approach geometry for ranks 2..9. The closure audit finds
ten existing XMarkerHeading references and ten seats with their exact C++ pair
mapping. ESP is byte-identical to HEAD: no reference recreation, FormID change,
quest-alias change, new master override or new SeatApproachXX reference is part
of this checkpoint. PSC/PEX remain the tracked placement-only bootstrap pair;
no Papyrus rebuild or CK session is needed. Keep the authoring procedure below
for later deliberate layout changes, not as an outstanding task for this slice.

#### CK authoring procedure

1. Preserve the current plugin and its existing changes. Edit the references
   listed above in STRE_CELL_AlternateStart. Do not duplicate/recreate them or
   change their IDs, names, XMarkerHeading base, persistent flag or seat pairing.
2. For each MarkerXX, choose a valid entry location for its own SeatXX manually.
   Keep it on usable floor, clear of furniture/obstacles, enabled, scale 1 and
   upright (X/Y rotation zero). Orient its heading for the native chair entry.
   Do not move it to the chair's raw origin. C++ does not repair a bad CK layout.
3. Check that this location also works as that player's initial creation position:
   distinct markers, adequate physical room, no forced sitting during creation.
   There is no fixed 90-unit spacing contract between chair approaches.
4. Save the ESP without regenerating quest fragments or changing PSC/PEX. No new
   reference, quest alias, property, script or network field is needed.
5. Run from the repository root:
   `python -B Tools/Scripts/audit_seat_approach_markers.py`.
   It reads the ESP, checks the ten record types/bases/flags/cells/transforms and
   both C++ ID tables, and reports transforms and the plugin hash. It writes
   nothing and does not prove physical clearance, pathing or rendered pose.
6. Follow TEST_PLAN for hands-off Solo and B-local acceptance; record the exact
   built executable and edited plugin hashes. Do not reuse old geometry PASS
   evidence as proof for newly positioned markers.

#### Native pipeline and diagnostics

Authoritative individual Applied, local finalization and the existing durable
identity/session/recovery gates remain prerequisites. No other Applied is needed.
Only PlayerCharacter::Get(), confirmed as registry Actor 0x14 and non-remote,
enters PrepareLocalApproach. Observers use STR animation/binding replay; never
Activate or MoveTo a remote actor. The local approach is not promoted to MASTER.

Reserve one MoveTo to marker.cell / exact marker.position. On a later update,
prove same actual cell and distance <=2 units, apply marker.rotation, then wait
for another UpdateEvent. Recheck arrival and orientation (0.05-radian tolerance)
before the unchanged single SeatXX Activate. The total preparation deadline is
5 seconds. Repeated Tick calls inside one UpdateEvent cannot satisfy the barrier.
No sleep, retry move, fallback Activate, ActorState write or forced animation.

Missing marker, wrong base (not Skyrim XMarkerHeading), disabled reference,
wrong cell, nonfinite/tilted transform or identity/transform change fails closed
with an explicit approach-* reason. Seat occupancy including reservations and
existing actor/root/process safety gates still precede each advance. Invisible
XMarkerHeading references do not need a rendered NiNode.

Logs use [STRE][CreationSeating][LocalProbe]: prototype-triggered or
prototype-rejected, approach-target source=ck-marker, before-approach-move,
after-approach-move-request, approach-arrived-before-facing,
approach-facing-applied, approach-await-next-engine-update,
approach-ready-after-engine-update, then activation-issued. Context includes
sessionConnected, localPlayer, durablePlayerId, creationPositionIndex, assigned
Seat and Marker names/IDs; approach-target includes actorRemote, exact position,
rotation and cell. Native event callbacks share immutable diagnostic context.

The passive capture still covers Activate +40 s or first Enter +5 s, whichever
ends later, and snapshots Furniture Enter immediately. Engine entry evidence
requires the matching event/root/graph and valid true furniture/sitting reads;
camera or logical furniture state alone is insufficient. Rendered pose still
requires human confirmation. Historical Seat01 prototype evidence is in STATUS.

#### Earlier observation-only implementation

Historical diagnostic rationale follows; current placement/activation behavior
is the MarkerXX pipeline above, and current validation is recorded in STATUS.

The Solo runtime reported on 0e421a99 has logical furniture state with a standing
pose. Manual activation of the same chair works (maintainer report). The old
`seated` phase therefore is not an acceptance signal.

The Solo observer now requires a matching furniture-enter event, current root
and graph, correct occupied furniture/GetSitState, and successful true reads of
`isInFurniture` and `isIdleSitting` before completing as
`entry-animation-observed`. Unknown graph reads are logged as -1 and remain
pending. These are engine observations; visible pose and camera behavior still
require human confirmation. No graph variable is written and no animation is
forced. The multiplayer native projection is not changed by this diagnostic;
its log is `furniture-state-confirmed`, with no claim of visual confirmation.

The 22:12:56 Solo run demonstrated Enter at +30.750 seconds, after the initial
diagnostic had stopped. Diagnostics now sample every 500 ms through the later
of Activate + 40 seconds and the first matching Enter + 5 seconds. A first Enter
after the initial window reopens five seconds of capture. Repeated Enter/Exit
events cannot extend it indefinitely. The existing ten-second request-status
timeout remains a status log, never a sampling stop or a reactivation trigger.
Sampling precedes readiness/unsafe-state/completion exits, so completion does
not hide the five seconds after Enter. Recovery/session/runtime fences remain.

The first matching Enter always logs an immediate synchronous, sequential
snapshot in the event callback: occupied furniture, registered GetSitState,
graph values, root, ActorState, transforms and process. This is the callback
entry observation, not an atomic engine snapshot or a deferred next-tick read.
Other furniture event snapshots are bounded to eight per attempt.
Actions have a renewable budget of 32 logs per second, reset at first Enter;
omitted actions are counted in process snapshots. Early movement spam cannot
consume a lifetime budget and hide later actions. The window ends with an
observation-window-ended snapshot, independent of whether the player is seated.
Action logs include target, action/idle IDs, selected event, result, action flags
and whether the existing remote-action guard blocked it. Native event callbacks
use atomic actor/seat tokens, never the intention map or retained engine pointers.
Fresh creation/disconnect clears those tokens. Polling does not sleep, retry
activation, teleport, eject an occupant or unblock the camera by force.

Process snapshots read the current and run-once package IDs/tokens, effective
package source, data token, target handle/resolved FormID, procedure index/start
time, raw procedure type, process level, furniture idle and raw furniture path
point. The added members replace padding with offset assertions, preserving
existing layout. Layout/provenance: [AIProcess](https://raw.githubusercontent.com/CharmedBaryon/CommonLibSSE-NG/main/include/RE/A/AIProcess.h),
[ActorPackage](https://raw.githubusercontent.com/CharmedBaryon/CommonLibSSE-NG/main/include/RE/A/ActorPackage.h),
[MiddleHighProcessData](https://raw.githubusercontent.com/CharmedBaryon/CommonLibSSE-NG/main/include/RE/M/MiddleHighProcessData.h),
and [GetRunningPackage](https://raw.githubusercontent.com/CharmedBaryon/CommonLibSSE-NG/main/src/RE/A/AIProcess.cpp).
Run-once takes precedence over current package for the effective observation.
These fields do not expose the navmesh solver or establish whether the raw path
point is an active world-space destination. Logs explicitly say
pathSolverStatus=unavailable and pathPointSpace=unverified.

Actor/seat distance and transforms are logged alongside PlayerControls movement,
look vectors, auto-move/block/handler state. These values are engine observations,
not raw physical-input provenance. Turn/move action events alone cannot establish
human intervention. The next Solo acceptance must be strictly hands-off for at
least 40 seconds and through Enter + 5 seconds. No native behavior was changed.

Static native audit on 1.6.1170: `ActivateRef` AE19796/RVA 0x2EAC20 calls
TESFurniture's virtual activation at RVA 0x269C00; its ordinary player branch
calls RVA 0x736B90/AE40486 and installs a native package. The Papyrus
ObjectReference.Activate callback at RVA 0xA2BC40 calls AE19796 with the same
0/null/1/defaultProcessing arguments as this adapter. Directly replacing the
adapter with the nested entry call is not a demonstrated fix. None of these
additional addresses is called or hooked by the follow-up. The observed
package/animation divergence still needs the new Solo trace.

### Remote seating correction of primitive (2026-09-19)

[ADR-0026](../../architecture/ADRs/ADR-0026-remote-seating-through-str-actions.md)
supersedes the observer activation described above. TESFurniture::Activate on
1.6.1170 rejects any activator other than the native PlayerCharacter singleton.
CreationSeating now exits the remote path before occupancy handling, local
approach or activation, observing only the final binding and the existing STR
network action projection. The local MarkerXX -> SeatXX pipeline is unchanged.
No CK, marker, chair, PSC, PEX or ESP change is needed for this diagnostic.

Do not repair the observer by calling Activate repeatedly, moving a remote,
spoofing PlayerCharacter, writing ActorState or forcing a chair animation. The
actual action receipt/application relative to candidate-commit must first be
observed. TEST_PLAN owns the observer-only collection procedure; STATUS owns the
evidence. The subsequent ADR-0027 replay checkpoint is accepted for reciprocal
roster-2 visible sitting; this earlier diagnostic alone did not establish it.
