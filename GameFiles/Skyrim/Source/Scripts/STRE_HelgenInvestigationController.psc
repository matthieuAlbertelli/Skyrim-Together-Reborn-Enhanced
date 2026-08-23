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

    ; In standalone Skyrim, this quest owns the relative T+4 deadline.
    ; While connected to STR, Papyrus never makes the campaign transition.
    ArmStandaloneBanditOccupationDeadline()

EndFunction

; ============================================================================
; Standalone T+4 authority
;
; The multiplayer campaign adapter/server owns this transition while connected.
; The local Papyrus timer exists only so STRE_AlternateStart remains standalone.
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
        Debug.Trace("[STRE][HelgenInvestigation] Connected to STR: standalone T+4 authority disabled")
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

Event OnUpdateGameTime()

    ; A timer may have been armed before the player connected. Never let that
    ; stale local registration become campaign authority while online.
    If SkyrimTogetherUtils.IsConnected()
        Debug.Trace("[STRE][HelgenInvestigation] Standalone T+4 update ignored while connected to STR")
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

    If HelgenWorldPhase != 1
        Return
    EndIf

    ; Do not mutate campaign state online. Keep only a lightweight watcher so
    ; standalone fallback can resume if this save is later disconnected.
    If SkyrimTogetherUtils.IsConnected()
        ArmStandalonePendingPresenceCheck()
        Return
    EndIf

    If IsLocalPlayerInHelgen()
        ArmStandalonePendingPresenceCheck()
    Else
        CommitBanditOccupation()
    EndIf

EndEvent

; ============================================================================
; Bandit occupation state transition
;
; In multiplayer the authoritative campaign layer calls these transitions.
; In standalone Skyrim the T+4 fallback above calls them locally.
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

    If encountersRef == None
        Debug.Trace("[STRE][HelgenInvestigation] ERROR: cannot project BanditOccupied without PostHelgenEncountersMarker")
        Return
    EndIf

    ; Reuse Bethesda's complete late post-Helgen encounter phase.
    encountersRef.Enable()

    DisableAliasReference(PostHelgenBridge, "PostHelgenBridge")
    DisableAliasReference(PostHelgenBridgeDebris, "PostHelgenBridgeDebris")

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