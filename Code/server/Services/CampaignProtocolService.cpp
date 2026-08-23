#include <Services/CampaignProtocolService.h>

#include <GameServer.h>
#include <World.h>

#include <Events/PlayerLeaveEvent.h>
#include <Messages/CampaignMessages.h>
#include <Messages/CampaignRequests.h>
#include <Components.h>
#include <GroupSpatialCondition.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <utility>

using namespace STRE::Campaign;

namespace
{
std::string FallbackDisplayName(std::string_view acDisplayName) noexcept
{
    try
    {
        TiltedPhoques::String normalized;
        if (NormalizeCampaignLobbyDisplayName(acDisplayName, normalized))
            return normalized.c_str();
    }
    catch (...)
    {
    }
    return "Player";
}

constexpr std::array<std::uint32_t, 11> kHelgenCellLocalFormIds{
    0x000097ED, // HelgenExterior04
    0x000097EE, // ChargenExit
    0x0000980B, // HelgenExterior
    0x0000980C, // HelgenExterior05
    0x0000982A, // HelgenExterior02
    0x0000982B, // HelgenExterior06
    0x00009849, // HelgenExterior03
    0x0000984A, // HelgenExterior07
    0x00013A66, // HelgenTorolfsMill
    0x00013A67, // HelgenHomestead
    0x0005DE24  // HelgenKeep01
};

std::vector<GameId> BuildHelgenFootprint(const World& acWorld)
{
    const ModsComponent& mods = acWorld.ctx().at<ModsComponent>();
    const auto findSkyrim = [](const auto& acMods) -> std::optional<std::uint32_t>
    {
        for (const auto& [name, entry] : acMods)
        {
            if (std::strcmp(name.c_str(), "Skyrim.esm") == 0)
                return entry.id;
        }
        return std::nullopt;
    };

    std::optional<std::uint32_t> modId = findSkyrim(mods.GetStandardMods());
    if (!modId)
        modId = findSkyrim(mods.GetLiteMods());
    if (!modId)
        return {};

    std::vector<GameId> footprint;
    footprint.reserve(kHelgenCellLocalFormIds.size());
    for (std::uint32_t localFormId : kHelgenCellLocalFormIds)
        footprint.emplace_back(*modId, localFormId);
    return footprint;
}

CampaignHelgenSpatialStatus ToWireStatus(STRE::Spatial::EvaluationStatus aStatus) noexcept
{
    using STRE::Spatial::EvaluationStatus;
    switch (aStatus)
    {
    case EvaluationStatus::Known: return CampaignHelgenSpatialStatus::Known;
    case EvaluationStatus::GateClosed: return CampaignHelgenSpatialStatus::GateClosed;
    case EvaluationStatus::EmptyFootprint: return CampaignHelgenSpatialStatus::EmptyFootprint;
    case EvaluationStatus::IncompleteRoster: return CampaignHelgenSpatialStatus::IncompleteRoster;
    case EvaluationStatus::UnknownPosition: return CampaignHelgenSpatialStatus::UnknownPosition;
    }
    return CampaignHelgenSpatialStatus::GateClosed;
}
} // namespace

CampaignProtocolService::CampaignProtocolService(World& aWorld, entt::dispatcher& aDispatcher) noexcept
    : m_world(aWorld)
    , m_admission(GameServer::Get()->GetCampaignRuntime())
    , m_createConnection(aDispatcher.sink<PacketEvent<CampaignCreateRequest>>().connect<&CampaignProtocolService::OnCreate>(this))
    , m_joinConnection(aDispatcher.sink<PacketEvent<CampaignJoinRequest>>().connect<&CampaignProtocolService::OnJoin>(this))
    , m_resumeConnection(aDispatcher.sink<PacketEvent<CampaignResumeRequest>>().connect<&CampaignProtocolService::OnResume>(this))
    , m_startConnection(aDispatcher.sink<PacketEvent<CampaignStartRequest>>().connect<&CampaignProtocolService::OnStart>(this))
    , m_readyConnection(aDispatcher.sink<PacketEvent<CampaignSetReadyRequest>>().connect<&CampaignProtocolService::OnSetReady>(this))
    , m_leaveConnection(aDispatcher.sink<PacketEvent<CampaignLeaveRequest>>().connect<&CampaignProtocolService::OnLeave>(this))
    , m_joinByCodeConnection(aDispatcher.sink<PacketEvent<CampaignJoinByCodeRequest>>().connect<&CampaignProtocolService::OnJoinByCode>(this))
    , m_helgenReadyConnection(aDispatcher.sink<PacketEvent<CampaignHelgenInvestigationReadyRequest>>().connect<&CampaignProtocolService::OnHelgenInvestigationReady>(this))
    , m_playerLeaveConnection(aDispatcher.sink<PlayerLeaveEvent>().connect<&CampaignProtocolService::OnPlayerLeave>(this))
{
}

