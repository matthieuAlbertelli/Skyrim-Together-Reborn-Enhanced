Scriptname STRE_HelgenInvestigationController extends Quest Conditional

; ============================================================================
; Quest aliases
; ============================================================================

ReferenceAlias Property Hadvar Auto
ReferenceAlias Property Ralof Auto

ReferenceAlias Property HadvarWoundedMarker Auto
ReferenceAlias Property RalofWoundedMarker Auto

ReferenceAlias Property HadvarCapturedMarker Auto
ReferenceAlias Property RalofCapturedMarker Auto

ReferenceAlias Property HadvarCapturedDoor Auto
ReferenceAlias Property RalofCapturedDoor Auto

ReferenceAlias Property PostHelgenEncountersMarker Auto
ReferenceAlias Property PostHelgenBridge Auto
ReferenceAlias Property PostHelgenBridgeDebris Auto
ReferenceAlias Property RubbleSqueezeEntranceSide Auto
ReferenceAlias Property RubbleSqueezeSurvivorSide Auto

Location Property HelgenLocation Auto

; ============================================================================
; Persistent investigation state
;
; InvestigationState:
;   0 = NotOffered
;   1 = Active
;   2 = Resolved
;
; Survivor state:
;   0 = Uninitialized
;   1 = WoundedInCave
;   2 = CapturedInKeep
;   3 = Freed
;   4 = Departed
;
; HelgenWorldPhase:
;   0 = RecentPostAttack
;   1 = BanditOccupationPending
;   2 = BanditOccupied
;
; MainQuestPath:
;   0 = Undecided
;   1 = Hadvar
;   2 = Ralof
;   3 = Neutral
; ============================================================================

Int InvestigationState = 0
Float InvestigationStartGameTime = -1.0
Bool MultiplayerCampaignObserved = False
Bool MultiplayerDeadlineArmed = False

Int Property HadvarState = 0 Auto Conditional
Int Property RalofState = 0 Auto Conditional

Int HelgenWorldPhase = 0
Int MainQuestPath = 0

; ============================================================================
; Investigation bootstrap
; ============================================================================

Function BeginInvestigation()

    If InvestigationState == 0
        InvestigationState = 1
        InvestigationStartGameTime = Utility.GetCurrentGameTime()

        Debug.Trace("[STRE][HelgenInvestigation] Investigation started at game time " + InvestigationStartGameTime)
    Else
        Debug.Trace("[STRE][HelgenInvestigation] BeginInvestigation: already initialized, no state reset")
    EndIf

    If HadvarState == 0
        HadvarState = 1
    EndIf

    If RalofState == 0
        RalofState = 1
    EndIf

    ProjectHadvarState()
    ProjectRalofState()

    ; Standalone uses the local start immediately. Connected campaigns first
    ; wait for the exact sealed roster to cross the ephemeral start barrier.
    ArmStandaloneBanditOccupationDeadline()

EndFunction

; ============================================================================
; Standalone and connected T+4 scheduling
; ============================================================================

Function ArmStandaloneBanditOccupationDeadline()

    If InvestigationState != 1 || HelgenWorldPhase == 2
        Return
    EndIf

    If InvestigationStartGameTime < 0.0
        Debug.Trace("[STRE][HelgenInvestigation] ERROR: cannot arm T+4 without InvestigationStartGameTime")
        Return
    EndIf

    If SkyrimTogetherUtils.IsConnected()
        ArmConnectedBanditOccupationDeadline()
        Return
    EndIf

    Float deadlineGameTime = InvestigationStartGameTime + 4.0
    Float remainingDays = deadlineGameTime - Utility.GetCurrentGameTime()

    UnregisterForUpdateGameTime()

    If remainingDays <= 0.0
        EvaluateStandaloneBanditOccupationDeadline()
    Else
        RegisterForSingleUpdateGameTime(remainingDays * 24.0)
        Debug.Trace("[STRE][HelgenInvestigation] Standalone T+4 armed in " + (remainingDays * 24.0) + " game hours")
    EndIf

