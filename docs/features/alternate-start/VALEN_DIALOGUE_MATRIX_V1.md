# Valen free conversation matrix v1

> Implementation contract and authoring proposal for [#96](https://github.com/matthieuAlbertelli/Skyrim-Together-Reborn-Enhanced/issues/96), under #20 / #19.
> Actual implementation and validation status belongs in [STATUS](../../project/STATUS.md).

## Scope and editorial authority

The whole approved free-conversation corpus is one increment. The collective
introductory scene remains separate. The seven existing #94/#95 records retain
their identities; the shared acceptance and the technical Helgen investigation
remain intact. A general Shared / PlayerLocal / TechnicalProjection taxonomy is
a later feature, not part of this increment.

The sole editable editorial source is
[VALEN_DIALOGUE_MATRIX_V1.json](../../narrative/valen/VALEN_DIALOGUE_MATRIX_V1.json),
validated against its adjacent formal JSON schema. The adjacent Markdown review
and survivor CSV/Markdown are generated views. The corpus preserves all 140
Valen response blocks in section 19 of the approved mission, including the common
recruitment introduction, 21 class variants and fallback. Two existing #95
response blocks are additionally retained from the merged ESP. Existing ASCII
apostrophes in that exchange remain byte-faithful to #95; new frozen French text
keeps its original typographic apostrophes and accents. Paragraphs and all
sentence-boundary segmentation are lossless.

The initial editorial graph contains 131 semantic nodes and 142 response blocks
including the shared recruitment introduction. These are **not record counts**:
multiple semantic variants will share a DIAL, and allocated FormKeys do not yet
exist. Performance directions are provisional metadata, not final voice acting.

```powershell
python Tools/Scripts/valen_matrix.py --check-generated
python Tools/Scripts/valen_matrix.py --for-authoring
python -m unittest discover -s Tools/Scripts -p 'test_valen_matrix.py'
```

The second command must currently fail. An editorial check returning
`success: true, authoringReady: false` is not permission to create a candidate.
The future bounded `AddValenDialogueMatrixV1` adapter must invoke the authoring
gate before even launching the private xEdit writer.

## Maintainer correction: discovery is independent of physical state

`HadvarState` and `RalofState` become `WoundedInCave` in
`STRE_HelgenInvestigationController.BeginInvestigation`, before the player has
met either actor. They cannot serve as discovery flags. No freely selectable
claim of discovery is permitted. Both physical properties remain read-only
inputs; the narrative quest must not maintain competing copies of them.

The proposed minimal discovery representation uses two independently completed
stages on the existing shared Side Quest `STRE_QUEST_ValenHelgen`:

| Fact | Native quest runtime representation | CK condition |
|---|---|---|
| HadvarDiscovered | stage 11 completed | GetStageDone(ValenHelgen, 11) = 1 |
| RalofDiscovered | stage 12 completed | GetStageDone(ValenHelgen, 12) = 1 |
| Arrived in Helgen | stage 20 completed | GetStageDone(ValenHelgen, 20) = 1 |

There are no duplicated Boolean globals and no packed state overwritten by
concurrent writers. Stages 11 and 12 have no journal entries or objective effects.
The script must establish stage 20 before either discovery stage. Skyrim's
`Quest.psc` defines GetStage as the **highest completed stage**, so the displayed
progression remains 20. The client sends `TESQuestStageEvent.stageId`, not merely
the highest current stage. `TESQuest::ScriptSetStage` skips an equal/already-done
stage, not an uncompleted lower-numbered stage. The existing server relay sends
the requested stage to the group without comparing its numerical order.

Consequently the proposed sequences are:

| Discovery order | Requested stages after acceptance | Completed facts | Highest stage |
|---|---|---|---|
| Hadvar then Ralof | 20, 11, 12 | 11 and 12 | 20 |
| Ralof then Hadvar | 20, 12, 11 | 11 and 12 | 20 |
| Concurrent discoveries / duplicate delivery | 20, either ordering of 11/12 | union of both facts | 20 |

This is a source-supported design, not a claim that Papyrus, the network, or save
restoration has passed a runtime test. No change to QuestService or its messages
is proposed. Side Quest type 8 continues to avoid the experimental Misc/None gate.

## Proposed triggers and lifecycle

Add one local-player alias to the narrative quest. A bound alias script handles
OnLocationChange, OnPlayerLoadGame, and bounded single-update polling. Reuse the
technical controller's Hadvar/Ralof actor aliases and physical-state properties;
do not create actor duplicates, change packages, place trigger boxes, or mutate
the technical quest's T0/T+4.

1. Require the narrative quest to be running with stage 10 completed. In Solo,
   permit normal progression. For a connected or previously observed campaign,
   require the existing `IsHelgenInvestigationStartAuthorized()` predicate,
   which includes the runtime recovery/checkpoint lock. Losing connectivity
   must never turn a campaign into standalone authority.
2. At a local player's entry into the bound HelgenLocation, set stage 20 once.
   Its fragment completes objective 10 and displays objective 20, without forcing
   redisplay. Arrival by one roster member is enough; no physical gathering is
   required. Evaluate already-in-location and post-load cases as well.
3. For physical state 1 or 2, discover only an actual local-player observation:
   the bound survivor is enabled, alive, has loaded 3D, is in the same cell,
   within a proposed 256 game units, and visible through `HasLOS`. Require the
   player and survivor to be in the relevant Helgen location. Do not discover
   through a wall, by loading a distant cell, or by reading WoundedInCave.
4. This observation follows the **actor's current projection**, including its
   captured location. It is not restricted to the original cave marker.
5. Once observed, recheck the progression gate and call SetStage(11) or
   SetStage(12) only if that fact is not already complete. Every peer receives
   that normal stage event; another peer need not be physically near the actor.
6. Future rescue/liberation callers must mark the relevant discovery before
   their own success transition. A defensive recheck of the trusted physical
   states `Freed` and `Departed`, separately, establishes a missing discovery
   when those future mechanics become implemented. This increment does not
   implement rescue, movement home, faction choice, or MainQuestPath mutation.
7. Resume polling after load; stop when both discoveries are complete. During
   recovery/checkpoint lock, issue no new discovery or arrival stage. Pending
   proximity is reevaluated after authorized recovery, never replayed from a
   cached pre-load actor pointer.

Completed-stage bits and alias/Papyrus state are stored in each player's native
`.ess`. Recovery remains ADR-0018: the full sealed roster restores its own saves
from the same committed CampaignCheckpoint together with the server snapshot.
The server's ordinary quest-log latest-stage entry cannot reconstruct both
completed-stage bits and must **not** be used as a save/catch-up substitute.
Uncommitted observations after the checkpoint are discarded by recovery, then
may be observed again normally. The model tests do not prove this engine path.

## Dialogue routing and exhaustive conditions

The maintainer explicitly assigned all eight unlabelled state responses to the
single generic player subject:

> Nous devons parler de ce que nous avons trouvé à Helgen.

The player must select this topic. No unsolicited speech is triggered. Keep the
explicit arrival, none-found, individual discovery, both-found and both-rescued
prompts separate. Arrival is available after stage 20 only before any known
discovery. Individual/both discovery reports use local **Say Once** INFO flags;
they do not create shared `Reported` facts and are not permanent state containers.
The both-rescued prompt allows exactly (Freed,Freed), (Freed,Departed),
(Departed,Freed), (Departed,Departed), with both discoveries known.

See the complete
[100-row condition table](../../narrative/valen/VALEN_SURVIVOR_MATRIX_V1.generated.md)
and [machine-readable CSV](../../narrative/valen/VALEN_SURVIVOR_MATRIX_V1.generated.csv).
INFO names there are **semantic targets**, not allocated records. Specificity is
explicit: both departed; one departed; both captured; captured/freed; remaining
single capture; freed/wounded. States 3 and 4 are never merged in the generic
conditions. The validator enumerates all rows and rejects overlap even if a
first-match implementation would conceal it.

Current coverage:

- 20 combinations select one of the eight generic state replies.
- 17 combinations use a separate approved explicit-report/navigation path,
  with no generic state reply. First reports are subject to their local Say Once
  condition; these table rows do not override it.
- 51 combinations are inconsistent or transient (known + Uninitialized, or
  Freed/Departed not yet discovered). They fail closed and must not disclose data.
- **12 physically consistent combinations lack a safe approved response:**
  one discovered survivor is Freed or Departed while the other is undiscovered
  in state 0, 1 or 2. The approved saved/departed replies mention or imply the
  other survivor. No approximate substitution is allowed. The generic subject
  is unavailable, and the authoring gate fails regardless of decision labels.

This last gap requires a maintainer decision on approved wording or explicit
scope. It is not silently deferred and is not presented as a complete matrix.

## Validation required before a real candidate

The implemented checks validate the editorial schema, frozen text fingerprint,
all 21 classes/fallback, Solo/group partitions, stage-label boundaries, graph
reachability/returns, reserved-node exclusion, effects, lossless segmentation,
all 100 survivor combinations, name-disclosure guards and generated concordance.
An optional local approved-source check compares all 140 new responses directly
with the exact mission attachment hash; no Bethesda payload is copied into a
test fixture.

Still required: close the twelve-state gap; finalize CK graph/conditions and
allocation plan; implement and test the class bridge and discovery scripts;
official xEdit fixture authoring; direct PapyrusCompiler compilation; real
private dry-run/prepare; independent binary reread; human xEdit review; validated
import and all post-import audits. Build/deploy and human runtime remain later
explicit gates. There is no candidate session to review yet.

Discovery acceptance must cover both orders, simultaneous observation of
different actors, repeated observation, first discovery only after capture,
same-cell/wall/disabled/dead/distance rejection, defensive Freed/Departed handling,
one peer discovering while the other remains elsewhere, save/reload after each
individual fact, and collective checkpoint recovery before and after either
discovery. Both roster-2 directions use separate clean campaigns. Tests of the
reference completed-stage set establish algebra only; they are not runtime PASS.
