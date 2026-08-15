#include <Services/CampaignProtocolService.h>

#include <GameServer.h>
#include <World.h>

#include <Events/PlayerLeaveEvent.h>
#include <Messages/CampaignMessages.h>
#include <Messages/CampaignRequests.h>

#include <cstdint>

using namespace STRE::Campaign;

CampaignProtocolService::CampaignProtocolService(
    World& aWorld,
    entt::dispatcher& aDispatcher) noexcept
    : m_world(aWorld)
    , m_admission(GameServer::Get()->GetCampaignRuntime())
    , m_createConnection(aDispatcher.sink<PacketEvent<CampaignCreateRequest>>()
          .connect<&CampaignProtocolService::OnCreate>(this))
    , m_joinConnection(aDispatcher.sink<PacketEvent<CampaignJoinRequest>>()
          .connect<&CampaignProtocolService::OnJoin>(this))
    , m_resumeConnection(aDispatcher.sink<PacketEvent<CampaignResumeRequest>>()
          .connect<&CampaignProtocolService::OnResume>(this))
    , m_startConnection(aDispatcher.sink<PacketEvent<CampaignStartRequest>>()
          .connect<&CampaignProtocolService::OnStart>(this))
    , m_readyConnection(aDispatcher.sink<PacketEvent<CampaignSetReadyRequest>>()
          .connect<&CampaignProtocolService::OnSetReady>(this))
    , m_leaveConnection(aDispatcher.sink<PacketEvent<CampaignLeaveRequest>>()
          .connect<&CampaignProtocolService::OnLeave>(this))
    , m_playerLeaveConnection(aDispatcher.sink<PlayerLeaveEvent>()
          .connect<&CampaignProtocolService::OnPlayerLeave>(this))
{
}

CampaignConnectionHandle CampaignProtocolService::ToHandle(
    const Player& acPlayer) noexcept
{
    return static_cast<CampaignConnectionHandle>(acPlayer.GetId());
}

CampaignConnectionRegistration
CampaignProtocolService::RegisterAuthenticatedPlayer(
    Player& aPlayer,
    const std::string& acDurablePlayerId) noexcept
{
    return m_admission.RegisterConnection(
        ToHandle(aPlayer), acDurablePlayerId);
}

void CampaignProtocolService::UnregisterAuthenticatedPlayer(
    Player& aPlayer) noexcept
{
    (void)m_admission.Disconnect(ToHandle(aPlayer));
}

const CampaignAdmissionRecord* CampaignProtocolService::GetAdmission(
    const Player& acPlayer) const noexcept
{
    return m_admission.FindConnection(ToHandle(acPlayer));
}

bool CampaignProtocolService::SharesCampaignParty(
    const Player& acPlayer,
    const CampaignId& acCampaign,
    bool aRequireCompleteRoster) noexcept
{
    const auto partyId = acPlayer.GetParty().JoinedPartyId;
    if (!partyId)
        return false;

    std::size_t matchingPlayers = 0;
    const std::vector<CampaignConnectionHandle> connections =
        m_admission.GetAdmittedConnections(acCampaign);
    for (CampaignConnectionHandle connection : connections)
    {
        const auto* const pMember = m_world.GetPlayerManager().GetById(
            static_cast<std::uint32_t>(connection));
        if (!pMember || pMember->GetParty().JoinedPartyId != partyId)
            return false;
        ++matchingPlayers;
    }

    if (aRequireCompleteRoster)
    {
        const std::optional<CampaignSnapshotData> snapshot =
            m_admission.BuildSnapshot(acCampaign);
        if (!snapshot || snapshot->Roster.size() != matchingPlayers)
            return false;
        for (const CampaignPublicSlotData& slot : snapshot->Roster)
        {
            if (!slot.Present)
                return false;
        }
    }
    return matchingPlayers > 0;
}

void CampaignProtocolService::SendResult(
    Player& aPlayer,
    const CommandResult& acResult) const noexcept
{
    CampaignCommandResponse response;
    response.Operation = acResult.Operation;
    response.Result = acResult.Result;
    response.MutationId = acResult.MutationId.c_str();
    response.CampaignId = acResult.CampaignId.c_str();
    response.StateVersion = acResult.Version;
    response.CampaignSlotId = acResult.CampaignSlotId.c_str();
    response.CharacterBindingId = acResult.CharacterBindingId.c_str();
    aPlayer.Send(response);
}

void CampaignProtocolService::BroadcastSnapshot(
    const CampaignSnapshotData& acSnapshot) const noexcept
{
    NotifyCampaignSnapshot notification;
    notification.Snapshot = acSnapshot;
    const auto connections = m_admission.GetAdmittedConnections(
        CampaignId{acSnapshot.CampaignId.c_str()});
    for (CampaignConnectionHandle connection : connections)
    {
        auto* const pPlayer = m_world.GetPlayerManager().GetById(
            static_cast<std::uint32_t>(connection));
        if (pPlayer)
            pPlayer->Send(notification);
    }
}