void CampaignProtocolService::OnPlayerLocationChanged(const Player& acPlayer) noexcept
{
    const CampaignAdmissionRecord* const pAdmission = GetAdmission(acPlayer);
    if (!pAdmission || !pAdmission->AdmittedIdentity)
        return;

    const CampaignId& campaign = pAdmission->AdmittedIdentity->Campaign;
    if (m_helgenStartedCampaigns.contains(campaign.Value))
        BroadcastHelgenState(campaign);
}

CampaignConnectionHandle CampaignProtocolService::ToHandle(const Player& acPlayer) noexcept
{
    return static_cast<CampaignConnectionHandle>(acPlayer.GetId());
}

CampaignConnectionRegistration CampaignProtocolService::RegisterAuthenticatedPlayer(Player& aPlayer, const std::string& acDurablePlayerId) noexcept
{
    return m_admission.RegisterConnection(ToHandle(aPlayer), acDurablePlayerId);
}

void CampaignProtocolService::UnregisterAuthenticatedPlayer(Player& aPlayer) noexcept
{
    (void)m_admission.Disconnect(ToHandle(aPlayer));
}

const CampaignAdmissionRecord* CampaignProtocolService::GetAdmission(const Player& acPlayer) const noexcept
{
    return m_admission.FindConnection(ToHandle(acPlayer));
}

bool CampaignProtocolService::SharesCampaignParty(const Player& acPlayer, const CampaignId& acCampaign, bool aRequireCompleteRoster) noexcept
{
    const auto partyId = acPlayer.GetParty().JoinedPartyId;
    if (!partyId)
        return false;

    std::size_t matchingPlayers = 0;
    const std::vector<CampaignConnectionHandle> connections = m_admission.GetAdmittedConnections(acCampaign);
    for (CampaignConnectionHandle connection : connections)
    {
        const auto* const pMember = m_world.GetPlayerManager().GetById(static_cast<std::uint32_t>(connection));
        if (!pMember || pMember->GetParty().JoinedPartyId != partyId)
            return false;
        ++matchingPlayers;
    }

    if (aRequireCompleteRoster)
    {
        const std::optional<CampaignSnapshotData> snapshot = m_admission.BuildSnapshot(acCampaign);
        if (!snapshot || snapshot->Roster.size() != matchingPlayers)
            return false;
        for (const CampaignPublicSlotData& slot : snapshot->Roster)
        {
            if (!slot.Present)
                return false;
        }
        const PartyService::Party* const pParty =
            m_world.GetPartyService().GetById(*partyId);
        if (!pParty || pParty->Members.size() != matchingPlayers)
            return false;
    }
    return matchingPlayers > 0;
}

void CampaignProtocolService::SendResult(Player& aPlayer, const CommandResult& acResult) const noexcept
{
    CampaignCommandResponse response;
    response.Operation = acResult.Operation;
    response.Result = acResult.Result;
    response.MutationId = acResult.MutationId.c_str();
    response.CampaignId = acResult.CampaignId.c_str();
    response.StateVersion = acResult.Version;
    response.CampaignSlotId = acResult.CampaignSlotId.c_str();
    response.CharacterBindingId = acResult.CharacterBindingId.c_str();
    response.JoinCode = acResult.JoinCode.c_str();
    aPlayer.Send(response);
}

void CampaignProtocolService::BroadcastSnapshot(const CampaignSnapshotData& acSnapshot) const noexcept
{
    NotifyCampaignSnapshot notification;
    notification.Snapshot = acSnapshot;
    const auto connections = m_admission.GetAdmittedConnections(CampaignId{acSnapshot.CampaignId.c_str()});
    for (CampaignConnectionHandle connection : connections)
    {
        auto* const pPlayer = m_world.GetPlayerManager().GetById(static_cast<std::uint32_t>(connection));
        if (pPlayer)
            pPlayer->Send(notification);
    }
}