EndFunction

Function ArmConnectedBanditOccupationDeadline()

    If InvestigationState != 1 || HelgenWorldPhase == 2
        Return
    EndIf

    UnregisterForUpdateGameTime()
    If SkyrimTogetherUtils.SignalHelgenInvestigationReady()
        MultiplayerCampaignObserved = True
    EndIf
    UnregisterForUpdate()
    RegisterForSingleUpdate(1.0)

EndFunction

Event OnUpdateGameTime()

    ; A timer may have been armed before the player connected. Never let that
    ; stale local registration become campaign authority while online.
    If SkyrimTogetherUtils.IsConnected()
        Debug.Trace("[STRE][HelgenInvestigation] Switching stale standalone timer to connected campaign gate")
        ArmConnectedBanditOccupationDeadline()
        Return
    EndIf

    EvaluateStandaloneBanditOccupationDeadline()

EndEvent

Function EvaluateStandaloneBanditOccupationDeadline()

    If InvestigationState != 1 || HelgenWorldPhase == 2
        Return
    EndIf

    If SkyrimTogetherUtils.IsConnected()
        Return
    EndIf

    Float deadlineGameTime = InvestigationStartGameTime + 4.0
    Float remainingDays = deadlineGameTime - Utility.GetCurrentGameTime()

    ; Defensive against early/rounded OnUpdateGameTime delivery.
    If remainingDays > 0.0
        UnregisterForUpdateGameTime()
        RegisterForSingleUpdateGameTime(remainingDays * 24.0)
        Return
    EndIf

    If IsLocalPlayerInHelgen()
        MarkBanditOccupationPending()
        ArmStandalonePendingPresenceCheck()
    Else
        CommitBanditOccupation()
    EndIf

EndFunction

Bool Function IsLocalPlayerInHelgen()

    If HelgenLocation == None
        ; Fail closed: never mutate Helgen off an unconfigured presence gate.
        Debug.Trace("[STRE][HelgenInvestigation] ERROR: HelgenLocation property is empty; treating Helgen as occupied")
        Return True
    EndIf

    Actor playerRef = Game.GetPlayer()

    If playerRef == None
        Debug.Trace("[STRE][HelgenInvestigation] ERROR: player unavailable; treating Helgen as occupied")
        Return True
    EndIf

    Return playerRef.IsInLocation(HelgenLocation)

EndFunction

Function ArmStandalonePendingPresenceCheck()

    UnregisterForUpdate()
    RegisterForSingleUpdate(5.0)

EndFunction

Event OnUpdate()

    If InvestigationState != 1 || HelgenWorldPhase == 2
        Return
    EndIf

    If SkyrimTogetherUtils.IsConnected()
        EvaluateConnectedBanditOccupationDeadline()
        Return
    EndIf

    ; A campaign save cannot become standalone authority during a disconnect.
    ; Campaign v1 fences progression until collective checkpoint recovery.
    If MultiplayerCampaignObserved
        ArmStandalonePendingPresenceCheck()
        Return
    EndIf

    If HelgenWorldPhase != 1
        Return
    EndIf

    If IsLocalPlayerInHelgen()
        ArmStandalonePendingPresenceCheck()
    Else
        CommitBanditOccupation()
    EndIf

EndEvent

Function EvaluateConnectedBanditOccupationDeadline()

    If SkyrimTogetherUtils.SignalHelgenInvestigationReady()
        MultiplayerCampaignObserved = True
    EndIf

    If !SkyrimTogetherUtils.IsHelgenInvestigationStartAuthorized()
        ArmStandalonePendingPresenceCheck()
        Return
    EndIf

    If !MultiplayerDeadlineArmed
        InvestigationStartGameTime = Utility.GetCurrentGameTime()
        MultiplayerDeadlineArmed = True
        Debug.Trace("[STRE][HelgenInvestigation] Collective investigation T+4 start armed at game time " + InvestigationStartGameTime)
    EndIf

    Float remainingDays = (InvestigationStartGameTime + 4.0) - Utility.GetCurrentGameTime()

    If remainingDays > 0.0
        ArmStandalonePendingPresenceCheck()
        Return
    EndIf

    Bool allRequiredPlayersOutside = SkyrimTogetherUtils.AreAllRequiredPlayersOutsideHelgen()

    If HelgenWorldPhase == 0
        Debug.Trace("[STRE][HelgenInvestigation] Collective T+4 reached; all required players outside = " + allRequiredPlayersOutside)
    EndIf

    If allRequiredPlayersOutside
        CommitBanditOccupation()
    Else
        MarkBanditOccupationPending()
        ArmStandalonePendingPresenceCheck()
    EndIf

