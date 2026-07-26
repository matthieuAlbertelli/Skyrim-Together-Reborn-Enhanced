;BEGIN FRAGMENT CODE - Do not edit anything between this and the end comment
;NEXT FRAGMENT INDEX 6
Scriptname QF_STRE_QUEST_AlternateStart_02001AF9 Extends Quest Hidden

;BEGIN ALIAS PROPERTY Player
;ALIAS PROPERTY TYPE ReferenceAlias
ReferenceAlias Property Alias_Player Auto
;END ALIAS PROPERTY

;BEGIN ALIAS PROPERTY PlayerSeat01
;ALIAS PROPERTY TYPE ReferenceAlias
ReferenceAlias Property Alias_PlayerSeat01 Auto
;END ALIAS PROPERTY

;BEGIN ALIAS PROPERTY PlayerSeat02
;ALIAS PROPERTY TYPE ReferenceAlias
ReferenceAlias Property Alias_PlayerSeat02 Auto
;END ALIAS PROPERTY

;BEGIN FRAGMENT Fragment_4
Function Fragment_4()
;BEGIN CODE
Actor playerRef = Alias_Player.GetActorReference()
ObjectReference seatRef = Alias_PlayerSeat02.GetReference()

if playerRef == None
    Debug.Trace("[STRE][AlternateStart] Player alias is empty")
    Debug.Notification("STRE : alias Player introuvable")

elseif seatRef == None
    Debug.Trace("[STRE][AlternateStart] PlayerSeat02 alias is empty")
    Debug.Notification("STRE : chaise 02 introuvable")

else
    Debug.Trace("[STRE][AlternateStart] Moving player directly to seat 02")

    playerRef.MoveTo(seatRef)
    Utility.Wait(1.0)

    if playerRef.GetSitState() == 3
        Debug.Trace("[STRE][AlternateStart] Player seated successfully on seat 02")
        Debug.Notification("STRE : joueur assis sur le siège 02")
        SetStage(20)
    else
        Debug.Trace("[STRE][AlternateStart] Player moved but is not seated on seat 02")
        Debug.Notification("STRE : joueur déplacé mais non assis")
    endif
endif
;END CODE
EndFunction
;END FRAGMENT

;BEGIN FRAGMENT Fragment_0
Function Fragment_0()
;BEGIN CODE
Actor playerRef = Alias_Player.GetActorReference()
ObjectReference seatRef = Alias_PlayerSeat01.GetReference()

if playerRef == None
    Debug.Trace("[STRE][AlternateStart] Player alias is empty")
    Debug.Notification("STRE : alias Player introuvable")

elseif seatRef == None
    Debug.Trace("[STRE][AlternateStart] PlayerSeat01 alias is empty")
    Debug.Notification("STRE : chaise introuvable")

else
    Debug.Trace("[STRE][AlternateStart] Moving player directly to seat")

    playerRef.MoveTo(seatRef)
    Utility.Wait(1.0)

    if playerRef.GetSitState() == 3
        Debug.Trace("[STRE][AlternateStart] Player seated successfully")
        Debug.Notification("STRE : joueur assis")
        SetStage(20)
    else
        Debug.Trace("[STRE][AlternateStart] Player moved but is not seated")
        Debug.Notification("STRE : joueur déplacé mais non assis")
    endif
endif
;END CODE
EndFunction
;END FRAGMENT

;END FRAGMENT CODE - Do not edit anything between this and the begin comment