void CampaignProtocolService::BroadcastLobbyState(
    const CampaignId& acCampaign) noexcept
{
    const CampaignLobbyAlias* const pLobby =
        m_lobbies.FindByCampaign(acCampaign.Value);
    const std::optional<CampaignSnapshotData> snapshot =
        m_admission.BuildSnapshot(acCampaign);
    if (!pLobby || !snapshot)
        return;
    if (snapshot->RosterSealed)
    {
        m_lobbies.Invalidate(acCampaign.Value);
        return;
    }
    if (snapshot->Roster.empty())
    {
        m_lobbies.Invalidate(acCampaign.Value);
        return;
    }

    NotifyCampaignLobbyState base;
    base.JoinCode = pLobby->JoinCode.c_str();
    base.CampaignId = acCampaign.Value.c_str();
    base.StateVersion = snapshot->StateVersion;
    for (const CampaignPublicSlotData& slot : snapshot->Roster)
    {
        const auto name = pLobby->DisplayNames.find(slot.PlayerId.c_str());
        if (name == pLobby->DisplayNames.end())
        {
            spdlog::error(
                "[STRE][CampaignLobby] presentation name unavailable campaign={} revision={}",
                acCampaign.Value, snapshot->StateVersion);
            return;
        }
        base.Members.push_back({name->second.c_str(), slot.Present});
    }

    const auto connections = m_admission.GetAdmittedConnections(acCampaign);
    for (CampaignConnectionHandle connection : connections)
    {
        Player* const pPlayer = m_world.GetPlayerManager().GetById(
            static_cast<std::uint32_t>(connection));
        if (!pPlayer)
            continue;
        NotifyCampaignLobbyState notification = base;
        notification.CanStart =
            m_world.GetPartyService().IsPlayerLeader(pPlayer) &&
            SharesCampaignParty(*pPlayer, acCampaign, true);
        pPlayer->Send(notification);
    }
}

void CampaignProtocolService::Finish(
    Player& aPlayer,
    CommandResult aResult,
    std::string_view acMutationId) noexcept
{
    aResult.MutationId = acMutationId;
    const auto* const pAdmission = GetAdmission(aPlayer);
    const char* const pLevel = aResult.Succeeded() ? "accepted" : "rejected";
    spdlog::info(
        "[STRE][CampaignProtocol] command={} operation={} result={} transientPlayer={} durablePlayer={} campaign={} revision={}", pLevel, static_cast<unsigned>(aResult.Operation),
        static_cast<unsigned>(aResult.Result), aPlayer.GetId(), pAdmission ? pAdmission->Player.Value : "unregistered", aResult.CampaignId, aResult.Version);
    SendResult(aPlayer, aResult);
    if (aResult.Snapshot)
    {
        BroadcastSnapshot(*aResult.Snapshot);
        BroadcastLobbyState(CampaignId{aResult.Snapshot->CampaignId.c_str()});
    }
}

