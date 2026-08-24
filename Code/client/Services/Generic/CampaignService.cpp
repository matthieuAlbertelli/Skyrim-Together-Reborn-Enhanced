#include <Services/CampaignService.h>

#include <Services/TransportService.h>

#include <Events/ConnectedEvent.h>
#include <Events/DisconnectedEvent.h>
#include <Messages/CampaignMessages.h>
#include <Messages/CampaignRequests.h>

#include <algorithm>

namespace
{
bool Succeeded(CampaignProtocolResult aResult) noexcept
{
    return aResult == CampaignProtocolResult::Applied || aResult == CampaignProtocolResult::AcceptedNoOp || aResult == CampaignProtocolResult::IdempotentReplay;
}
} // namespace

CampaignService::CampaignService(entt::dispatcher& aDispatcher, TransportService& aTransport) noexcept
    : m_transport(aTransport)
    , m_responseConnection(aDispatcher.sink<CampaignCommandResponse>()
          .connect<&CampaignService::OnCommandResponse>(this))
    , m_snapshotConnection(aDispatcher.sink<NotifyCampaignSnapshot>()
          .connect<&CampaignService::OnSnapshot>(this))
    , m_lobbyStateConnection(aDispatcher.sink<NotifyCampaignLobbyState>()
          .connect<&CampaignService::OnLobbyState>(this))
    , m_helgenStateConnection(aDispatcher.sink<NotifyCampaignHelgenState>()
          .connect<&CampaignService::OnHelgenState>(this))
    , m_connectedConnection(aDispatcher.sink<ConnectedEvent>()
          .connect<&CampaignService::OnConnected>(this))
    , m_disconnectedConnection(aDispatcher.sink<DisconnectedEvent>()
          .connect<&CampaignService::OnDisconnected>(this))
{
    auto directory = STRE::Campaign::CampaignIdentityStore::ResolveDefaultDirectory();
    if (!directory)
    {
        m_storageError = std::move(directory.Message);
        spdlog::error("[STRE][CampaignIdentity] {}", m_storageError);
        return;
    }
    m_store = std::make_unique<STRE::Campaign::CampaignIdentityStore>(std::move(directory.Value));
    auto identity = m_store->LoadOrCreatePlayerId();
    if (!identity)
    {
        m_storageError = std::move(identity.Message);
        spdlog::error("[STRE][CampaignIdentity] {}", m_storageError);
        return;
    }
    m_playerId = std::move(identity.Value);
    const auto cacheValidation = m_store->LoadBinding("validation");
    if (!cacheValidation)
    {
        m_storageError = cacheValidation.Message;
        spdlog::error("[STRE][CampaignIdentity] {}", m_storageError);
        return;
    }
    m_bindingCacheAvailable = true;
    spdlog::info("[STRE][CampaignIdentity] durable local PlayerId loaded");
}

bool CampaignService::SignalHelgenInvestigationReady() noexcept
{
    const auto readiness = m_admissionState.GetHelgenReadinessView();
    if (!readiness.Admission)
    {
        if (!m_helgenReadinessRejectionLogged.exchange(
                true, std::memory_order_relaxed))
        {
            spdlog::warn(
                "[STRE][CampaignAdmission] Helgen readiness rejected: no client admission");
        }
        return false;
    }
    if (!readiness.CanSignal)
    {
        if (!m_helgenReadinessRejectionLogged.exchange(
                true, std::memory_order_relaxed))
        {
            spdlog::warn(
                "[STRE][CampaignAdmission] Helgen readiness rejected: canonical ACTIVE exact-roster snapshot unavailable campaign={}",
                readiness.Admission->CampaignId);
        }
        return false;
    }
    CampaignHelgenInvestigationReadyRequest request;
    const bool sent = m_transport.Send(request);
    spdlog::debug(
        "[STRE][CampaignAdmission] Helgen readiness transport boundary campaign={} sent={}",
        readiness.Admission->CampaignId, sent);
    return sent;
}

std::optional<TiltedPhoques::String> CampaignService::GetDurablePlayerIdForAuthentication() const noexcept
{
    if (!m_playerId)
        return std::nullopt;
    return TiltedPhoques::String(m_playerId->c_str());
}

std::string CampaignService::GenerateMutationId() const
{
    return STRE::Campaign::CampaignIdentityStore::GenerateOpaqueId(16);
}

