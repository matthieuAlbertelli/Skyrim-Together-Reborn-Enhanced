Scriptname STRE_HelgenContinuityController extends Quest

; ============================================================================
; Helgen post-attack references to ENABLE
; ============================================================================

ObjectReference Property MQ101AlduinAttackEnableMarker1 Auto
ObjectReference Property dunCGPostMajorClutterMarker Auto
ObjectReference Property dunCGPostMajorFXMarker Auto
ObjectReference Property dunCGPostAMarker Auto
ObjectReference Property dunCGPostAClutterMarker Auto
ObjectReference Property dunCGPostBMarker Auto
ObjectReference Property dunCGPostBClutterMarker Auto
ObjectReference Property dunCGPostCMarker Auto
ObjectReference Property dunCGPostDMarker Auto
ObjectReference Property dunCGPostEMarker Auto
ObjectReference Property dunCGPostEClutterMarker Auto
ObjectReference Property dunCGOutsideClutterMarker Auto

; ============================================================================
; Helgen pre-attack / chargen references to DISABLE
; ============================================================================

ObjectReference Property dunCGTowerHole01REF Auto

ObjectReference Property dunCGSmokeAMarker Auto
ObjectReference Property dunCGSmokeBMarker Auto
ObjectReference Property dunCGSmokeCMarker Auto
ObjectReference Property dunCGSmokeDMarker Auto
ObjectReference Property dunCGSmokeEMarker Auto

ObjectReference Property dunCGPostTriggerCleanupMarker Auto

ObjectReference Property MQ101SetStage368 Auto
ObjectReference Property MQ101SetStage360 Auto
ObjectReference Property MQ101SetStage267 Auto
ObjectReference Property MQ101SetStage210 Auto
ObjectReference Property MQ101SetStage400 Auto
ObjectReference Property MQ101SetStage485 Auto

ObjectReference Property MQ101BearRef Auto

; Anonymous Skyrim.esm REFR 0010FDE3.
; Assigned manually in CK to the ChargenFxTrigger reference.
ObjectReference Property ChargenFxTriggerRef Auto

; ============================================================================
; Helgen Keep collapse
;
; Skyrim.esm REFR 000BACC8
; EditorID: MQ101KeepCollapseTriggerRef
; Script: HelgenHallCollapseScript
; ============================================================================

HelgenHallCollapseScript Property MQ101Collapse Auto

; ============================================================================
; Runtime diagnostics
; ============================================================================

Int ApplyCallCount = 0

Int DisabledBeforeCount = -1
Int DisabledAfterCount = -1

Int PreAttackDisabledBeforeCount = -1
Int PreAttackDisabledAfterCount = -1

Bool CollapseSoundNeutralized = False
String CollapseStateAfter = ""

Int CollapseVisualApplyCount = 0
Int CollapseVisualWaitIterations = -1
Bool CollapseVisual3DLoaded = False
Bool CollapseVisualApplied = False

Bool LastApplyCompleted = False

; ============================================================================
; Semantic / world-state projection
;
; Called while the player is still outside Helgen.
; ============================================================================

Function ApplyPostAttackProjection()
    ApplyCallCount = ApplyCallCount + 1

    LastApplyCompleted = False

    CollapseSoundNeutralized = False
    CollapseStateAfter = ""

    CollapseVisualApplyCount = 0
    CollapseVisualWaitIterations = -1
    CollapseVisual3DLoaded = False
    CollapseVisualApplied = False

    DisabledBeforeCount = CountDisabledPostAttackRefs()
    PreAttackDisabledBeforeCount = CountDisabledPreAttackRefs()

    ; ========================================================================
    ; Enable Helgen's post-attack world state.
    ; ========================================================================

    MQ101AlduinAttackEnableMarker1.Enable()

    dunCGPostMajorClutterMarker.Enable()
    dunCGPostMajorFXMarker.Enable()

    dunCGPostAMarker.Enable()
    dunCGPostAClutterMarker.Enable()

    dunCGPostBMarker.Enable()
    dunCGPostBClutterMarker.Enable()

    dunCGPostCMarker.Enable()
    dunCGPostDMarker.Enable()

    dunCGPostEMarker.Enable()
    dunCGPostEClutterMarker.Enable()

    dunCGOutsideClutterMarker.Enable()

    ; ========================================================================
    ; Disable Helgen's pre-attack / chargen state.
    ; ========================================================================

    dunCGTowerHole01REF.Disable()

    dunCGSmokeAMarker.Disable()
    dunCGSmokeBMarker.Disable()
    dunCGSmokeCMarker.Disable()
    dunCGSmokeDMarker.Disable()
    dunCGSmokeEMarker.Disable()

    dunCGPostTriggerCleanupMarker.Disable()

    MQ101SetStage368.Disable()
    MQ101SetStage360.Disable()
    MQ101SetStage267.Disable()
    MQ101SetStage210.Disable()
    MQ101SetStage400.Disable()
    MQ101SetStage485.Disable()

    MQ101BearRef.Disable()
    ChargenFxTriggerRef.Disable()

    ; ========================================================================
    ; Helgen Keep collapse semantic state.
    ;
    ; The collapse happened historically before the STRE player arrives.
    ;
    ; Do NOT call TriggerCollapse():
    ; vanilla TriggerCollapse() also causes sound, camera shake, controller
    ; shake, knockback and dust.
    ;
    ; Do NOT try to play PlayAnim02 here:
    ; HelgenKeep01 is normally unloaded at this point, so its 3D animation
    ; state cannot be relied upon.
    ;
    ; The visual state is applied later by ApplyLoadedKeepCollapseVisual()
    ; when HelgenKeep01 actually attaches.
    ; ========================================================================

    If MQ101Collapse
        MQ101Collapse.mysound01 = None
        CollapseSoundNeutralized = True

        ; Prevent vanilla OnTriggerEnter from replaying the historical event.
        MQ101Collapse.GoToState("done")

        CollapseStateAfter = MQ101Collapse.GetState()
    EndIf

    ; ========================================================================
    ; Record resulting world state.
    ; ========================================================================

    DisabledAfterCount = CountDisabledPostAttackRefs()
    PreAttackDisabledAfterCount = CountDisabledPreAttackRefs()

    LastApplyCompleted = True