void CampaignProtocolService::OnCreate(const PacketEvent<CampaignCreateRequest>& acPacket) noexcept
{
    TiltedPhoques::String displayName;
    if (!acPacket.Packet.IsValid() ||
        !NormalizeCampaignLobbyDisplayName(
            std::string_view(acPacket.Packet.DisplayName.data(),
                acPacket.Packet.DisplayName.size()),
            displayName))
    {
        Finish(*acPacket.pPlayer, {CampaignProtocolOperation::Create, CampaignProtocolResult::InvalidRequest}, acPacket.Packet.MutationId.c_str());
        return;
    }
    const std::optional<PartyService::CampaignLeaderParty> leaderParty =
        m_world.GetPartyService().EnsureCampaignLeaderParty(acPacket.pPlayer);
    if (!leaderParty)
    {
        Finish(*acPacket.pPlayer,
            {CampaignProtocolOperation::Create,
             CampaignProtocolResult::PartyAlignmentFailed},
            acPacket.Packet.MutationId.c_str());
        return;
    }

    CommandResult result = m_admission.CreateCampaign(
        ToHandle(*acPacket.pPlayer), acPacket.Packet.MutationId.c_str(), true);
    if (!result.Succeeded())
    {
        m_world.GetPartyService().RollbackCampaignLeaderParty(
            acPacket.pPlayer, *leaderParty);
    }
    else
    {
        const auto code = m_lobbies.Allocate(
            result.CampaignId, leaderParty->PartyId);
        if (code)
        {
            result.JoinCode = *code;
            if (const CampaignAdmissionRecord* const pAdmission =
                    GetAdmission(*acPacket.pPlayer))
            {
                m_lobbies.RememberDisplayName(
                    result.CampaignId,
                    pAdmission->Player.Value,
                    displayName.c_str());
            }
        }
        else
        {
            spdlog::error(
                "[STRE][CampaignLobby] join-code allocation exhausted campaign={}",
                result.CampaignId);
            CommandResult rollback = m_admission.LeaveCampaign(
                ToHandle(*acPacket.pPlayer), result.CampaignId,
                "bootstrap-code-rollback", result.Version);
            if (rollback.Succeeded())
            {
                m_world.GetPartyService().RollbackCampaignLeaderParty(
                    acPacket.pPlayer, *leaderParty);
                result = {};
                result.Operation = CampaignProtocolOperation::Create;
                result.Result =
                    CampaignProtocolResult::JoinCodeUnavailable;
            }
            else
            {
                spdlog::critical(
                    "[STRE][CampaignLobby] failed to compensate join-code allocation campaign={} result={}",
                    result.CampaignId,
                    static_cast<unsigned>(rollback.Result));
                result.Result = CampaignProtocolResult::PersistenceFailure;
                result.Snapshot.reset();
            }
        }
    }
    Finish(*acPacket.pPlayer, std::move(result),
        acPacket.Packet.MutationId.c_str());
}

void CampaignProtocolService::OnJoin(const PacketEvent<CampaignJoinRequest>& acPacket) noexcept
{
    if (!acPacket.Packet.IsValid())
    {
        Finish(*acPacket.pPlayer, {CampaignProtocolOperation::Join, CampaignProtocolResult::InvalidRequest}, acPacket.Packet.MutationId.c_str());
        return;
    }
    const CampaignId campaign{acPacket.Packet.CampaignId.c_str()};
    Finish(
        *acPacket.pPlayer,
        m_admission.JoinCampaign(
            ToHandle(*acPacket.pPlayer), campaign.Value, acPacket.Packet.MutationId.c_str(), acPacket.Packet.ExpectedRevision,
            SharesCampaignParty(*acPacket.pPlayer, campaign, false)),
        acPacket.Packet.MutationId.c_str());
}

