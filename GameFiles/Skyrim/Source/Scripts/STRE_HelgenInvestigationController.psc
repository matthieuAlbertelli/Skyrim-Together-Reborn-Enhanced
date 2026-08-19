Scriptname STRE_HelgenInvestigationController extends Quest

; ============================================================================
; Quest aliases
; ============================================================================

ReferenceAlias Property Hadvar Auto
ReferenceAlias Property Ralof Auto

ReferenceAlias Property HadvarWoundedMarker Auto
ReferenceAlias Property RalofWoundedMarker Auto

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
; MainQuestPath:
;   0 = Undecided
;   1 = Hadvar
;   2 = Ralof
;   3 = Neutral
; ============================================================================

Int InvestigationState = 0
Float InvestigationStartGameTime = -1.0

Int HadvarState = 0
Int RalofState = 0

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
        ObjectReference markerRef = HadvarWoundedMarker.GetReference()

        If markerRef == None
            Debug.Trace("[STRE][HelgenInvestigation] ERROR: Hadvar wounded marker alias is empty")
            Return
        EndIf

        hadvarRef.Enable()
        hadvarRef.MoveTo(markerRef)

        ; Temporary WoundedInCave projection.
        ; A dedicated wounded behaviour/package will replace or complement this.
	hadvarRef.SetRestrained(False)
	hadvarRef.EvaluatePackage()

        Debug.Trace("[STRE][HelgenInvestigation] Hadvar state projected: WoundedInCave")
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
        ObjectReference markerRef = RalofWoundedMarker.GetReference()

        If markerRef == None
            Debug.Trace("[STRE][HelgenInvestigation] ERROR: Ralof wounded marker alias is empty")
            Return
        EndIf

        ralofRef.Enable()
        ralofRef.MoveTo(markerRef)

        ; Temporary WoundedInCave projection.
        ; A dedicated wounded behaviour/package will replace or complement this.
	ralofRef.SetRestrained(False)
	ralofRef.EvaluatePackage()

        Debug.Trace("[STRE][HelgenInvestigation] Ralof state projected: WoundedInCave")
    Else
        Debug.Trace("[STRE][HelgenInvestigation] Ralof projection not implemented for state " + RalofState)
    EndIf

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

Int Function GetMainQuestPath()
    Return MainQuestPath
EndFunction