EndFunction

; ============================================================================
; Loaded-cell visual projection
;
; Called by STRE_HelgenCollapseLoadAlias when HelgenKeep01 attaches.
; ============================================================================

Function ApplyLoadedKeepCollapseVisual()
    CollapseVisualApplyCount = CollapseVisualApplyCount + 1

    CollapseVisualWaitIterations = 0
    CollapseVisual3DLoaded = False
    CollapseVisualApplied = False

    ; Do nothing unless the historical projection has already completed.
    If LastApplyCompleted == False
        Return
    EndIf

    If MQ101Collapse == None
        Return
    EndIf

    ObjectReference collapseFX = MQ101Collapse.ImpFortHallCollapseFX01

    If collapseFX == None
        Return
    EndIf

    ; OnCellAttach may arrive before the object's 3D is ready.
    ; Wait for at most five seconds.
    While (collapseFX.Is3DLoaded() == False) && (CollapseVisualWaitIterations < 50)
        Utility.Wait(0.1)
        CollapseVisualWaitIterations = CollapseVisualWaitIterations + 1
    EndWhile

    CollapseVisual3DLoaded = collapseFX.Is3DLoaded()

    If CollapseVisual3DLoaded
        ; Apply only the visual collapse animation.
        ; No sound, shake, dust or knockback.
        collapseFX.PlayAnimation("PlayAnim02")

        CollapseVisualApplied = True
    EndIf
EndFunction

; ============================================================================
; Diagnostics: post-attack references
;
; Expected after successful projection:
; DisabledAfterCount = 0
; ============================================================================

Int Function CountDisabledPostAttackRefs()
    Int count = 0

    If MQ101AlduinAttackEnableMarker1.IsDisabled()
        count = count + 1
    EndIf

    If dunCGPostMajorClutterMarker.IsDisabled()
        count = count + 1
    EndIf

    If dunCGPostMajorFXMarker.IsDisabled()
        count = count + 1
    EndIf

    If dunCGPostAMarker.IsDisabled()
        count = count + 1
    EndIf

    If dunCGPostAClutterMarker.IsDisabled()
        count = count + 1
    EndIf

    If dunCGPostBMarker.IsDisabled()
        count = count + 1
    EndIf

    If dunCGPostBClutterMarker.IsDisabled()
        count = count + 1
    EndIf

    If dunCGPostCMarker.IsDisabled()
        count = count + 1
    EndIf

    If dunCGPostDMarker.IsDisabled()
        count = count + 1
    EndIf

    If dunCGPostEMarker.IsDisabled()
        count = count + 1
    EndIf

    If dunCGPostEClutterMarker.IsDisabled()
        count = count + 1
    EndIf

    If dunCGOutsideClutterMarker.IsDisabled()
        count = count + 1
    EndIf

    Return count
EndFunction

; ============================================================================
; Diagnostics: pre-attack references
;
; Expected after successful projection:
; PreAttackDisabledAfterCount = 15
; ============================================================================

Int Function CountDisabledPreAttackRefs()
    Int count = 0

    If dunCGTowerHole01REF.IsDisabled()
        count = count + 1
    EndIf

    If dunCGSmokeAMarker.IsDisabled()
        count = count + 1
    EndIf

    If dunCGSmokeBMarker.IsDisabled()
        count = count + 1
    EndIf

    If dunCGSmokeCMarker.IsDisabled()
        count = count + 1
    EndIf

    If dunCGSmokeDMarker.IsDisabled()
        count = count + 1
    EndIf

    If dunCGSmokeEMarker.IsDisabled()
        count = count + 1
    EndIf

    If dunCGPostTriggerCleanupMarker.IsDisabled()
        count = count + 1
    EndIf

    If MQ101SetStage368.IsDisabled()
        count = count + 1
    EndIf

    If MQ101SetStage360.IsDisabled()
        count = count + 1
    EndIf

    If MQ101SetStage267.IsDisabled()
        count = count + 1
    EndIf

    If MQ101SetStage210.IsDisabled()
        count = count + 1
    EndIf

    If MQ101SetStage400.IsDisabled()
        count = count + 1
    EndIf

    If MQ101SetStage485.IsDisabled()
        count = count + 1
    EndIf

    If MQ101BearRef.IsDisabled()
        count = count + 1
    EndIf

    If ChargenFxTriggerRef.IsDisabled()
        count = count + 1
    EndIf

    Return count
EndFunction
