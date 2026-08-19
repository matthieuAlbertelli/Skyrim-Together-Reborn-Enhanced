Scriptname STRE_HelgenRubbleSqueezeActivator extends ObjectReference

Bool Busy = False

Event OnActivate(ObjectReference akActionRef)

    If akActionRef != Game.GetPlayer()
        Return
    EndIf

    If Busy
        Return
    EndIf

    ObjectReference destination = GetLinkedRef()

    If destination == None
        Debug.Trace("[STRE][HelgenInvestigation] ERROR: Rubble squeeze activator has no destination linked ref")
        Return
    EndIf

    Busy = True

    Debug.Trace("[STRE][HelgenInvestigation] Rubble squeeze activated")

    Game.FadeOutGame(True, True, 0.0, 0.25)
    Utility.Wait(0.35)

    akActionRef.MoveTo(destination)

    Utility.Wait(0.10)
    Game.FadeOutGame(False, True, 0.0, 0.25)
    Utility.Wait(0.30)

    Busy = False

EndEvent