void CampaignProtocolService::OnJoinByCode(
    const PacketEvent<CampaignJoinByCodeRequest>& acPacket) noexcept
{
    Player& player = *acPacket.pPlayer;
    if (!IsValidCampaignWireId(acPacket.Packet.MutationId))
    {
        Finish(player,
            {CampaignProtocolOperation::JoinByCode,
             CampaignProtocolResult::InvalidJoinCode},
            acPacket.Packet.MutationId.c_str());
        return;
    }

    TiltedPhoques::String normalizedCode;
    TiltedPhoques::String displayName;
    if (!NormalizeCampaignJoinCode(
            std::string_view(acPacket.Packet.JoinCode.data(),
                acPacket.Packet.JoinCode.size()),
            normalizedCode))
    {
        Finish(player,
            {CampaignProtocolOperation::JoinByCode,
             CampaignProtocolResult::InvalidRequest},
            acPacket.Packet.MutationId.c_str());
        return;
    }
    if (!NormalizeCampaignLobbyDisplayName(
            std::string_view(acPacket.Packet.DisplayName.data(),
                acPacket.Packet.DisplayName.size()),
            displayName))
    {
        Finish(player,
            {CampaignProtocolOperation::JoinByCode,
             CampaignProtocolResult::InvalidRequest},
            acPacket.Packet.MutationId.c_str());
        return;
    }

    const CampaignLobbyAlias* const pLobby =
        m_lobbies.Resolve(normalizedCode.c_str());
    if (!pLobby)
    {
        Finish(player,
            {CampaignProtocolOperation::JoinByCode,
             CampaignProtocolResult::CampaignNotFound},
            acPacket.Packet.MutationId.c_str());
        return;
    }

    const CampaignId campaign{pLobby->CampaignId};
    CampaignLobbyAlias* const pMutableLobby =
        m_lobbies.FindByCampaign(campaign.Value);
    if (!pMutableLobby)
    {
        Finish(player,
            {CampaignProtocolOperation::JoinByCode,
             CampaignProtocolResult::CampaignNotFound},
            acPacket.Packet.MutationId.c_str());
        return;
    }
    const CampaignAdmissionRecord* const pExisting = GetAdmission(player);
    if (!pExisting)
    {
        Finish(player,
            {CampaignProtocolOperation::JoinByCode,
             CampaignProtocolResult::NotAdmitted,
             {}, campaign.Value},
            acPacket.Packet.MutationId.c_str());
        return;
    }

    if (pExisting->AdmittedIdentity)
    {
        if (pExisting->AdmittedIdentity->Campaign != campaign)
        {
            Finish(player,
                {CampaignProtocolOperation::JoinByCode,
                 CampaignProtocolResult::NotAdmitted,
                 {}, campaign.Value},
                acPacket.Packet.MutationId.c_str());
            return;
        }

        CommandResult replay;
        replay.Operation = CampaignProtocolOperation::JoinByCode;
        replay.Result = CampaignProtocolResult::AcceptedNoOp;
        replay.CampaignId = campaign.Value;
        replay.CampaignSlotId = pExisting->AdmittedIdentity->Slot.Value;
        replay.CharacterBindingId =
            pExisting->AdmittedIdentity->CharacterBinding.Value;
        replay.JoinCode = normalizedCode.c_str();
        replay.Snapshot = m_admission.BuildSnapshot(campaign);
        replay.Version = replay.Snapshot
            ? replay.Snapshot->StateVersion
            : 0;
        m_lobbies.RememberDisplayName(
            campaign.Value, pExisting->Player.Value, displayName.c_str());
        Finish(player, std::move(replay),
            acPacket.Packet.MutationId.c_str());
        return;
    }

    const std::optional<CampaignSnapshotData> snapshot =
        m_admission.BuildSnapshot(campaign);
    if (!snapshot || snapshot->RosterSealed)
    {
        m_lobbies.Invalidate(campaign.Value);
        Finish(player,
            {CampaignProtocolOperation::JoinByCode,
             snapshot
                 ? CampaignProtocolResult::RosterSealed
                 : CampaignProtocolResult::CampaignNotFound,
             {}, campaign.Value},
            acPacket.Packet.MutationId.c_str());
        return;
    }

    PartyService::CampaignAlignment alignment =
        m_world.GetPartyService().AlignPlayerWithCampaignParty(
            &player, pMutableLobby->PartyId);
    if (alignment.Result ==
            PartyService::CampaignAlignmentResult::PartyNotFound &&
        std::none_of(
            snapshot->Roster.begin(), snapshot->Roster.end(),
            [](const CampaignPublicSlotData& acSlot)
            {
                return acSlot.Present;
            }))
    {
        const auto recreated =
            m_world.GetPartyService().EnsureCampaignLeaderParty(&player);
        if (recreated && recreated->Created)
        {
            pMutableLobby->PartyId = recreated->PartyId;
            alignment = {
                PartyService::CampaignAlignmentResult::Added,
                recreated->PartyId};
            spdlog::info(
                "[STRE][CampaignLobby] recreated empty transient party campaign={} code={} party={}",
                campaign.Value, normalizedCode.c_str(), recreated->PartyId);
        }
    }
    if (!alignment.Succeeded())
    {
        spdlog::warn(
            "[STRE][CampaignLobby] party alignment rejected campaign={} code={} player={} reason={}",
            campaign.Value, normalizedCode.c_str(), player.GetId(),
            static_cast<unsigned>(alignment.Result));
        Finish(player,
            {CampaignProtocolOperation::JoinByCode,
             CampaignProtocolResult::PartyAlignmentFailed,
             {}, campaign.Value},
            acPacket.Packet.MutationId.c_str());
        return;
    }

    CommandResult result = m_admission.JoinCampaign(
        ToHandle(player), campaign.Value,
        acPacket.Packet.MutationId.c_str(),
        snapshot->StateVersion, true);
    result.Operation = CampaignProtocolOperation::JoinByCode;
    result.JoinCode = normalizedCode.c_str();

    if (result.Result ==
        CampaignProtocolResult::ExistingMembershipRequiresResume)
    {
        const CampaignConnectionHandle connection = ToHandle(player);
        const auto pending = m_pendingResumeAlignments.find(connection);
        if (pending == m_pendingResumeAlignments.end())
        {
            m_pendingResumeAlignments.emplace(connection,
                PendingResumeAlignment{
                    alignment.PartyId, alignment.WasAdded(),
                    displayName.c_str()});
        }
        else if (pending->second.PartyId == alignment.PartyId)
        {
            pending->second.Added =
                pending->second.Added || alignment.WasAdded();
            pending->second.DisplayName = displayName.c_str();
        }
        else
        {
            spdlog::error(
                "[STRE][CampaignLobby] conflicting pending resume alignment player={} existingParty={} requestedParty={}",
                player.GetId(), pending->second.PartyId, alignment.PartyId);
            m_world.GetPartyService().RollbackCampaignPartyAlignment(
                &player, alignment);
            result.Result = CampaignProtocolResult::PartyAlignmentFailed;
            result.Snapshot.reset();
        }
    }
    else if (!result.Succeeded())
    {
        m_world.GetPartyService().RollbackCampaignPartyAlignment(
            &player, alignment);
    }
    else
    {
        if (const CampaignAdmissionRecord* const pAdmission =
                GetAdmission(player))
        {
            m_lobbies.RememberDisplayName(
                campaign.Value, pAdmission->Player.Value,
                displayName.c_str());
        }
    }

    Finish(player, std::move(result),
        acPacket.Packet.MutationId.c_str());
}

