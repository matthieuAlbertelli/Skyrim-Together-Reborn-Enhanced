;BEGIN FRAGMENT CODE - Do not edit anything between this and the end comment
;NEXT FRAGMENT INDEX 7
Scriptname QF_STRE_QUEST_HelgenInvestig_0305BCA5 Extends Quest Hidden

;BEGIN ALIAS PROPERTY Hadvar
;ALIAS PROPERTY TYPE ReferenceAlias
ReferenceAlias Property Alias_Hadvar Auto
;END ALIAS PROPERTY

;BEGIN ALIAS PROPERTY Ralof
;ALIAS PROPERTY TYPE ReferenceAlias
ReferenceAlias Property Alias_Ralof Auto
;END ALIAS PROPERTY

;BEGIN ALIAS PROPERTY RalofCapturedDoor
;ALIAS PROPERTY TYPE ReferenceAlias
ReferenceAlias Property Alias_RalofCapturedDoor Auto
;END ALIAS PROPERTY

;BEGIN ALIAS PROPERTY PostHelgenBridge
;ALIAS PROPERTY TYPE ReferenceAlias
ReferenceAlias Property Alias_PostHelgenBridge Auto
;END ALIAS PROPERTY

;BEGIN ALIAS PROPERTY RalofWoundedMarker
;ALIAS PROPERTY TYPE ReferenceAlias
ReferenceAlias Property Alias_RalofWoundedMarker Auto
;END ALIAS PROPERTY

;BEGIN ALIAS PROPERTY HadvarCapturedMarker
;ALIAS PROPERTY TYPE ReferenceAlias
ReferenceAlias Property Alias_HadvarCapturedMarker Auto
;END ALIAS PROPERTY

;BEGIN ALIAS PROPERTY HadvarWoundedMarker
;ALIAS PROPERTY TYPE ReferenceAlias
ReferenceAlias Property Alias_HadvarWoundedMarker Auto
;END ALIAS PROPERTY

;BEGIN ALIAS PROPERTY RubbleSqueezeEntranceSide
;ALIAS PROPERTY TYPE ReferenceAlias
ReferenceAlias Property Alias_RubbleSqueezeEntranceSide Auto
;END ALIAS PROPERTY

;BEGIN ALIAS PROPERTY RalofCapturedMarker
;ALIAS PROPERTY TYPE ReferenceAlias
ReferenceAlias Property Alias_RalofCapturedMarker Auto
;END ALIAS PROPERTY

;BEGIN ALIAS PROPERTY HadvarCapturedDoor
;ALIAS PROPERTY TYPE ReferenceAlias
ReferenceAlias Property Alias_HadvarCapturedDoor Auto
;END ALIAS PROPERTY

;BEGIN ALIAS PROPERTY PostHelgenEncountersMarker
;ALIAS PROPERTY TYPE ReferenceAlias
ReferenceAlias Property Alias_PostHelgenEncountersMarker Auto
;END ALIAS PROPERTY

;BEGIN ALIAS PROPERTY RubbleSqueezeSurvivorSide
;ALIAS PROPERTY TYPE ReferenceAlias
ReferenceAlias Property Alias_RubbleSqueezeSurvivorSide Auto
;END ALIAS PROPERTY

;BEGIN ALIAS PROPERTY PostHelgenBridgeDebris
;ALIAS PROPERTY TYPE ReferenceAlias
ReferenceAlias Property Alias_PostHelgenBridgeDebris Auto
;END ALIAS PROPERTY

;BEGIN FRAGMENT Fragment_0
Function Fragment_0()
;BEGIN CODE
Quest owningQuest = Self as Quest
STRE_HelgenInvestigationController controller = owningQuest as STRE_HelgenInvestigationController

If controller
    controller.BeginInvestigation()
Else
    Debug.Trace("[STRE][HelgenInvestigation] ERROR: controller unavailable at stage 10")
EndIf
;END CODE
EndFunction
;END FRAGMENT

;END FRAGMENT CODE - Do not edit anything between this and the begin comment