std::string CampaignService::CreateCampaign(
    const std::string& acDisplayName,
    const std::string& acMutationId) noexcept
{
    if (!m_bindingCacheAvailable)
        return {};
    try
    {
        CampaignCreateRequest request;
        const std::string mutation = acMutationId.empty() ? GenerateMutationId() : acMutationId;
        if (!STRE::Campaign::CampaignIdentityStore::IsValidCacheId(mutation))
            return {};
        TiltedPhoques::String displayName;
        if (!NormalizeCampaignLobbyDisplayName(acDisplayName, displayName))
            return {};
        request.MutationId = mutation.c_str();
        request.DisplayName = displayName;
        (void)m_transport.Send(request);
        return mutation;
    }
    catch (...)
    {
        return {};
    }
}

std::string CampaignService::JoinCampaign(const std::string& acCampaignId, std::uint64_t aExpectedRevision, const std::string& acMutationId) noexcept
{
    if (!m_bindingCacheAvailable)
        return {};
    try
    {
        CampaignJoinRequest request;
        const std::string mutation = acMutationId.empty() ? GenerateMutationId() : acMutationId;
        if (!STRE::Campaign::CampaignIdentityStore::IsValidCacheId(mutation))
            return {};
        request.CampaignId = acCampaignId.c_str();
        request.MutationId = mutation.c_str();
        request.ExpectedRevision = aExpectedRevision;
        (void)m_transport.Send(request);
        return mutation;
    }
    catch (...)
    {
        return {};
    }
}

std::string CampaignService::JoinCampaignByCode(
    const std::string& acJoinCode,
    const std::string& acDisplayName,
    const std::string& acMutationId) noexcept
{
    if (!m_bindingCacheAvailable)
        return {};
    try
    {
        TiltedPhoques::String normalized;
        if (!NormalizeCampaignJoinCode(acJoinCode, normalized))
            return {};
        TiltedPhoques::String displayName;
        if (!NormalizeCampaignLobbyDisplayName(acDisplayName, displayName))
            return {};
        CampaignJoinByCodeRequest request;
        const std::string mutation = acMutationId.empty()
            ? GenerateMutationId() : acMutationId;
        if (!STRE::Campaign::CampaignIdentityStore::IsValidCacheId(mutation))
            return {};
        request.JoinCode = normalized;
        request.MutationId = mutation.c_str();
        request.DisplayName = displayName;
        (void)m_transport.Send(request);
        return mutation;
    }
    catch (...)
    {
        return {};
    }
}

bool CampaignService::ResumeCampaign(
    const std::string& acCampaignId) noexcept
{
    if (!m_store)
        return false;
    if (!m_bindingCacheAvailable)
        return false;
    auto binding = m_store->LoadBinding(acCampaignId);
    if (!binding)
    {
        m_storageError = binding.Message;
        m_bindingCacheAvailable = false;
        spdlog::error("[STRE][CampaignIdentity] {}", binding.Message);
        return false;
    }
    if (!binding.Value)
    {
        spdlog::error("[STRE][CampaignIdentity] no cached binding for campaign={}", acCampaignId);
        return false;
    }
    CampaignResumeRequest request;
    request.CampaignId = acCampaignId.c_str();
    request.CharacterBindingId = binding.Value->CharacterBindingId.c_str();
    return m_transport.Send(request);
}

std::string CampaignService::StartCampaign(const std::string& acCampaignId, std::uint64_t aExpectedRevision, const std::string& acMutationId) noexcept
{
    try
    {
        CampaignStartRequest request;
        const std::string mutation = acMutationId.empty() ? GenerateMutationId() : acMutationId;
        if (!STRE::Campaign::CampaignIdentityStore::IsValidCacheId(mutation))
            return {};
        request.CampaignId = acCampaignId.c_str();
        request.MutationId = mutation.c_str();
        request.ExpectedRevision = aExpectedRevision;
        (void)m_transport.Send(request);
        return mutation;
    }
    catch (...)
    {
        return {};
    }
}

std::string CampaignService::SetReady(const std::string& acCampaignId, std::uint64_t aExpectedRevision, bool aReady, const std::string& acMutationId) noexcept
{
    try
    {
        CampaignSetReadyRequest request;
        const std::string mutation = acMutationId.empty() ? GenerateMutationId() : acMutationId;
        if (!STRE::Campaign::CampaignIdentityStore::IsValidCacheId(mutation))
            return {};
        request.CampaignId = acCampaignId.c_str();
        request.MutationId = mutation.c_str();
        request.ExpectedRevision = aExpectedRevision;
        request.Ready = aReady;
        (void)m_transport.Send(request);
        return mutation;
    }
    catch (...)
    {
        return {};
    }
}

