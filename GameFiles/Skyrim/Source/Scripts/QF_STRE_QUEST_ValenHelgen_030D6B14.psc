Scriptname QF_STRE_QUEST_ValenHelgen_030D6B14 extends Quest Hidden

Function Fragment_0()
    If !IsObjectiveDisplayed(10)
        SetObjectiveDisplayed(10)
    EndIf
    SetActive(True)
    STRE_ValenHelgenObserver observer = GetAlias(0) as STRE_ValenHelgenObserver
    If observer
        observer.Arm()
    EndIf
EndFunction

Function Fragment_1()
    If !IsObjectiveCompleted(10)
        SetObjectiveCompleted(10)
    EndIf
    If !IsObjectiveDisplayed(20)
        SetObjectiveDisplayed(20)
    EndIf
EndFunction
