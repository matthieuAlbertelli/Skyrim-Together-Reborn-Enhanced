;BEGIN FRAGMENT CODE - Do not edit anything between this and the end comment
;NEXT FRAGMENT INDEX 13
Scriptname QF_STRE_HelgenCleanup_0302D022 Extends Quest Hidden

;BEGIN ALIAS PROPERTY TortureRoomStormcloak1
;ALIAS PROPERTY TYPE ReferenceAlias
ReferenceAlias Property Alias_TortureRoomStormcloak1 Auto
;END ALIAS PROPERTY

;BEGIN ALIAS PROPERTY ObjectiveKeepEscapeExit
;ALIAS PROPERTY TYPE ReferenceAlias
ReferenceAlias Property Alias_ObjectiveKeepEscapeExit Auto
;END ALIAS PROPERTY

;BEGIN ALIAS PROPERTY StoreroomImperial2
;ALIAS PROPERTY TYPE ReferenceAlias
ReferenceAlias Property Alias_StoreroomImperial2 Auto
;END ALIAS PROPERTY

;BEGIN ALIAS PROPERTY Torturer
;ALIAS PROPERTY TYPE ReferenceAlias
ReferenceAlias Property Alias_Torturer Auto
;END ALIAS PROPERTY

;BEGIN ALIAS PROPERTY TortureRoomStormcloak2
;ALIAS PROPERTY TYPE ReferenceAlias
ReferenceAlias Property Alias_TortureRoomStormcloak2 Auto
;END ALIAS PROPERTY

;BEGIN ALIAS PROPERTY TortureRoomImperialSoldier2
;ALIAS PROPERTY TYPE ReferenceAlias
ReferenceAlias Property Alias_TortureRoomImperialSoldier2 Auto
;END ALIAS PROPERTY

;BEGIN ALIAS PROPERTY Hadvar
;ALIAS PROPERTY TYPE ReferenceAlias
ReferenceAlias Property Alias_Hadvar Auto
;END ALIAS PROPERTY

;BEGIN ALIAS PROPERTY StoreroomStormcloak1
;ALIAS PROPERTY TYPE ReferenceAlias
ReferenceAlias Property Alias_StoreroomStormcloak1 Auto
;END ALIAS PROPERTY

;BEGIN ALIAS PROPERTY PoolRoomStormcloak4
;ALIAS PROPERTY TYPE ReferenceAlias
ReferenceAlias Property Alias_PoolRoomStormcloak4 Auto
;END ALIAS PROPERTY

;BEGIN ALIAS PROPERTY PoolRoomImperial3
;ALIAS PROPERTY TYPE ReferenceAlias
ReferenceAlias Property Alias_PoolRoomImperial3 Auto
;END ALIAS PROPERTY

;BEGIN ALIAS PROPERTY PoolRoomStormcloak1
;ALIAS PROPERTY TYPE ReferenceAlias
ReferenceAlias Property Alias_PoolRoomStormcloak1 Auto
;END ALIAS PROPERTY

;BEGIN ALIAS PROPERTY PoolRoomImperial5
;ALIAS PROPERTY TYPE ReferenceAlias
ReferenceAlias Property Alias_PoolRoomImperial5 Auto
;END ALIAS PROPERTY

;BEGIN ALIAS PROPERTY TortureRoomImperialSoldier3
;ALIAS PROPERTY TYPE ReferenceAlias
ReferenceAlias Property Alias_TortureRoomImperialSoldier3 Auto
;END ALIAS PROPERTY

;BEGIN ALIAS PROPERTY TortureRoomImperialSoldier1
;ALIAS PROPERTY TYPE ReferenceAlias
ReferenceAlias Property Alias_TortureRoomImperialSoldier1 Auto
;END ALIAS PROPERTY

;BEGIN ALIAS PROPERTY TortureRoomStormcloak3
;ALIAS PROPERTY TYPE ReferenceAlias
ReferenceAlias Property Alias_TortureRoomStormcloak3 Auto
;END ALIAS PROPERTY