void CampaignProtocolService::OnResume(
    const PacketEvent<CampaignResumeRequest>& acPacket) noexcept
{
    Player& player = *acPacket.pPlayer;
    const auto pending = m_pendingResumeAlignments.find(ToHandle(player));
    std::string pendingDisplayName;
    if (!acPacket.Packet.IsValid())
    {
        if (pending != m_pendingResumeAlignments.end())
        {
            if (pending->second.Added)
            {
                m_world.GetPartyService().RollbackCampaignPartyAlignment(
                    &player,
                    {PartyService::CampaignAlignmentResult::Added,
                     pending->second.PartyId});
            }
            m_pendingResumeAlignments.erase(pending);
        }
        Finish(player,
            {CampaignProtocolOperation::Resume,
             CampaignProtocolResult::InvalidRequest});
        return;
    }
    CommandResult result = m_admission.ResumeCampaign(
        ToHandle(player), acPacket.Packet.CampaignId.c_str(),
        acPacket.Packet.CharacterBindingId.c_str());
    if (pending != m_pendingResumeAlignments.end())
    {
        pendingDisplayName = pending->second.DisplayName;
        if (!result.Succeeded() && pending->second.Added)
        {
            m_world.GetPartyService().RollbackCampaignPartyAlignment(
                &player,
                {PartyService::CampaignAlignmentResult::Added,
                 pending->second.PartyId});
        }
        m_pendingResumeAlignments.erase(pending);
    }
    if (result.Succeeded())
    {
        if (CampaignLobbyAlias* const pLobby =
                m_lobbies.FindByCampaign(result.CampaignId))
        {
            result.JoinCode = pLobby->JoinCode;
            if (const CampaignAdmissionRecord* const pAdmission =
                    GetAdmission(player))
            {
                const bool alreadyProjected = pLobby->DisplayNames.contains(
                    pAdmission->Player.Value);
                if (!pendingDisplayName.empty() || !alreadyProjected)
                {
                    m_lobbies.RememberDisplayName(
                        result.CampaignId, pAdmission->Player.Value,
                        pendingDisplayName.empty()
                            ? FallbackDisplayName(
                                  player.GetUsername().c_str())
                            : pendingDisplayName);
                }
            }
        }
    }
    Finish(player, std::move(result));
}