std::string CampaignService::LeaveCampaign(const std::string& acCampaignId, std::uint64_t aExpectedRevision, const std::string& acMutationId) noexcept
{
    try
    {
        CampaignLeaveRequest request;
        const std::string mutation = acMutationId.empty() ? GenerateMutationId() : acMutationId;
        if (!STRE::Campaign::CampaignIdentityStore::IsValidCacheId(mutation))
            return {};
        request.CampaignId = acCampaignId.c_str();
        request.MutationId = mutation.c_str();
        request.ExpectedRevision = aExpectedRevision;
        (void)m_transport.Send(request);
        return mutation;
    }
    catch (...)
    {
        return {};
    }
}

void CampaignService::OnCommandResponse(const CampaignCommandResponse& acResponse) noexcept
{
    if (!acResponse.IsValid())
    {
        spdlog::error("[STRE][CampaignProtocol] ignored malformed command response");
        return;
    }
    m_lastCommandOutcome = CampaignClientCommandOutcome{
        acResponse.Operation,
        acResponse.Result,
        acResponse.MutationId.c_str(),
        acResponse.CampaignId.c_str(),
        acResponse.StateVersion,
        acResponse.JoinCode.c_str()};
    if (!Succeeded(acResponse.Result))
    {
        if (acResponse.Operation == CampaignProtocolOperation::Resume)
            m_admissionState.ResumeRejected();
        spdlog::warn(
            "[STRE][CampaignProtocol] command rejected operation={} result={} campaign={} revision={}", static_cast<unsigned>(acResponse.Operation),
            static_cast<unsigned>(acResponse.Result), acResponse.CampaignId.c_str(), acResponse.StateVersion);
        return;
    }

    if ((acResponse.Operation == CampaignProtocolOperation::Create ||
         acResponse.Operation == CampaignProtocolOperation::Join ||
         acResponse.Operation == CampaignProtocolOperation::JoinByCode ||
         acResponse.Operation == CampaignProtocolOperation::Resume) &&
        !acResponse.CampaignSlotId.empty() && !acResponse.CharacterBindingId.empty())
    {
        STRE::Campaign::CampaignClientAdmission admission{
            acResponse.CampaignId.c_str(),
            acResponse.CampaignSlotId.c_str(),
            acResponse.CharacterBindingId.c_str()};
        const auto previousAdmission = m_admissionState.GetAdmission();
        if (!previousAdmission ||
            previousAdmission->CampaignId != admission.CampaignId)
        {
            m_helgenState.Reset();
        }
        if (m_store)
        {
            const auto saved = m_store->SaveBinding({admission.CampaignId, admission.CampaignSlotId, admission.CharacterBindingId});
            if (!saved)
            {
                m_storageError = saved.Message;
                m_bindingCacheAvailable = false;
                spdlog::error("[STRE][CampaignIdentity] {}", saved.Message);
                return;
            }
        }
        m_admissionState.Accept(std::move(admission));
        m_helgenReadinessRejectionLogged.store(
            false, std::memory_order_relaxed);
        spdlog::info(
            "[STRE][CampaignAdmission] server-validated admission accepted operation={} campaign={} revision={}",
            static_cast<unsigned>(acResponse.Operation),
            acResponse.CampaignId.c_str(), acResponse.StateVersion);
    }
    else if (acResponse.Operation == CampaignProtocolOperation::Start)
    {
        const auto admission = m_admissionState.GetAdmission();
        const bool admissionRetained = admission &&
            admission->CampaignId == acResponse.CampaignId.c_str();
        spdlog::log(
            admissionRetained ? spdlog::level::info : spdlog::level::err,
            "[STRE][CampaignAdmission] campaign Start accepted campaign={} revision={} admissionRetained={}",
            acResponse.CampaignId.c_str(), acResponse.StateVersion,
            admissionRetained);
    }
    else if (acResponse.Operation == CampaignProtocolOperation::Leave)
    {
        if (m_store)
        {
            const auto removed = m_store->RemoveBinding(acResponse.CampaignId.c_str());
            if (!removed)
            {
                m_storageError = removed.Message;
                m_bindingCacheAvailable = false;
                spdlog::error("[STRE][CampaignIdentity] {}", removed.Message);
            }
        }
        m_admissionState.Leave(acResponse.CampaignId.c_str());
        m_latestSnapshot.reset();
        m_lobbyState.reset();
        m_helgenState.Reset();
    }
}

