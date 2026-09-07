;BEGIN FRAGMENT CODE - Do not edit anything between this and the end comment
;NEXT FRAGMENT INDEX 8
Scriptname QF_STRE_QUEST_AlternateStart_02001AF9 Extends Quest Hidden

;BEGIN ALIAS PROPERTY PlayerSeat08
;ALIAS PROPERTY TYPE ReferenceAlias
ReferenceAlias Property Alias_PlayerSeat08 Auto
;END ALIAS PROPERTY

;BEGIN ALIAS PROPERTY PlayerSeat01
;ALIAS PROPERTY TYPE ReferenceAlias
ReferenceAlias Property Alias_PlayerSeat01 Auto
;END ALIAS PROPERTY

;BEGIN ALIAS PROPERTY PlayerSeat03
;ALIAS PROPERTY TYPE ReferenceAlias
ReferenceAlias Property Alias_PlayerSeat03 Auto
;END ALIAS PROPERTY

;BEGIN ALIAS PROPERTY PlayerSeat04
;ALIAS PROPERTY TYPE ReferenceAlias
ReferenceAlias Property Alias_PlayerSeat04 Auto
;END ALIAS PROPERTY

;BEGIN ALIAS PROPERTY PlayerSeat06
;ALIAS PROPERTY TYPE ReferenceAlias
ReferenceAlias Property Alias_PlayerSeat06 Auto
;END ALIAS PROPERTY

;BEGIN ALIAS PROPERTY PlayerSeat10
;ALIAS PROPERTY TYPE ReferenceAlias
ReferenceAlias Property Alias_PlayerSeat10 Auto
;END ALIAS PROPERTY

;BEGIN ALIAS PROPERTY Player
;ALIAS PROPERTY TYPE ReferenceAlias
ReferenceAlias Property Alias_Player Auto
;END ALIAS PROPERTY

;BEGIN ALIAS PROPERTY PlayerSeat09
;ALIAS PROPERTY TYPE ReferenceAlias
ReferenceAlias Property Alias_PlayerSeat09 Auto
;END ALIAS PROPERTY

;BEGIN ALIAS PROPERTY PlayerSeat07
;ALIAS PROPERTY TYPE ReferenceAlias
ReferenceAlias Property Alias_PlayerSeat07 Auto
;END ALIAS PROPERTY

;BEGIN ALIAS PROPERTY PlayerSeat05
;ALIAS PROPERTY TYPE ReferenceAlias
ReferenceAlias Property Alias_PlayerSeat05 Auto
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

;BEGIN FRAGMENT Fragment_6
Function Fragment_6()
;BEGIN CODE
Debug.Trace("[STRE][AlternateStart] Starting MQ101 continuity cleanup prototype")

STREHelgenNPCCleanup.Start()

MQ101.SetStage(20)
MQ101.SetStage(25)
MQ101.SetStage(26)
MQ101.SetStage(28)
MQ101.SetStage(30)
MQ101.SetStage(40)
MQ101.SetStage(70)
MQ101.SetStage(100)
MQ101.SetStage(145)
MQ101.SetStage(150)
MQ101.SetStage(180)
MQ101.SetStage(200)
MQ101.SetStage(250)
MQ101.SetStage(500)
MQ101.SetStage(800)
MQ101.SetStage(900)

STREHelgenNPCCleanup.SetStage(10)
STREHelgenNPCCleanup.SetStage(20)
STREHelgenNPCCleanup.SetStage(30)

STRE_HelgenContinuityController helgenContinuity = STREHelgenNPCCleanup as STRE_HelgenContinuityController

If helgenContinuity
    helgenContinuity.ApplyPostAttackProjection()
Else
    Debug.Trace("[STRE][Helgen] ERROR: continuity controller unavailable")
EndIf

STREHelgenNPCCleanup.SetStage(40)

Debug.Trace("[STRE][AlternateStart] MQ101 continuity cleanup prototype completed")
;END CODE
EndFunction
;END FRAGMENT

;END FRAGMENT CODE - Do not edit anything between this and the begin comment

Quest Property MQ101  Auto

Quest Property STREHelgenNPCCleanup  Auto
