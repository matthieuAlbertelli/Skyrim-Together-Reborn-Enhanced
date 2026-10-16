Scriptname STRE_ValenHelgenObserver extends ReferenceAlias

STRE_HelgenInvestigationController Property Investigation Auto
Location Property HelgenLocation Auto
Bool CampaignObserved = False

Event OnInit()
    Arm()
EndEvent

Event OnPlayerLoadGame()
    ; Native quest bits and this alias are restored by the player's .ess.
    ; Never replay a cached actor or reconstruct discovery from highest stage.
    UnregisterForUpdate()
    Arm()
EndEvent

Event OnLocationChange(Location akOldLoc, Location akNewLoc)
    Arm()
EndEvent

Function Arm()
    UnregisterForUpdate()
    Quest narrative = GetOwningQuest()
    If narrative && narrative.IsRunning()
        If !narrative.GetStageDone(11) || !narrative.GetStageDone(12)
            RegisterForSingleUpdate(1.0)
        EndIf
    EndIf
EndFunction

Bool Function CanProgress()
    If SkyrimTogetherUtils.IsConnected() || SkyrimTogetherUtils.IsHelgenCampaignRequired()
        CampaignObserved = True
    EndIf
    If CampaignObserved && !SkyrimTogetherUtils.IsHelgenInvestigationStartAuthorized()
        Return False
    EndIf
    Quest narrative = GetOwningQuest()
    Return narrative && narrative.IsRunning() && narrative.GetStageDone(10)
EndFunction

Bool Function Observes(Actor playerRef, Actor survivor)
    If !playerRef || !survivor || !HelgenLocation
        Return False
    EndIf
    If survivor.IsDead() || survivor.IsDisabled() || !survivor.Is3DLoaded()
        Return False
    EndIf
    If playerRef.GetParentCell() != survivor.GetParentCell()
        Return False
    EndIf
    If !playerRef.IsInLocation(HelgenLocation) || !survivor.IsInLocation(HelgenLocation)
        Return False
    EndIf
    Return playerRef.GetDistance(survivor) <= 256.0 && playerRef.HasLOS(survivor)
EndFunction

Function MarkDiscovered(Int stage)
    ; Future rescue callers may use this BEFORE their physical success mutation.
    ; Callers must have established an actual rescue; this is not a player command.
    If stage != 11 && stage != 12
        Return
    EndIf
    Quest narrative = GetOwningQuest()
    If !CanProgress() || narrative.GetStageDone(stage)
        Return
    EndIf
    If !narrative.GetStageDone(20)
        narrative.SetStage(20)
    EndIf
    If CanProgress() && narrative.GetStageDone(20) && !narrative.GetStageDone(stage)
        narrative.SetStage(stage)
        Debug.Trace("[STRE][Valen] Shared survivor discovery stage=" + stage)
    EndIf
EndFunction

Function CheckSurvivor(Actor playerRef, ReferenceAlias survivorAlias, Int physicalState, Int discoveryStage)
    Quest narrative = GetOwningQuest()
    If narrative.GetStageDone(discoveryStage)
        Return
    EndIf
    ; Freed and Departed remain distinct trusted physical states. Both imply a
    ; prior encounter, defensively, when future rescue gameplay implements them.
    If physicalState == 3
        MarkDiscovered(discoveryStage)
    ElseIf physicalState == 4
        MarkDiscovered(discoveryStage)
    ElseIf (physicalState == 1 || physicalState == 2) && survivorAlias
        If Observes(playerRef, survivorAlias.GetActorReference())
            MarkDiscovered(discoveryStage)
        EndIf
    EndIf
EndFunction

Event OnUpdate()
    If CanProgress()
        Actor playerRef = GetActorReference()
        Quest narrative = GetOwningQuest()
        If playerRef && HelgenLocation && playerRef.IsInLocation(HelgenLocation)
            If !narrative.GetStageDone(20) && CanProgress()
                narrative.SetStage(20)
            EndIf
        EndIf
        If Investigation && CanProgress()
            CheckSurvivor(playerRef, Investigation.Hadvar, Investigation.HadvarState, 11)
            CheckSurvivor(playerRef, Investigation.Ralof, Investigation.RalofState, 12)
        EndIf
    EndIf
    Arm()
EndEvent