void CampaignProtocolService::Finish(
    Player& aPlayer,
    CommandResult aResult,
    std::string_view acMutationId) const noexcept
{
    aResult.MutationId = acMutationId;
    const auto* const pAdmission = GetAdmission(aPlayer);
    const char* const pLevel = aResult.Succeeded() ? "accepted" : "rejected";
    spdlog::info(
        "[STRE][CampaignProtocol] command={} operation={} result={} transientPlayer={} durablePlayer={} campaign={} revision={}",
        pLevel, static_cast<unsigned>(aResult.Operation),
        static_cast<unsigned>(aResult.Result), aPlayer.GetId(),
        pAdmission ? pAdmission->Player.Value : "unregistered",
        aResult.CampaignId, aResult.Version);
    SendResult(aPlayer, aResult);
    if (aResult.Snapshot)
        BroadcastSnapshot(*aResult.Snapshot);
}

void CampaignProtocolService::OnCreate(
    const PacketEvent<CampaignCreateRequest>& acPacket) noexcept
{
    if (!acPacket.Packet.IsValid())
    {
        Finish(*acPacket.pPlayer,
            {CampaignProtocolOperation::Create,
             CampaignProtocolResult::InvalidRequest},
            acPacket.Packet.MutationId.c_str());
        return;
    }
    Finish(*acPacket.pPlayer, m_admission.CreateCampaign(
        ToHandle(*acPacket.pPlayer), acPacket.Packet.MutationId.c_str(),
        m_world.GetPartyService().IsPlayerLeader(acPacket.pPlayer)),
        acPacket.Packet.MutationId.c_str());
}

void CampaignProtocolService::OnJoin(
    const PacketEvent<CampaignJoinRequest>& acPacket) noexcept
{
    if (!acPacket.Packet.IsValid())
    {
        Finish(*acPacket.pPlayer,
            {CampaignProtocolOperation::Join,
             CampaignProtocolResult::InvalidRequest},
            acPacket.Packet.MutationId.c_str());
        return;
    }
    const CampaignId campaign{acPacket.Packet.CampaignId.c_str()};
    Finish(*acPacket.pPlayer, m_admission.JoinCampaign(
        ToHandle(*acPacket.pPlayer), campaign.Value,
        acPacket.Packet.MutationId.c_str(),
        acPacket.Packet.ExpectedRevision,
        SharesCampaignParty(*acPacket.pPlayer, campaign, false)),
        acPacket.Packet.MutationId.c_str());
}

void CampaignProtocolService::OnResume(
    const PacketEvent<CampaignResumeRequest>& acPacket) noexcept
{
    if (!acPacket.Packet.IsValid())
    {
        Finish(*acPacket.pPlayer,
            {CampaignProtocolOperation::Resume,
             CampaignProtocolResult::InvalidRequest});
        return;
    }
    Finish(*acPacket.pPlayer, m_admission.ResumeCampaign(
        ToHandle(*acPacket.pPlayer), acPacket.Packet.CampaignId.c_str(),
        acPacket.Packet.CharacterBindingId.c_str()));
}

void CampaignProtocolService::OnStart(
    const PacketEvent<CampaignStartRequest>& acPacket) noexcept
{
    if (!acPacket.Packet.IsValid())
    {
        Finish(*acPacket.pPlayer,
            {CampaignProtocolOperation::Start,
             CampaignProtocolResult::InvalidRequest},
            acPacket.Packet.MutationId.c_str());
        return;
    }
    const CampaignId campaign{acPacket.Packet.CampaignId.c_str()};
    Finish(*acPacket.pPlayer, m_admission.StartCampaign(
        ToHandle(*acPacket.pPlayer), campaign.Value,
        acPacket.Packet.MutationId.c_str(),
        acPacket.Packet.ExpectedRevision,
        m_world.GetPartyService().IsPlayerLeader(acPacket.pPlayer),
        SharesCampaignParty(*acPacket.pPlayer, campaign, true)),
        acPacket.Packet.MutationId.c_str());
}

void CampaignProtocolService::OnSetReady(
    const PacketEvent<CampaignSetReadyRequest>& acPacket) noexcept
{
    if (!acPacket.Packet.IsValid())
    {
        Finish(*acPacket.pPlayer,
            {CampaignProtocolOperation::SetReady,
             CampaignProtocolResult::InvalidRequest},
            acPacket.Packet.MutationId.c_str());
        return;
    }
    Finish(*acPacket.pPlayer, m_admission.SetReady(
        ToHandle(*acPacket.pPlayer), acPacket.Packet.CampaignId.c_str(),
        acPacket.Packet.MutationId.c_str(),
        acPacket.Packet.ExpectedRevision, acPacket.Packet.Ready),
        acPacket.Packet.MutationId.c_str());
}

void CampaignProtocolService::OnLeave(
    const PacketEvent<CampaignLeaveRequest>& acPacket) noexcept
{
    if (!acPacket.Packet.IsValid())
    {
        Finish(*acPacket.pPlayer,
            {CampaignProtocolOperation::Leave,
             CampaignProtocolResult::InvalidRequest},
            acPacket.Packet.MutationId.c_str());
        return;
    }
    Finish(*acPacket.pPlayer, m_admission.LeaveCampaign(
        ToHandle(*acPacket.pPlayer), acPacket.Packet.CampaignId.c_str(),
        acPacket.Packet.MutationId.c_str(),
        acPacket.Packet.ExpectedRevision),
        acPacket.Packet.MutationId.c_str());
}

void CampaignProtocolService::OnPlayerLeave(
    const PlayerLeaveEvent& acEvent) noexcept
{
    const std::optional<CampaignSnapshotData> snapshot =
        m_admission.Disconnect(ToHandle(*acEvent.pPlayer));
    if (snapshot)
        BroadcastSnapshot(*snapshot);
}
