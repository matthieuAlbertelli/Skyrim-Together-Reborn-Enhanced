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
BeginCharacterCreation()
;END CODE
EndFunction
;END FRAGMENT

;BEGIN FRAGMENT Fragment_0
Function Fragment_0()
;BEGIN CODE
BeginCharacterCreation()
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

; Both entry fragments share placement only. Alias properties/ESP bindings stay unchanged.
Function BeginCharacterCreation()
    Actor playerRef = Alias_Player.GetActorReference()
    ObjectReference startRef = Game.GetFormFromFile(0x0001B771, "STRE_AlternateStart.esp") as ObjectReference
    if playerRef == None || startRef == None
        Debug.Trace("[STRE][AlternateStart] Creation placement failed: player/start marker missing")
        return
    endif
    Cell startCell = startRef.GetParentCell()
    if startCell == None
        Debug.Trace("[STRE][AlternateStart] Creation placement failed: start cell missing")
        return
    endif
    ; Existing non-furniture marker; native sealed-PlayerId rank placement follows authorization.
    playerRef.MoveTo(startRef)
    Utility.Wait(1.0)
    if playerRef.GetParentCell() != startCell || playerRef.GetDistance(startRef) > 32.0
        Debug.Trace("[STRE][AlternateStart] Creation placement failed: wrong cell or position")
        return
    endif
    Debug.Trace("[STRE][AlternateStart] Creation placement ready; entering stage 20")
    SetStage(20)
EndFunction


Quest Property MQ101  Auto

Quest Property STREHelgenNPCCleanup  Auto