EndFunction

; ============================================================================
; Bandit occupation state transition
;
; In multiplayer each client calls these transitions locally only after the
; cached server predicate authorizes them. Standalone uses the local T+4 gate.
; ============================================================================

Function MarkBanditOccupationPending()

    If HelgenWorldPhase == 0
        HelgenWorldPhase = 1
        Debug.Trace("[STRE][HelgenInvestigation] Helgen world phase: BanditOccupationPending")
    Else
        Debug.Trace("[STRE][HelgenInvestigation] MarkBanditOccupationPending ignored from phase " + HelgenWorldPhase)
    EndIf

EndFunction

Function CommitBanditOccupation()

    If HelgenWorldPhase == 2
        Debug.Trace("[STRE][HelgenInvestigation] CommitBanditOccupation: already occupied, re-projecting")
    Else
        HelgenWorldPhase = 2

        ; Capture only survivors who were still left wounded in Helgen.
        ; Freed / Departed survivors never regress.
        If HadvarState == 1
            HadvarState = 2
        EndIf

        If RalofState == 1
            RalofState = 2
        EndIf

        Debug.Trace("[STRE][HelgenInvestigation] Helgen world phase committed: BanditOccupied")
    EndIf

    ProjectBanditOccupied()
    ProjectHadvarState()
    ProjectRalofState()

EndFunction

Function ProjectBanditOccupied()

    ObjectReference encountersRef = GetRequiredAliasReference(PostHelgenEncountersMarker, "PostHelgenEncountersMarker")
    STRE_HelgenContinuityController continuityController = Quest.GetQuest("STRE_QUEST_HelgenNPCCleanup") as STRE_HelgenContinuityController

    If encountersRef == None
        Debug.Trace("[STRE][HelgenInvestigation] ERROR: cannot project BanditOccupied without PostHelgenEncountersMarker")
        Return
    EndIf

    If continuityController == None || !continuityController.IsRunning()
        Debug.Trace("[STRE][HelgenInvestigation] ERROR: cannot project BanditOccupied without the running Helgen continuity controller")
        Return
    EndIf

    ; Retire only the recent post-attack fire/smoke while preserving the
    ; collapsed bridge and debris established by the #66 continuity projection.
    continuityController.RetirePostAttackMajorFX()

    ; Reuse Bethesda's complete late post-Helgen encounter phase.
    encountersRef.Enable()

    ; STRE's temporary rubble squeeze is only useful in RecentPostAttack.
    DisableAliasReference(RubbleSqueezeEntranceSide, "RubbleSqueezeEntranceSide")
    DisableAliasReference(RubbleSqueezeSurvivorSide, "RubbleSqueezeSurvivorSide")

    Debug.Trace("[STRE][HelgenInvestigation] BanditOccupied world projection applied")

EndFunction

ObjectReference Function GetRequiredAliasReference(ReferenceAlias aliasRef, String aliasName)

    If aliasRef == None
        Debug.Trace("[STRE][HelgenInvestigation] ERROR: " + aliasName + " alias property is empty")
        Return None
    EndIf

    ObjectReference ref = aliasRef.GetReference()

    If ref == None
        Debug.Trace("[STRE][HelgenInvestigation] ERROR: " + aliasName + " alias is empty")
    EndIf

    Return ref

EndFunction