;BEGIN ALIAS PROPERTY ImperialSoldierHelgen01
;ALIAS PROPERTY TYPE ReferenceAlias
ReferenceAlias Property Alias_ImperialSoldierHelgen01 Auto
;END ALIAS PROPERTY

;BEGIN ALIAS PROPERTY Ralof
;ALIAS PROPERTY TYPE ReferenceAlias
ReferenceAlias Property Alias_Ralof Auto
;END ALIAS PROPERTY

;BEGIN ALIAS PROPERTY HelgenKeepCollapseTrigger
;ALIAS PROPERTY TYPE ReferenceAlias
ReferenceAlias Property Alias_HelgenKeepCollapseTrigger Auto
;END ALIAS PROPERTY

;BEGIN FRAGMENT Fragment_0
Function Fragment_0()
;BEGIN CODE
Alias_StoreroomStormcloak1.TryToEnable()
Alias_StoreroomStormcloak1.GetActorReference().Kill()

Alias_StoreroomImperial2.TryToEnable()
Alias_StoreroomImperial2.GetActorReference().Kill()

Alias_Torturer.TryToEnable()
Alias_Torturer.GetActorReference().Kill()

Alias_TortureRoomImperialSoldier1.TryToEnable()
Alias_TortureRoomImperialSoldier1.GetActorReference().Kill()

Alias_TortureRoomImperialSoldier2.TryToEnable()
Alias_TortureRoomImperialSoldier2.GetActorReference().Kill()

Alias_TortureRoomImperialSoldier3.TryToEnable()
Alias_TortureRoomImperialSoldier3.GetActorReference().Kill()

Alias_TortureRoomStormcloak1.TryToEnable()
Alias_TortureRoomStormcloak1.GetActorReference().Kill()

Alias_TortureRoomStormcloak2.TryToEnable()
Alias_TortureRoomStormcloak2.GetActorReference().Kill()

Alias_TortureRoomStormcloak3.TryToEnable()
Alias_TortureRoomStormcloak3.GetActorReference().Kill()

Alias_PoolRoomStormcloak1.TryToEnable()
Alias_PoolRoomStormcloak1.GetActorReference().Kill()

Alias_PoolRoomStormcloak4.TryToEnable()
Alias_PoolRoomStormcloak4.GetActorReference().Kill()

Alias_PoolRoomImperial3.TryToEnable()
Alias_PoolRoomImperial3.GetActorReference().Kill()

Alias_PoolRoomImperial5.TryToEnable()
Alias_PoolRoomImperial5.GetActorReference().Kill()
;END CODE
EndFunction
;END FRAGMENT

;BEGIN FRAGMENT Fragment_9
Function Fragment_9()
;BEGIN AUTOCAST TYPE STRE_HelgenContinuityController
Quest __temp = self as Quest
STRE_HelgenContinuityController kmyQuest = __temp as STRE_HelgenContinuityController
;END AUTOCAST
;BEGIN CODE
Debug.Trace("[STRE][Helgen] Post-attack projection stage completed")
;END CODE
EndFunction
;END FRAGMENT

;BEGIN FRAGMENT Fragment_5
Function Fragment_5()
;BEGIN CODE
Alias_ImperialSoldierHelgen01.TryToDisable()
;END CODE
EndFunction
;END FRAGMENT

;BEGIN FRAGMENT Fragment_3
Function Fragment_3()
;BEGIN CODE
Alias_Hadvar.GetActorReference().MoveTo(Alias_ObjectiveKeepEscapeExit.GetReference())
Alias_Ralof.GetActorReference().MoveTo(Alias_ObjectiveKeepEscapeExit.GetReference())
;END CODE
EndFunction
;END FRAGMENT

;END FRAGMENT CODE - Do not edit anything between this and the begin comment