void CampaignProtocolService::OnStart(const PacketEvent<CampaignStartRequest>& acPacket) noexcept
{
    if (!acPacket.Packet.IsValid())
    {
        Finish(*acPacket.pPlayer, {CampaignProtocolOperation::Start, CampaignProtocolResult::InvalidRequest}, acPacket.Packet.MutationId.c_str());
        return;
    }
    const CampaignId campaign{acPacket.Packet.CampaignId.c_str()};
    CommandResult result = m_admission.StartCampaign(
        ToHandle(*acPacket.pPlayer), campaign.Value,
        acPacket.Packet.MutationId.c_str(),
        acPacket.Packet.ExpectedRevision,
        m_world.GetPartyService().IsPlayerLeader(acPacket.pPlayer),
        SharesCampaignParty(*acPacket.pPlayer, campaign, true));
    if (result.Succeeded())
        m_lobbies.Invalidate(campaign.Value);
    Finish(*acPacket.pPlayer, std::move(result),
        acPacket.Packet.MutationId.c_str());
}

void CampaignProtocolService::OnSetReady(const PacketEvent<CampaignSetReadyRequest>& acPacket) noexcept
{
    if (!acPacket.Packet.IsValid())
    {
        Finish(*acPacket.pPlayer, {CampaignProtocolOperation::SetReady, CampaignProtocolResult::InvalidRequest}, acPacket.Packet.MutationId.c_str());
        return;
    }
    Finish(
        *acPacket.pPlayer,
        m_admission.SetReady(
            ToHandle(*acPacket.pPlayer), acPacket.Packet.CampaignId.c_str(), acPacket.Packet.MutationId.c_str(), acPacket.Packet.ExpectedRevision, acPacket.Packet.Ready),
        acPacket.Packet.MutationId.c_str());
}

void CampaignProtocolService::OnLeave(const PacketEvent<CampaignLeaveRequest>& acPacket) noexcept
{
    if (!acPacket.Packet.IsValid())
    {
        Finish(*acPacket.pPlayer, {CampaignProtocolOperation::Leave, CampaignProtocolResult::InvalidRequest}, acPacket.Packet.MutationId.c_str());
        return;
    }
    const CampaignAdmissionRecord* const pAdmission =
        GetAdmission(*acPacket.pPlayer);
    const std::string playerId = pAdmission
        ? pAdmission->Player.Value
        : std::string{};
    CommandResult result = m_admission.LeaveCampaign(
        ToHandle(*acPacket.pPlayer), acPacket.Packet.CampaignId.c_str(),
        acPacket.Packet.MutationId.c_str(),
        acPacket.Packet.ExpectedRevision);
    if (result.Succeeded() && !playerId.empty())
    {
        m_lobbies.ForgetDisplayName(
            acPacket.Packet.CampaignId.c_str(), playerId);
    }
    Finish(*acPacket.pPlayer, std::move(result),
        acPacket.Packet.MutationId.c_str());
}

void CampaignProtocolService::OnHelgenInvestigationReady(const PacketEvent<CampaignHelgenInvestigationReadyRequest>& acPacket) noexcept
{
    Player& player = *acPacket.pPlayer;
    const CampaignAdmissionRecord* const pAdmission = GetAdmission(player);
    if (!pAdmission || !pAdmission->AdmittedIdentity)
        return;

    const CampaignId campaign = pAdmission->AdmittedIdentity->Campaign;
    const std::optional<CampaignSnapshotData> snapshot = m_admission.BuildSnapshot(campaign);
    if (!snapshot || !snapshot->RosterSealed || snapshot->RuntimeState != static_cast<std::uint8_t>(CampaignRuntimeState::ACTIVE))
    {
        return;
    }

    const std::vector<CampaignConnectionHandle> connections = m_admission.GetAdmittedConnections(campaign);
    if (connections.size() != snapshot->Roster.size() ||
        std::any_of(snapshot->Roster.begin(), snapshot->Roster.end(), [](const CampaignPublicSlotData& acSlot) { return !acSlot.Present; }))
    {
        return;
    }

    auto& ready = m_helgenReadyConnections[campaign.Value];
    const auto [readyIterator, firstAnnouncement] = ready.insert(ToHandle(player));
    (void)readyIterator;
    spdlog::log(
        firstAnnouncement ? spdlog::level::info : spdlog::level::debug, "[STRE][Helgen] investigation readiness observed campaign={} player={} firstAnnouncement={} ready={}/{}",
        campaign.Value, player.GetId(), firstAnnouncement, ready.size(), snapshot->Roster.size());

    if (m_helgenStartedCampaigns.contains(campaign.Value))
    {
        BroadcastHelgenState(campaign, &player);
        return;
    }

    const bool allReady = std::all_of(connections.begin(), connections.end(), [&](CampaignConnectionHandle aConnection) { return ready.contains(aConnection); });
    if (!allReady)
        return;

    m_helgenStartedCampaigns.insert(campaign.Value);
    spdlog::info("[STRE][Helgen] collective investigation start authorized campaign={} roster={}", campaign.Value, connections.size());
    BroadcastHelgenState(campaign);
}

