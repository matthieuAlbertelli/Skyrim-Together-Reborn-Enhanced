#include <Services/CampaignProtocolService.h>

#include <GameServer.h>
#include <World.h>

#include <Events/PlayerLeaveEvent.h>
#include <Messages/CampaignMessages.h>
#include <Messages/CampaignRequests.h>
#include <Components.h>
#include <GroupSpatialCondition.h>
#include <Structs/NativeSaveBundle.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

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

const char* RecoveryRecipientErrorName(
    CampaignRecoveryRecipientError aError) noexcept
{
    switch (aError)
    {
    case CampaignRecoveryRecipientError::None: return "none";
    case CampaignRecoveryRecipientError::InvalidCheckpointRoster:
        return "invalid-checkpoint-roster";
    case CampaignRecoveryRecipientError::MissingCurrentAdmission:
        return "missing-current-admission";
    case CampaignRecoveryRecipientError::DuplicateCurrentAdmission:
        return "duplicate-current-admission";
    case CampaignRecoveryRecipientError::UnexpectedCurrentAdmission:
        return "unexpected-current-admission";
    }
    return "unknown";
}

std::string RecoverySlotSet(
    const std::unordered_set<std::string>& acSlots)
{
    std::vector<std::string> slots(acSlots.begin(), acSlots.end());
    std::sort(slots.begin(), slots.end());
    std::string result = "[";
    for (std::size_t index = 0; index < slots.size(); ++index)
    {
        if (index != 0)
            result += ',';
        result += slots[index];
    }
    result += ']';
    return result;
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
    , m_checkpointRequestConnection(aDispatcher.sink<PacketEvent<CampaignCheckpointRequest>>().connect<&CampaignProtocolService::OnCheckpointRequest>(this))
    , m_checkpointResultConnection(aDispatcher.sink<PacketEvent<CampaignCheckpointSaveResult>>().connect<&CampaignProtocolService::OnCheckpointSaveResult>(this))
    , m_recoveryLoadedConnection(aDispatcher.sink<PacketEvent<CampaignRecoveryLoadedResult>>().connect<&CampaignProtocolService::OnRecoveryLoadedResult>(this))
    , m_recoverySnapshotAppliedConnection(aDispatcher.sink<PacketEvent<CampaignRecoverySnapshotApplied>>().connect<&CampaignProtocolService::OnRecoverySnapshotApplied>(this))
    , m_playerLeaveConnection(aDispatcher.sink<PlayerLeaveEvent>().connect<&CampaignProtocolService::OnPlayerLeave>(this))
{
}

bool CampaignProtocolService::SendRecoveryLoadRequests(
    const CampaignRecoveryActivity& acActivity,
    const CheckpointRecord& acCheckpoint) noexcept
{
    try
    {
        const auto connections =
            m_admission.GetAdmittedConnections(acActivity.Campaign);
        if (connections.size() != acCheckpoint.Slots.size())
            return false;

        std::vector<std::pair<Player*, CampaignRecoveryLoadRequest>> requests;
        requests.reserve(connections.size());
        for (const CampaignConnectionHandle connection : connections)
        {
            Player* const pPlayer = m_world.GetPlayerManager().GetById(
                static_cast<std::uint32_t>(connection));
            const CampaignAdmissionRecord* const pAdmission =
                std::as_const(m_admission).FindConnection(connection);
            if (!pPlayer || !pAdmission || !pAdmission->AdmittedIdentity)
                return false;

            const CampaignMemberIdentity& identity =
                *pAdmission->AdmittedIdentity;
            const auto slot = std::find_if(
                acCheckpoint.Slots.begin(), acCheckpoint.Slots.end(),
                [&](const CheckpointSlotRecord& acSlot)
                {
                    return acSlot.Slot == identity.Slot &&
                        acSlot.Player == identity.Player &&
                        acSlot.CharacterBinding == identity.CharacterBinding;
                });
            if (slot == acCheckpoint.Slots.end() ||
                !slot->NativeSaveIdentity || !slot->FingerprintAlgorithm ||
                !slot->FingerprintVersion ||
                !slot->SaveMetadataCodecVersion)
            {
                return false;
            }

            CampaignRecoveryLoadRequest request;
            request.CampaignId = acActivity.Campaign.Value.c_str();
            request.RestoreAttemptId = acActivity.Attempt.Value.c_str();
            request.CheckpointId = acCheckpoint.Id.Value.c_str();
            request.SourceRevision = acCheckpoint.SourceRevision;
            request.CampaignSlotId = slot->Slot.Value.c_str();
            request.CharacterBindingId =
                slot->CharacterBinding.Value.c_str();
            request.NativeSaveIdentity = slot->NativeSaveIdentity->c_str();
            request.FingerprintAlgorithm =
                slot->FingerprintAlgorithm->c_str();
            request.FingerprintVersion = *slot->FingerprintVersion;
            request.Fingerprint.assign(
                slot->Fingerprint.begin(), slot->Fingerprint.end());
            request.SaveMetadataCodecVersion =
                *slot->SaveMetadataCodecVersion;
            request.SaveMetadata.assign(
                slot->SaveMetadata.begin(), slot->SaveMetadata.end());
            if (!request.IsValid())
                return false;
            requests.emplace_back(pPlayer, std::move(request));
        }

        for (const auto& [pPlayer, request] : requests)
            pPlayer->Send(request);
        spdlog::info(
            "[STRE][CampaignRecovery] LOAD_BARRIER_DISPATCHED campaign={} attempt={} checkpoint={} sourceRevision={} restoreRevision={} members={} action={}",
            acActivity.Campaign.Value,
            acActivity.Attempt.Value,
            acCheckpoint.Id.Value,
            acCheckpoint.SourceRevision,
            acActivity.RestoreRevision.value_or(0),
            requests.size(),
            acActivity.RestoreRevision
                ? "replay-before-restored-snapshot"
                : "start-native-load-barrier");
        return true;
    }
    catch (...)
    {
        return false;
    }
}

bool CampaignProtocolService::SendRecoverySnapshot(
    const CampaignRecoveryActivity& acActivity,
    const CheckpointRecord& acCheckpoint) noexcept
{
    try
    {
        if (!acActivity.Checkpoint || !acActivity.RestoreRevision ||
            *acActivity.Checkpoint != acCheckpoint.Id ||
            acActivity.Campaign != acCheckpoint.Campaign)
        {
            spdlog::error(
                "[STRE][CampaignRecovery] RESTORE_SNAPSHOT_DISPATCH_FAILED campaign={} attempt={} checkpoint={} restoreRevision={} requiredMembers={} resolvedRecipients=0 reason=activity-checkpoint-mismatch",
                acActivity.Campaign.Value,
                acActivity.Attempt.Value,
                acActivity.Checkpoint
                    ? acActivity.Checkpoint->Value : "unknown",
                acActivity.RestoreRevision.value_or(0),
                acCheckpoint.Slots.size());
            return false;
        }

        const CampaignRecoveryRecipientPlan plan =
            m_admission.PrepareRecoveryRecipients(acCheckpoint);
        if (!plan.Succeeded())
        {
            spdlog::error(
                "[STRE][CampaignRecovery] RESTORE_SNAPSHOT_DISPATCH_FAILED campaign={} attempt={} checkpoint={} restoreRevision={} requiredMembers={} resolvedRecipients={} reason={} slot={} durablePlayer={}",
                acActivity.Campaign.Value,
                acActivity.Attempt.Value,
                acActivity.Checkpoint->Value,
                *acActivity.RestoreRevision,
                plan.RequiredMemberCount,
                plan.Recipients.size(),
                RecoveryRecipientErrorName(plan.Error),
                plan.FailedIdentity
                    ? plan.FailedIdentity->Slot.Value : "unknown",
                plan.FailedIdentity
                    ? plan.FailedIdentity->Player.Value : "unknown");
            return false;
        }

        const auto snapshot = m_admission.BuildSnapshot(acActivity.Campaign);
        if (!snapshot)
        {
            spdlog::error(
                "[STRE][CampaignRecovery] RESTORE_SNAPSHOT_DISPATCH_FAILED campaign={} attempt={} checkpoint={} sourceRevision={} restoreRevision={} requiredMembers={} resolvedRecipients={} reason=snapshot-unavailable snapshotLookup=runtime-canonical checkpointSnapshotPresent={} runtimeCanonicalSnapshotPresent=false",
                acActivity.Campaign.Value,
                acActivity.Attempt.Value,
                acActivity.Checkpoint->Value,
                acCheckpoint.SourceRevision,
                *acActivity.RestoreRevision,
                plan.RequiredMemberCount,
                plan.Recipients.size(),
                acCheckpoint.SnapshotCoreStateCodecVersion != 0 &&
                    !acCheckpoint.SnapshotCoreStatePayload.empty());
            return false;
        }

        CampaignRecoverySnapshot message;
        message.CampaignId = acActivity.Campaign.Value.c_str();
        message.RestoreAttemptId = acActivity.Attempt.Value.c_str();
        message.CheckpointId = acActivity.Checkpoint->Value.c_str();
        message.RestoreRevision = *acActivity.RestoreRevision;
        message.Snapshot = *snapshot;
        if (!message.IsValid())
        {
            spdlog::error(
                "[STRE][CampaignRecovery] RESTORE_SNAPSHOT_DISPATCH_FAILED campaign={} attempt={} checkpoint={} restoreRevision={} requiredMembers={} resolvedRecipients={} reason=invalid-message snapshotRevision={} runtimeState={}",
                acActivity.Campaign.Value,
                acActivity.Attempt.Value,
                acActivity.Checkpoint->Value,
                *acActivity.RestoreRevision,
                plan.RequiredMemberCount,
                plan.Recipients.size(),
                snapshot->StateVersion,
                static_cast<unsigned>(snapshot->RuntimeState));
            return false;
        }

        std::vector<Player*> recipients;
        recipients.reserve(plan.Recipients.size());
        for (const CampaignRecoveryRecipient& recipient : plan.Recipients)
        {
            Player* const pPlayer = m_world.GetPlayerManager().GetById(
                static_cast<std::uint32_t>(recipient.Connection));
            if (!pPlayer)
            {
                spdlog::error(
                    "[STRE][CampaignRecovery] RESTORE_SNAPSHOT_DISPATCH_FAILED campaign={} attempt={} checkpoint={} restoreRevision={} requiredMembers={} resolvedRecipients={} reason=transient-player-unavailable slot={} durablePlayer={} transientPlayer={}",
                    acActivity.Campaign.Value,
                    acActivity.Attempt.Value,
                    acActivity.Checkpoint->Value,
                    *acActivity.RestoreRevision,
                    plan.RequiredMemberCount,
                    recipients.size(),
                    recipient.Identity.Slot.Value,
                    recipient.Identity.Player.Value,
                    recipient.Connection);
                return false;
            }
            recipients.push_back(pPlayer);
        }

        spdlog::info(
            "[STRE][CampaignRecovery] RESTORE_SNAPSHOT_DISPATCH_PREPARED campaign={} attempt={} checkpoint={} sourceRevision={} restoreRevision={} requiredMembers={} resolvedRecipients={} snapshotLookup=runtime-canonical checkpointSnapshotPresent=true runtimeCanonicalSnapshotPresent=true",
            acActivity.Campaign.Value,
            acActivity.Attempt.Value,
            acActivity.Checkpoint->Value,
            acCheckpoint.SourceRevision,
            *acActivity.RestoreRevision,
            plan.RequiredMemberCount,
            recipients.size());
        for (Player* const pPlayer : recipients)
            pPlayer->Send(message);
        spdlog::info(
            "[STRE][CampaignRecovery] RESTORE_SNAPSHOT_SENT campaign={} attempt={} checkpoint={} restoreRevision={} members={}",
            acActivity.Campaign.Value,
            acActivity.Attempt.Value,
            acActivity.Checkpoint->Value,
            *acActivity.RestoreRevision,
            recipients.size());
        return true;
    }
    catch (...)
    {
        spdlog::error(
            "[STRE][CampaignRecovery] RESTORE_SNAPSHOT_DISPATCH_FAILED campaign={} attempt={} checkpoint={} restoreRevision={} requiredMembers={} resolvedRecipients=0 reason=exception",
            acActivity.Campaign.Value,
            acActivity.Attempt.Value,
            acActivity.Checkpoint
                ? acActivity.Checkpoint->Value : "unknown",
            acActivity.RestoreRevision.value_or(0),
            acCheckpoint.Slots.size());
        return false;
    }
}

bool CampaignProtocolService::AdvanceRecovery(
    const CampaignId& acCampaign) noexcept
{
    CampaignRecoveryCommandResult prepared =
        m_admission.PrepareRecovery(acCampaign);
    if (!prepared || !prepared.Activity)
    {
        const bool noCheckpoint =
            prepared.Command.Error == CampaignError::NoCommittedCheckpoint;
        spdlog::log(
            noCheckpoint ? spdlog::level::err : spdlog::level::debug,
            "[STRE][CampaignRecovery] PREPARE_BLOCKED campaign={} error={} reason={} state={}",
            acCampaign.Value,
            static_cast<unsigned>(prepared.Command.Error),
            prepared.Command.Message,
            noCheckpoint ? "NO_COMMITTED_CHECKPOINT" : "LOCKED");
        if (const auto snapshot = m_admission.BuildSnapshot(acCampaign))
            BroadcastSnapshot(*snapshot);
        return false;
    }

    if (const auto snapshot = m_admission.BuildSnapshot(acCampaign))
        BroadcastSnapshot(*snapshot);
    if (prepared.Dispatch == CampaignRecoveryDispatch::NativeLoad &&
        prepared.Checkpoint)
    {
        return SendRecoveryLoadRequests(
            *prepared.Activity, *prepared.Checkpoint);
    }
    if (prepared.Dispatch == CampaignRecoveryDispatch::RestoredSnapshot)
    {
        if (!prepared.Checkpoint)
        {
            spdlog::error(
                "[STRE][CampaignRecovery] RESTORE_SNAPSHOT_DISPATCH_FAILED campaign={} attempt={} checkpoint=unknown restoreRevision={} requiredMembers=0 resolvedRecipients=0 reason=checkpoint-unavailable",
                acCampaign.Value,
                prepared.Activity->Attempt.Value,
                prepared.Activity->RestoreRevision.value_or(0));
            return false;
        }
        return SendRecoverySnapshot(
            *prepared.Activity, *prepared.Checkpoint);
    }
    return false;
}

bool CampaignProtocolService::SendCheckpointRequest(
    const CampaignCheckpointActivity& acActivity) noexcept
{
    try
    {
        CampaignCheckpointSaveRequest request;
        request.CampaignId = acActivity.Campaign.Value.c_str();
        request.CheckpointId = acActivity.Checkpoint.Value.c_str();
        request.SourceRevision = acActivity.SourceRevision;
        request.NativeSaveIdentity = acActivity.NativeSaveIdentity.c_str();
        if (!request.IsValid())
            return false;

        const auto connections =
            m_admission.GetAdmittedConnections(acActivity.Campaign);
        const auto snapshot = m_admission.BuildSnapshot(acActivity.Campaign);
        if (!snapshot || connections.size() != snapshot->Roster.size() ||
            std::any_of(
                snapshot->Roster.begin(), snapshot->Roster.end(),
                [](const CampaignPublicSlotData& acSlot)
                {
                    return !acSlot.Present;
                }))
        {
            return false;
        }
        std::vector<Player*> players;
        players.reserve(connections.size());
        for (CampaignConnectionHandle connection : connections)
        {
            Player* const pPlayer = m_world.GetPlayerManager().GetById(
                static_cast<std::uint32_t>(connection));
            if (!pPlayer)
                return false;
            players.push_back(pPlayer);
        }
        for (Player* const pPlayer : players)
            pPlayer->Send(request);
        spdlog::info(
            "[STRE][CampaignCheckpoint] SAVE_REQUEST_SENT campaign={} checkpoint={} sourceRevision={} nativeSaveIdentity={} members={}",
            acActivity.Campaign.Value,
            acActivity.Checkpoint.Value,
            acActivity.SourceRevision,
            acActivity.NativeSaveIdentity,
            connections.size());
        return true;
    }
    catch (...)
    {
        return false;
    }
}

void CampaignProtocolService::SendCheckpointState(
    const CampaignId& acCampaign,
    std::string_view acCheckpointId,
    CampaignCheckpointPublicState aState,
    Player* apOnlyPlayer) noexcept
{
    try
    {
        NotifyCampaignCheckpointState message;
        message.CampaignId = acCampaign.Value.c_str();
        message.CheckpointId.assign(
            acCheckpointId.data(), acCheckpointId.size());
        message.State = aState;
        if (!message.IsValid())
            return;

        if (apOnlyPlayer)
        {
            apOnlyPlayer->Send(message);
            return;
        }
        for (const CampaignConnectionHandle connection :
             m_admission.GetAdmittedConnections(acCampaign))
        {
            if (Player* const pPlayer =
                    m_world.GetPlayerManager().GetById(
                        static_cast<std::uint32_t>(connection)))
            {
                pPlayer->Send(message);
            }
        }
    }
    catch (...)
    {
    }
}

void CampaignProtocolService::OnCheckpointRequest(
    const PacketEvent<CampaignCheckpointRequest>& acPacket) noexcept
{
    Player& player = *acPacket.pPlayer;
    const CampaignAdmissionRecord* const pAdmission = GetAdmission(player);
    if (!acPacket.Packet.IsValid() || !pAdmission ||
        !pAdmission->AdmittedIdentity)
    {
        spdlog::warn(
            "[STRE][CampaignCheckpoint] INTENT_REJECTED transientPlayer={} reason=invalid-or-not-admitted",
            player.GetId());
        return;
    }

    const CampaignId campaign =
        pAdmission->AdmittedIdentity->Campaign;
    const auto snapshot = m_admission.BuildSnapshot(campaign);
    if (!snapshot ||
        (snapshot->RuntimeState != kCampaignWireRuntimeActive &&
         snapshot->RuntimeState != kCampaignWireRuntimeCheckpointing))
    {
        SendCheckpointState(
            campaign, {}, CampaignCheckpointPublicState::Failed, &player);
        spdlog::warn(
            "[STRE][CampaignCheckpoint] INTENT_REJECTED campaign={} transientPlayer={} reason=runtime-not-active runtime={}",
            campaign.Value, player.GetId(),
            snapshot ? static_cast<unsigned>(snapshot->RuntimeState) : 0u);
        return;
    }

    CampaignCheckpointCommandResult begun =
        m_admission.BeginCheckpoint(ToHandle(player));
    if (begun.Command.Error == CampaignError::CheckpointInProgress &&
        begun.Activity)
    {
        SendCheckpointState(
            campaign, begun.Activity->Checkpoint.Value,
            CampaignCheckpointPublicState::Started, &player);
        return;
    }
    if (!begun || !begun.Activity)
    {
        SendCheckpointState(
            campaign, {}, CampaignCheckpointPublicState::Failed, &player);
        spdlog::warn(
            "[STRE][CampaignCheckpoint] INTENT_REJECTED campaign={} transientPlayer={} reasonCode={} reason={}",
            campaign.Value, player.GetId(),
            static_cast<unsigned>(begun.Command.Error),
            begun.Command.Message);
        return;
    }

    if (const auto current = m_admission.BuildSnapshot(campaign))
        BroadcastSnapshot(*current);
    if (!SendCheckpointRequest(*begun.Activity))
    {
        GameServer::Get()->GetCampaignRuntime().AbandonCheckpoint(campaign);
        if (const auto current = m_admission.BuildSnapshot(campaign))
            BroadcastSnapshot(*current);
        SendCheckpointState(
            campaign, begun.Activity->Checkpoint.Value,
            CampaignCheckpointPublicState::Failed);
        return;
    }

    SendCheckpointState(
        campaign, begun.Activity->Checkpoint.Value,
        CampaignCheckpointPublicState::Started);
    spdlog::info(
        "[STRE][CampaignCheckpoint] INTENT_ACCEPTED campaign={} checkpoint={} source={} transientPlayer={}",
        campaign.Value, begun.Activity->Checkpoint.Value,
        acPacket.Packet.Reason == CampaignCheckpointRequestReason::Manual
            ? "manual" : "quick",
        player.GetId());
}

bool CampaignProtocolService::BeginCheckpointDevelopment(
    const std::string& acCampaignId) noexcept
{
    try
    {
        TiltedPhoques::String campaignId(acCampaignId.c_str());
        if (!IsValidCampaignWireId(campaignId))
            return false;
        CampaignCheckpointCommandResult begun =
            m_admission.BeginCheckpoint(CampaignId{acCampaignId});
        if (!begun || !begun.Activity)
        {
            spdlog::warn(
                "[STRE][CampaignCheckpoint] BEGIN_REJECTED campaign={} error={} persistenceError={} reason={}",
                acCampaignId,
                static_cast<unsigned>(begun.Command.Error),
                static_cast<unsigned>(begun.Command.PersistenceError),
                begun.Command.Message);
            return false;
        }
        if (const auto snapshot =
                m_admission.BuildSnapshot(begun.Activity->Campaign))
        {
            BroadcastSnapshot(*snapshot);
        }
        if (!SendCheckpointRequest(*begun.Activity))
        {
            GameServer::Get()->GetCampaignRuntime().AbandonCheckpoint(
                begun.Activity->Campaign);
            if (const auto snapshot =
                    m_admission.BuildSnapshot(begun.Activity->Campaign))
            {
                BroadcastSnapshot(*snapshot);
            }
            spdlog::error(
                "[STRE][CampaignCheckpoint] BEGIN_ABANDONED campaign={} checkpoint={} reason=dispatch-failed",
                begun.Activity->Campaign.Value,
                begun.Activity->Checkpoint.Value);
            return false;
        }
        spdlog::info(
            "[STRE][CampaignCheckpoint] CANDIDATE_CREATED campaign={} checkpoint={} sourceRevision={} candidateRevision={}",
            begun.Activity->Campaign.Value,
            begun.Activity->Checkpoint.Value,
            begun.Activity->SourceRevision,
            begun.Command.Version);
        return true;
    }
    catch (...)
    {
        return false;
    }
}

bool CampaignProtocolService::ResendCheckpointDevelopment(
    const std::string& acCampaignId) noexcept
{
    try
    {
        const auto active = m_admission.GetActiveCheckpoint(
            CampaignId{acCampaignId});
        return active && SendCheckpointRequest(*active);
    }
    catch (...)
    {
        return false;
    }
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
    if (result.Succeeded() &&
        acPacket.Packet.RestoreCommittedCheckpoint &&
        result.Result == CampaignProtocolResult::Applied &&
        result.Snapshot && result.Snapshot->RosterSealed)
    {
        m_pendingCampaignLoads.insert(result.CampaignId);
    }
    if (result.Succeeded() && result.Snapshot &&
        (result.Snapshot->RuntimeState ==
             kCampaignWireRuntimeRecoveryLock ||
         result.Snapshot->RuntimeState ==
             kCampaignWireRuntimeRestoringCheckpoint))
    {
        m_pendingCampaignLoads.erase(result.CampaignId);
    }
    const bool resumed = result.Succeeded();
    const CampaignId resumedCampaign{result.CampaignId};
    if (resumed)
    {
        if (const auto recovery =
                m_admission.GetRecoveryActivity(resumedCampaign))
        {
            spdlog::info(
                "[STRE][CampaignRecovery] RECOVERY_REHYDRATION_STATE campaign={} attempt={} checkpoint={} sourceRevision={} restoreRevision={} persistedPhase={} replayAction={} durableLoadedSlots={} durableAppliedSlots={} volatileLoadedSlots={} volatileAppliedSlots={} durableRestoreApplied={}",
                resumedCampaign.Value,
                recovery->Attempt.Value,
                recovery->Checkpoint
                    ? recovery->Checkpoint->Value : "pending",
                recovery->SourceRevision,
                recovery->RestoreRevision.value_or(0),
                recovery->RestoreRevision
                    ? "restore-applied"
                    : "waiting-loaded",
                recovery->RestoreRevision
                    ? "replay-native-load-before-snapshot"
                    : "resume-native-load-barrier",
                RecoverySlotSet(recovery->DurableLoadedSlots),
                RecoverySlotSet(
                    recovery->DurableSnapshotAppliedSlots),
                recovery->LoadedSlots.size(),
                recovery->SnapshotAppliedSlots.size(),
                recovery->RestoreRevision.has_value());
        }
    }
    bool loadRecoveryBegun = false;
    if (resumed && result.Snapshot &&
        result.Snapshot->RuntimeState == kCampaignWireRuntimeActive &&
        m_pendingCampaignLoads.contains(result.CampaignId))
    {
        CampaignRecoveryCommandResult begun =
            m_admission.BeginCampaignLoadRecovery(
                ToHandle(player), resumedCampaign);
        if (begun)
        {
            loadRecoveryBegun = true;
            m_pendingCampaignLoads.erase(result.CampaignId);
            result.Version = begun.Command.Version;
            result.Snapshot = m_admission.BuildSnapshot(resumedCampaign);
            spdlog::info(
                "[STRE][CampaignRecovery] CAMPAIGN_LOAD_RECOVERY_OPENED campaign={} attempt={} source=ACTIVE",
                resumedCampaign.Value,
                begun.Activity ? begun.Activity->Attempt.Value : "unknown");
        }
        else
        {
            spdlog::error(
                "[STRE][CampaignRecovery] CAMPAIGN_LOAD_RECOVERY_BLOCKED campaign={} error={} reason={}",
                resumedCampaign.Value,
                static_cast<unsigned>(begun.Command.Error),
                begun.Command.Message);
        }
    }
    Finish(player, std::move(result));
    if (resumed || loadRecoveryBegun)
        (void)AdvanceRecovery(resumedCampaign);
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
    {
        spdlog::debug(
            "[STRE][Helgen] readiness rejected transientPlayer={} reason=no-admission",
            player.GetId());
        return;
    }

    const CampaignId campaign = pAdmission->AdmittedIdentity->Campaign;
    const std::optional<CampaignSnapshotData> snapshot = m_admission.BuildSnapshot(campaign);
    if (!snapshot || !snapshot->RosterSealed || snapshot->RuntimeState != static_cast<std::uint8_t>(CampaignRuntimeState::ACTIVE))
    {
        spdlog::debug(
            "[STRE][Helgen] readiness rejected campaign={} transientPlayer={} reason=runtime-gate snapshot={} sealed={} runtime={}",
            campaign.Value, player.GetId(), snapshot.has_value(),
            snapshot && snapshot->RosterSealed,
            snapshot ? static_cast<unsigned>(snapshot->RuntimeState) : 0u);
        return;
    }

    const std::vector<CampaignConnectionHandle> connections = m_admission.GetAdmittedConnections(campaign);
    if (connections.size() != snapshot->Roster.size() ||
        std::any_of(snapshot->Roster.begin(), snapshot->Roster.end(), [](const CampaignPublicSlotData& acSlot) { return !acSlot.Present; }))
    {
        spdlog::debug(
            "[STRE][Helgen] readiness rejected campaign={} transientPlayer={} reason=incomplete-roster connections={} roster={}",
            campaign.Value, player.GetId(), connections.size(),
            snapshot->Roster.size());
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

void CampaignProtocolService::OnCheckpointSaveResult(
    const PacketEvent<CampaignCheckpointSaveResult>& acPacket) noexcept
{
    Player& player = *acPacket.pPlayer;
    if (!acPacket.Packet.IsValid())
    {
        spdlog::warn(
            "[STRE][CampaignCheckpoint] RESULT_REJECTED transientPlayer={} reason=malformed-packet",
            player.GetId());
        return;
    }

    const CampaignId campaign{acPacket.Packet.CampaignId.c_str()};
    const bool succeeded = acPacket.Packet.Result ==
        CampaignCheckpointSaveResultCode::Success;
    Bytes fingerprint(
        acPacket.Packet.Fingerprint.begin(),
        acPacket.Packet.Fingerprint.end());
    Bytes metadata(
        acPacket.Packet.SaveMetadata.begin(),
        acPacket.Packet.SaveMetadata.end());
    CampaignCheckpointCommandResult handled =
        m_admission.HandleCheckpointSaveResult(
            ToHandle(player),
            campaign,
            CheckpointId{acPacket.Packet.CheckpointId.c_str()},
            acPacket.Packet.NativeSaveIdentity.c_str(),
            succeeded,
            acPacket.Packet.FingerprintAlgorithm.c_str(),
            acPacket.Packet.FingerprintVersion,
            std::move(fingerprint),
            acPacket.Packet.SaveMetadataCodecVersion,
            std::move(metadata));
    if (!handled)
    {
        spdlog::warn(
            "[STRE][CampaignCheckpoint] RESULT_REJECTED transientPlayer={} campaign={} checkpoint={} error={} persistenceError={} reason={}",
            player.GetId(),
            campaign.Value,
            acPacket.Packet.CheckpointId.c_str(),
            static_cast<unsigned>(handled.Command.Error),
            static_cast<unsigned>(handled.Command.PersistenceError),
            handled.Command.Message);
        return;
    }

    const CampaignAdmissionRecord* const pAdmission = GetAdmission(player);
    std::string fingerprintHex;
    if (succeeded)
    {
        NativeSaveSha256 digest{};
        std::copy(
            acPacket.Packet.Fingerprint.begin(),
            acPacket.Packet.Fingerprint.end(),
            digest.begin());
        fingerprintHex = NativeSaveSha256ToHex(digest);
    }
    spdlog::info(
        "[STRE][CampaignCheckpoint] RESULT_ACCEPTED campaign={} checkpoint={} slot={} player={} success={} fingerprint={} revision={} replay={} committed={}",
        campaign.Value,
        acPacket.Packet.CheckpointId.c_str(),
        pAdmission && pAdmission->AdmittedIdentity
            ? pAdmission->AdmittedIdentity->Slot.Value
            : "unavailable",
        pAdmission ? pAdmission->Player.Value : "unavailable",
        succeeded,
        succeeded ? fingerprintHex : "none",
        handled.Command.Version,
        handled.Command.IdempotentReplay,
        handled.Committed);
    if (handled.Committed)
    {
        spdlog::info(
            "[STRE][CampaignCheckpoint] CHECKPOINT_COMMITTED campaign={} checkpoint={} revision={}",
            campaign.Value,
            acPacket.Packet.CheckpointId.c_str(),
            handled.Command.Version);
        SendCheckpointState(
            campaign, acPacket.Packet.CheckpointId.c_str(),
            CampaignCheckpointPublicState::Committed);
    }
    else if (!succeeded)
    {
        SendCheckpointState(
            campaign, acPacket.Packet.CheckpointId.c_str(),
            CampaignCheckpointPublicState::Failed);
    }
    if (const auto snapshot = m_admission.BuildSnapshot(campaign))
        BroadcastSnapshot(*snapshot);
}

void CampaignProtocolService::OnRecoveryLoadedResult(
    const PacketEvent<CampaignRecoveryLoadedResult>& acPacket) noexcept
{
    Player& player = *acPacket.pPlayer;
    if (!acPacket.Packet.IsValid())
    {
        spdlog::warn(
            "[STRE][CampaignRecovery] LOAD_RESULT_REJECTED transientPlayer={} reason=malformed-packet",
            player.GetId());
        return;
    }

    const CampaignId campaign{acPacket.Packet.CampaignId.c_str()};
    spdlog::info(
        "[STRE][CampaignRecovery] LOAD_RESULT_RECEIVED campaign={} attempt={} checkpoint={} transientPlayer={} success={}",
        campaign.Value,
        acPacket.Packet.RestoreAttemptId.c_str(),
        acPacket.Packet.CheckpointId.c_str(),
        player.GetId(),
        acPacket.Packet.Result ==
            CampaignRecoveryLoadedResultCode::Success);
    CampaignRecoveryCommandResult handled =
        m_admission.HandleRecoveryLoaded(
            ToHandle(player),
            campaign,
            RestoreAttemptId{
                acPacket.Packet.RestoreAttemptId.c_str()},
            CheckpointId{acPacket.Packet.CheckpointId.c_str()},
            acPacket.Packet.Result ==
                CampaignRecoveryLoadedResultCode::Success,
            acPacket.Packet.NativeSaveIdentity.c_str(),
            acPacket.Packet.FingerprintAlgorithm.c_str(),
            acPacket.Packet.FingerprintVersion,
            Bytes(acPacket.Packet.Fingerprint.begin(),
                acPacket.Packet.Fingerprint.end()),
            acPacket.Packet.SaveMetadataCodecVersion,
            Bytes(acPacket.Packet.SaveMetadata.begin(),
                acPacket.Packet.SaveMetadata.end()));
    if (!handled)
    {
        spdlog::warn(
            "[STRE][CampaignRecovery] LOAD_RESULT_REJECTED campaign={} attempt={} checkpoint={} transientPlayer={} error={} reason={}",
            campaign.Value,
            acPacket.Packet.RestoreAttemptId.c_str(),
            acPacket.Packet.CheckpointId.c_str(),
            player.GetId(),
            static_cast<unsigned>(handled.Command.Error),
            handled.Command.Message);
        return;
    }

    spdlog::info(
        "[STRE][CampaignRecovery] LOAD_ACK campaign={} attempt={} checkpoint={} transientPlayer={} firstBarrier={} replay={}",
        campaign.Value,
        acPacket.Packet.RestoreAttemptId.c_str(),
        acPacket.Packet.CheckpointId.c_str(),
        player.GetId(),
        handled.FirstBarrierCompleted,
        handled.Command.IdempotentReplay);
    if (handled.Dispatch == CampaignRecoveryDispatch::RestoredSnapshot &&
        handled.Activity)
    {
        spdlog::info(
            "[STRE][CampaignRecovery] LOADED_BARRIER_COMPLETE campaign={} attempt={} checkpoint={} restoreRevision={} members={} replay={}",
            campaign.Value,
            handled.Activity->Attempt.Value,
            handled.Activity->Checkpoint
                ? handled.Activity->Checkpoint->Value : "unknown",
            handled.Activity->RestoreRevision.value_or(0),
            handled.Activity->LoadedSlots.size(),
            handled.Command.IdempotentReplay);
        if (!handled.Command.IdempotentReplay)
        {
            spdlog::info(
                "[STRE][CampaignRecovery] RESTORE_APPLIED campaign={} attempt={} checkpoint={} restoreRevision={}",
                campaign.Value,
                handled.Activity->Attempt.Value,
                handled.Activity->Checkpoint
                    ? handled.Activity->Checkpoint->Value : "unknown",
                handled.Activity->RestoreRevision.value_or(0));
        }
        if (const auto snapshot = m_admission.BuildSnapshot(campaign))
            BroadcastSnapshot(*snapshot);
        if (handled.Checkpoint)
        {
            (void)SendRecoverySnapshot(
                *handled.Activity, *handled.Checkpoint);
        }
        else
        {
            spdlog::error(
                "[STRE][CampaignRecovery] RESTORE_SNAPSHOT_DISPATCH_FAILED campaign={} attempt={} checkpoint=unknown restoreRevision={} requiredMembers=0 resolvedRecipients=0 reason=checkpoint-unavailable",
                campaign.Value,
                handled.Activity->Attempt.Value,
                handled.Activity->RestoreRevision.value_or(0));
        }
    }
}

void CampaignProtocolService::OnRecoverySnapshotApplied(
    const PacketEvent<CampaignRecoverySnapshotApplied>& acPacket) noexcept
{
    Player& player = *acPacket.pPlayer;
    if (!acPacket.Packet.IsValid())
    {
        spdlog::warn(
            "[STRE][CampaignRecovery] SNAPSHOT_ACK_REJECTED transientPlayer={} reason=malformed-packet",
            player.GetId());
        return;
    }

    const CampaignId campaign{acPacket.Packet.CampaignId.c_str()};
    spdlog::info(
        "[STRE][CampaignRecovery] APPLIED_RESULT_RECEIVED campaign={} attempt={} checkpoint={} restoreRevision={} transientPlayer={}",
        campaign.Value,
        acPacket.Packet.RestoreAttemptId.c_str(),
        acPacket.Packet.CheckpointId.c_str(),
        acPacket.Packet.RestoreRevision,
        player.GetId());
    CampaignRecoveryCommandResult handled =
        m_admission.HandleRecoverySnapshotApplied(
            ToHandle(player),
            campaign,
            RestoreAttemptId{
                acPacket.Packet.RestoreAttemptId.c_str()},
            CheckpointId{acPacket.Packet.CheckpointId.c_str()},
            acPacket.Packet.RestoreRevision);
    if (!handled)
    {
        spdlog::warn(
            "[STRE][CampaignRecovery] SNAPSHOT_ACK_REJECTED campaign={} attempt={} checkpoint={} transientPlayer={} error={} reason={}",
            campaign.Value,
            acPacket.Packet.RestoreAttemptId.c_str(),
            acPacket.Packet.CheckpointId.c_str(),
            player.GetId(),
            static_cast<unsigned>(handled.Command.Error),
            handled.Command.Message);
        return;
    }

    spdlog::info(
        "[STRE][CampaignRecovery] APPLIED_ACK campaign={} attempt={} checkpoint={} restoreRevision={} transientPlayer={} secondBarrier={} replay={}",
        campaign.Value,
        acPacket.Packet.RestoreAttemptId.c_str(),
        acPacket.Packet.CheckpointId.c_str(),
        acPacket.Packet.RestoreRevision,
        player.GetId(),
        handled.RecoveryCompleted,
        handled.Command.IdempotentReplay);
    if (!handled.RecoveryCompleted)
        return;

    spdlog::info(
        "[STRE][CampaignRecovery] APPLIED_BARRIER_COMPLETE campaign={} attempt={} checkpoint={} restoreRevision={}",
        campaign.Value,
        acPacket.Packet.RestoreAttemptId.c_str(),
        acPacket.Packet.CheckpointId.c_str(),
        acPacket.Packet.RestoreRevision);

    CampaignRecoveryComplete complete;
    complete.CampaignId = acPacket.Packet.CampaignId;
    complete.RestoreAttemptId = acPacket.Packet.RestoreAttemptId;
    complete.CheckpointId = acPacket.Packet.CheckpointId;
    complete.RestoreRevision = acPacket.Packet.RestoreRevision;
    if (!complete.IsValid())
        return;

    if (handled.Activity)
    {
        for (const CampaignConnectionHandle connection :
             m_admission.GetAdmittedConnections(campaign))
        {
            Player* const pMember = m_world.GetPlayerManager().GetById(
                static_cast<std::uint32_t>(connection));
            if (pMember)
                pMember->Send(complete);
        }
    }
    else
    {
        player.Send(complete);
    }
    if (const auto snapshot = m_admission.BuildSnapshot(campaign))
        BroadcastSnapshot(*snapshot);
    spdlog::info(
        "[STRE][CampaignRecovery] RECOVERY_COMPLETED campaign={} attempt={} checkpoint={} restoreRevision={} replay={}",
        campaign.Value,
        acPacket.Packet.RestoreAttemptId.c_str(),
        acPacket.Packet.CheckpointId.c_str(),
        acPacket.Packet.RestoreRevision,
        handled.Command.IdempotentReplay);
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