void CampaignService::OnSnapshot(const NotifyCampaignSnapshot& acNotification) noexcept
{
    if (!acNotification.IsValid())
    {
        spdlog::error("[STRE][CampaignProtocol] ignored malformed campaign snapshot");
        return;
    }
    if (m_latestSnapshot && m_latestSnapshot->CampaignId == acNotification.Snapshot.CampaignId && m_latestSnapshot->StateVersion > acNotification.Snapshot.StateVersion)
    {
        return;
    }
    m_latestSnapshot = acNotification.Snapshot;
    const std::size_t presentCount = static_cast<std::size_t>(std::count_if(
        acNotification.Snapshot.Roster.begin(),
        acNotification.Snapshot.Roster.end(),
        [](const CampaignPublicSlotData& acSlot) { return acSlot.Present; }));
    m_admissionState.ObserveSnapshot(
        acNotification.Snapshot.CampaignId.c_str(),
        acNotification.Snapshot.RosterSealed,
        acNotification.Snapshot.RuntimeState == kCampaignWireRuntimeActive,
        acNotification.Snapshot.Roster.size(), presentCount);
    spdlog::log(
        acNotification.Snapshot.RosterSealed &&
                acNotification.Snapshot.RuntimeState == kCampaignWireRuntimeActive
            ? spdlog::level::info
            : spdlog::level::debug,
        "[STRE][CampaignAdmission] canonical snapshot observed campaign={} revision={} sealed={} runtime={} present={}/{}",
        acNotification.Snapshot.CampaignId.c_str(),
        acNotification.Snapshot.StateVersion,
        acNotification.Snapshot.RosterSealed,
        static_cast<unsigned>(acNotification.Snapshot.RuntimeState),
        presentCount, acNotification.Snapshot.Roster.size());
    if (acNotification.Snapshot.RosterSealed)
        m_lobbyState.reset();
}

void CampaignService::OnLobbyState(
    const NotifyCampaignLobbyState& acNotification) noexcept
{
    if (!acNotification.IsValid())
    {
        spdlog::error(
            "[STRE][CampaignLobby] ignored malformed lobby projection");
        return;
    }
    const auto admission = m_admissionState.GetAdmission();
    if (!admission ||
        admission->CampaignId != acNotification.CampaignId.c_str())
    {
        spdlog::warn(
            "[STRE][CampaignLobby] ignored projection for an unrelated campaign");
        return;
    }
    if (m_lobbyState &&
        m_lobbyState->StateVersion > acNotification.StateVersion)
    {
        return;
    }

    CampaignClientLobbyState state;
    state.JoinCode = acNotification.JoinCode.c_str();
    state.StateVersion = acNotification.StateVersion;
    state.CanStart = acNotification.CanStart;
    state.Members.reserve(acNotification.Members.size());
    for (const CampaignLobbyMemberData& member : acNotification.Members)
        state.Members.push_back({member.Name.c_str(), member.Present});
    m_lobbyState = std::move(state);
}

void CampaignService::OnHelgenState(const NotifyCampaignHelgenState& acNotification) noexcept
{
    if (!acNotification.IsValid() || !m_admissionState.GetAdmission())
        return;

    const bool wasAuthorized =
        m_helgenState.IsInvestigationStartAuthorized();
    m_helgenState.Apply(acNotification.InvestigationStartAuthorized, acNotification.SpatialStatus == CampaignHelgenSpatialStatus::Known, acNotification.AllRequiredPlayersOutside);
    if (!wasAuthorized && acNotification.InvestigationStartAuthorized)
    {
        spdlog::info(
            "[STRE][CampaignAdmission] Helgen investigation authorization received");
    }
}

void CampaignService::OnConnected(const ConnectedEvent&) noexcept
{
    const auto campaign = m_admissionState.BeginResume();
    if (!campaign)
    {
        spdlog::debug(
            "[STRE][CampaignAdmission] transport connected without a pending campaign resume");
        return;
    }

    if (!ResumeCampaign(*campaign))
    {
        m_admissionState.ResumeRejected();
        spdlog::warn(
            "[STRE][CampaignAdmission] explicit resume could not be sent campaign={}",
            *campaign);
        return;
    }
    spdlog::info(
        "[STRE][CampaignAdmission] explicit resume sent after reconnect campaign={}",
        *campaign);
}

void CampaignService::OnDisconnected(const DisconnectedEvent&) noexcept
{
    const auto resumeCandidate = m_admissionState.Disconnect();
    m_helgenReadinessRejectionLogged.store(
        false, std::memory_order_relaxed);
    spdlog::info(
        "[STRE][CampaignAdmission] transport disconnected; volatile admission cleared resumeCandidate={}",
        resumeCandidate ? *resumeCandidate : "none");
    m_latestSnapshot.reset();
    m_lastCommandOutcome.reset();
    m_lobbyState.reset();
    m_helgenState.Reset();
}
