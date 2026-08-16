Scriptname STRE_HelgenCollapseLoadAlias extends ReferenceAlias

; ============================================================================
; Helgen Keep collapse visual loader
;
; This alias holds MQ101's vanilla collapse trigger reference.
;
; The semantic state of the collapse is established earlier by
; STRE_HelgenContinuityController.
;
; When HelgenKeep01 becomes attached and its 3D becomes available, this
; listener asks the controller to project the already-historical collapse
; visually.
; ============================================================================

Event OnCellAttach()
    STRE_HelgenContinuityController controller = GetOwningQuest() as STRE_HelgenContinuityController

    If controller
        controller.ApplyLoadedKeepCollapseVisual()
    EndIf
EndEvent