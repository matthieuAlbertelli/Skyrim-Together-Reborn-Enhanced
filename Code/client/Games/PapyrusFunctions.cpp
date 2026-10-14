#include "PapyrusFunctions.h"

#include <Games/ActorExtension.h>
#include <Services/TransportService.h>
#include <Services/CampaignRuntimeGateService.h>
#include <World.h>

namespace PapyrusFunctions
{

bool IsRemotePlayer(Actor* apActor)
{
    spdlog::info("Calling IsRemotePlayer");

    auto* pExtension = apActor->GetExtension();
    if (!pExtension)
        return false;

    return pExtension->IsRemotePlayer();
}

bool IsPlayer(Actor* apActor)
{
    spdlog::info("Calling IsPlayer");

    auto* pExtension = apActor->GetExtension();
    if (!pExtension)
        return false;

    return pExtension->IsPlayer();
}

bool IsConnected()
{
    return World::Get().GetTransport().IsConnected();
}

bool SignalHelgenInvestigationReady()
{
    return World::Get().GetCampaignService().SignalHelgenInvestigationReady();
}

bool IsHelgenInvestigationStartAuthorized()
{
    const auto* gate = CampaignRuntimeGateService::TryGet();
    return (!gate || !gate->IsLocked()) && World::Get().GetCampaignService().IsHelgenInvestigationStartAuthorized();
}

bool IsHelgenCampaignRequired()
{
    return World::Get().GetCampaignService().IsHelgenCampaignRequired();
}

bool AreAllRequiredPlayersOutsideHelgen()
{
    return IsHelgenInvestigationStartAuthorized() && World::Get().GetCampaignService().AreAllRequiredPlayersOutsideHelgen();
}

} // namespace PapyrusFunctions