void CampaignProtocolService::BroadcastHelgenState(const CampaignId& acCampaign, Player* apOnlyPlayer) noexcept
{
    const std::optional<CampaignSnapshotData> snapshot = m_admission.BuildSnapshot(acCampaign);
    const std::vector<CampaignConnectionHandle> connections = m_admission.GetAdmittedConnections(acCampaign);

    const bool gateOpen = snapshot && snapshot->RosterSealed && snapshot->RuntimeState == static_cast<std::uint8_t>(CampaignRuntimeState::ACTIVE) &&
                          snapshot->Roster.size() == connections.size() &&
                          std::all_of(snapshot->Roster.begin(), snapshot->Roster.end(), [](const CampaignPublicSlotData& acSlot) { return acSlot.Present; });

    std::vector<STRE::Spatial::MemberPosition> members;
    members.reserve(connections.size());
    for (CampaignConnectionHandle connection : connections)
    {
        const Player* const pMember = m_world.GetPlayerManager().GetById(static_cast<std::uint32_t>(connection));
        if (!pMember)
        {
            members.push_back({false, std::nullopt});
            continue;
        }

        const CellIdComponent& cell = pMember->GetCellComponent();
        members.push_back({true, cell ? std::optional<GameId>(cell.Cell) : std::nullopt});
    }

    const STRE::Spatial::Evaluation evaluation = STRE::Spatial::EvaluateGroupSpatialCondition(members, BuildHelgenFootprint(m_world), STRE::Spatial::GroupOperator::None, gateOpen);

    NotifyCampaignHelgenState notification;
    notification.InvestigationStartAuthorized = m_helgenStartedCampaigns.contains(acCampaign.Value);
    notification.SpatialStatus = ToWireStatus(evaluation.Status);
    notification.AllRequiredPlayersOutside = evaluation.ConditionMet;

    spdlog::log(
        apOnlyPlayer ? spdlog::level::debug : spdlog::level::info, "[STRE][Helgen] spatial predicate campaign={} gateOpen={} status={} inside={}/{} allOutside={}",
        acCampaign.Value, gateOpen, static_cast<unsigned>(evaluation.Status), evaluation.InsideCount, evaluation.RelevantCount, evaluation.ConditionMet);

    if (apOnlyPlayer)
    {
        apOnlyPlayer->Send(notification);
        return;
    }

    for (CampaignConnectionHandle connection : connections)
    {
        Player* const pPlayer = m_world.GetPlayerManager().GetById(static_cast<std::uint32_t>(connection));
        if (pPlayer)
            pPlayer->Send(notification);
    }
}

void CampaignProtocolService::OnPlayerLeave(const PlayerLeaveEvent& acEvent) noexcept
{
    std::optional<CampaignId> campaign;
    if (const CampaignAdmissionRecord* const pAdmission = GetAdmission(*acEvent.pPlayer); pAdmission && pAdmission->AdmittedIdentity)
    {
        campaign = pAdmission->AdmittedIdentity->Campaign;
        auto ready = m_helgenReadyConnections.find(campaign->Value);
        if (ready != m_helgenReadyConnections.end())
            ready->second.erase(ToHandle(*acEvent.pPlayer));
    }

    m_pendingResumeAlignments.erase(ToHandle(*acEvent.pPlayer));
    const std::optional<CampaignSnapshotData> snapshot = m_admission.Disconnect(ToHandle(*acEvent.pPlayer));
    if (snapshot)
    {
        BroadcastSnapshot(*snapshot);
        if (campaign)
            BroadcastLobbyState(*campaign);
    }
    if (campaign && m_helgenStartedCampaigns.contains(campaign->Value))
        BroadcastHelgenState(*campaign);
}