Function DisableAliasReference(ReferenceAlias aliasRef, String aliasName)

    ObjectReference ref = GetRequiredAliasReference(aliasRef, aliasName)

    If ref != None
        ref.Disable()
    EndIf

EndFunction

; ============================================================================
; Local Skyrim projections
; ============================================================================

Function ProjectHadvarState()

    Actor hadvarRef = Hadvar.GetActorReference()

    If hadvarRef == None
        Debug.Trace("[STRE][HelgenInvestigation] ERROR: Hadvar alias is empty")
        Return
    EndIf

    If HadvarState == 1
        ProjectSurvivorAtMarker(hadvarRef, HadvarWoundedMarker, "Hadvar", "WoundedInCave")
    ElseIf HadvarState == 2
        ProjectCapturedSurvivor(hadvarRef, HadvarCapturedMarker, HadvarCapturedDoor, "Hadvar")
    Else
        Debug.Trace("[STRE][HelgenInvestigation] Hadvar projection not implemented for state " + HadvarState)
    EndIf

EndFunction

Function ProjectRalofState()

    Actor ralofRef = Ralof.GetActorReference()

    If ralofRef == None
        Debug.Trace("[STRE][HelgenInvestigation] ERROR: Ralof alias is empty")
        Return
    EndIf

    If RalofState == 1
        ProjectSurvivorAtMarker(ralofRef, RalofWoundedMarker, "Ralof", "WoundedInCave")
    ElseIf RalofState == 2
        ProjectCapturedSurvivor(ralofRef, RalofCapturedMarker, RalofCapturedDoor, "Ralof")
    Else
        Debug.Trace("[STRE][HelgenInvestigation] Ralof projection not implemented for state " + RalofState)
    EndIf

EndFunction

Function ProjectCapturedSurvivor(Actor survivorRef, ReferenceAlias markerAlias, ReferenceAlias doorAlias, String survivorName)

    If doorAlias == None
        Debug.Trace("[STRE][HelgenInvestigation] ERROR: " + survivorName + " CapturedInKeep door alias property is empty")
        Return
    EndIf

    ObjectReference doorRef = doorAlias.GetReference()

    If doorRef == None
        Debug.Trace("[STRE][HelgenInvestigation] ERROR: " + survivorName + " CapturedInKeep door alias is empty")
        Return
    EndIf

    ; The capture transition is projected only while Helgen is off-screen.
    ; Re-applying these operations is safe after save/load or campaign recovery.
    doorRef.SetOpen(False)
    doorRef.Lock(True)

    ProjectSurvivorAtMarker(survivorRef, markerAlias, survivorName, "CapturedInKeep")

EndFunction

Function ProjectSurvivorAtMarker(Actor survivorRef, ReferenceAlias markerAlias, String survivorName, String stateName)

    If markerAlias == None
        Debug.Trace("[STRE][HelgenInvestigation] ERROR: " + survivorName + " " + stateName + " marker alias property is empty")
        Return
    EndIf

    ObjectReference markerRef = markerAlias.GetReference()

    If markerRef == None
        Debug.Trace("[STRE][HelgenInvestigation] ERROR: " + survivorName + " " + stateName + " marker alias is empty")
        Return
    EndIf

    survivorRef.Enable()
    survivorRef.MoveTo(markerRef)
    survivorRef.SetRestrained(False)
    survivorRef.EvaluatePackage()

    Debug.Trace("[STRE][HelgenInvestigation] " + survivorName + " state projected: " + stateName)

EndFunction

; ============================================================================
; Diagnostics
; ============================================================================

Int Function GetInvestigationState()
    Return InvestigationState
EndFunction

Float Function GetInvestigationStartGameTime()
    Return InvestigationStartGameTime
EndFunction

Int Function GetHadvarState()
    Return HadvarState
EndFunction

Int Function GetRalofState()
    Return RalofState
EndFunction

Int Function GetHelgenWorldPhase()
    Return HelgenWorldPhase
EndFunction

Int Function GetMainQuestPath()
    Return MainQuestPath
EndFunction
