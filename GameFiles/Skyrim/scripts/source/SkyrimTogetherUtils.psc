Scriptname SkyrimTogetherUtils

bool Function IsRemotePlayer(Actor actor) global native

bool Function IsPlayer(Actor actor) global native

bool Function IsConnected() global native

bool Function SignalHelgenInvestigationReady() global native
; Remains true across disconnect until a fresh game chooses its bootstrap mode.
bool Function IsHelgenCampaignRequired() global native

bool Function IsHelgenInvestigationStartAuthorized() global native

bool Function AreAllRequiredPlayersOutsideHelgen() global native
