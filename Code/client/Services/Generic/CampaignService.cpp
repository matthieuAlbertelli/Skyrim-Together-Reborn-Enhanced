#include <Services/CampaignService.h>

#include <Services/TransportService.h>

#include <Events/DisconnectedEvent.h>
#include <Messages/CampaignMessages.h>
#include <Messages/CampaignRequests.h>

namespace
{
bool Succeeded(CampaignProtocolResult aResult) noexcept
{
    return aResult == CampaignProtocolResult::Applied ||
        aResult == CampaignProtocolResult::AcceptedNoOp ||
        aResult == CampaignProtocolResult::IdempotentReplay;
}
}

CampaignService::CampaignService(
    entt::dispatcher& aDispatcher,
    TransportService& aTransport) noexcept
    : m_transport(aTransport)
    , m_responseConnection(aDispatcher.sink<CampaignCommandResponse>()
          .connect<&CampaignService::OnCommandResponse>(this))
    , m_snapshotConnection(aDispatcher.sink<NotifyCampaignSnapshot>()
          .connect<&CampaignService::OnSnapshot>(this))
    , m_disconnectedConnection(aDispatcher.sink<DisconnectedEvent>()
          .connect<&CampaignService::OnDisconnected>(this))
{
    auto directory =
        STRE::Campaign::CampaignIdentityStore::ResolveDefaultDirectory();
    if (!directory)
    {
        m_storageError = std::move(directory.Message);
        spdlog::error("[STRE][CampaignIdentity] {}", m_storageError);
        return;
    }
    m_store = std::make_unique<STRE::Campaign::CampaignIdentityStore>(
        std::move(directory.Value));
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
    spdlog::info(
        "[STRE][CampaignIdentity] durable local PlayerId loaded");
}

std::optional<TiltedPhoques::String>
CampaignService::GetDurablePlayerIdForAuthentication() const noexcept
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
    const std::string& acMutationId) noexcept
{
    if (!m_bindingCacheAvailable)
        return {};
    try
    {
        CampaignCreateRequest request;
        const std::string mutation = acMutationId.empty()
            ? GenerateMutationId() : acMutationId;
        if (!STRE::Campaign::CampaignIdentityStore::IsValidCacheId(mutation))
            return {};
        request.MutationId = mutation.c_str();
        (void)m_transport.Send(request);
        return mutation;
    }
    catch (...)
    {
        return {};
    }
}

std::string CampaignService::JoinCampaign(
    const std::string& acCampaignId,
    std::uint64_t aExpectedRevision,
    const std::string& acMutationId) noexcept
{
    if (!m_bindingCacheAvailable)
        return {};
    try
    {
        CampaignJoinRequest request;
        const std::string mutation = acMutationId.empty()
            ? GenerateMutationId() : acMutationId;
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
        spdlog::error(
            "[STRE][CampaignIdentity] no cached binding for campaign={}",
            acCampaignId);
        return false;
    }
    CampaignResumeRequest request;
    request.CampaignId = acCampaignId.c_str();
    request.CharacterBindingId =
        binding.Value->CharacterBindingId.c_str();
    return m_transport.Send(request);
}

std::string CampaignService::StartCampaign(
    const std::string& acCampaignId,
    std::uint64_t aExpectedRevision,
    const std::string& acMutationId) noexcept
{
    try
    {
        CampaignStartRequest request;
        const std::string mutation = acMutationId.empty()
            ? GenerateMutationId() : acMutationId;
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

std::string CampaignService::SetReady(
    const std::string& acCampaignId,
    std::uint64_t aExpectedRevision,
    bool aReady,
    const std::string& acMutationId) noexcept
{
    try
    {
        CampaignSetReadyRequest request;
        const std::string mutation = acMutationId.empty()
            ? GenerateMutationId() : acMutationId;
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

std::string CampaignService::LeaveCampaign(
    const std::string& acCampaignId,
    std::uint64_t aExpectedRevision,
    const std::string& acMutationId) noexcept
{
    try
    {
        CampaignLeaveRequest request;
        const std::string mutation = acMutationId.empty()
            ? GenerateMutationId() : acMutationId;
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

void CampaignService::OnCommandResponse(
    const CampaignCommandResponse& acResponse) noexcept
{
    if (!acResponse.IsValid())
    {
        spdlog::error(
            "[STRE][CampaignProtocol] ignored malformed command response");
        return;
    }
    m_lastCommandOutcome = CampaignClientCommandOutcome{
        acResponse.Operation,
        acResponse.Result,
        acResponse.MutationId.c_str(),
        acResponse.CampaignId.c_str(),
        acResponse.StateVersion};
    if (!Succeeded(acResponse.Result))
    {
        spdlog::warn(
            "[STRE][CampaignProtocol] command rejected operation={} result={} campaign={} revision={}",
            static_cast<unsigned>(acResponse.Operation),
            static_cast<unsigned>(acResponse.Result),
            acResponse.CampaignId.c_str(), acResponse.StateVersion);
        return;
    }

    if ((acResponse.Operation == CampaignProtocolOperation::Create ||
         acResponse.Operation == CampaignProtocolOperation::Join ||
         acResponse.Operation == CampaignProtocolOperation::Resume) &&
        !acResponse.CampaignSlotId.empty() &&
        !acResponse.CharacterBindingId.empty())
    {
        CampaignClientAdmission admission{
            acResponse.CampaignId.c_str(),
            acResponse.CampaignSlotId.c_str(),
            acResponse.CharacterBindingId.c_str()};
        if (m_store)
        {
            const auto saved = m_store->SaveBinding(
                {admission.CampaignId, admission.CampaignSlotId,
                 admission.CharacterBindingId});
            if (!saved)
            {
                m_storageError = saved.Message;
                m_bindingCacheAvailable = false;
                spdlog::error("[STRE][CampaignIdentity] {}", saved.Message);
                return;
            }
        }
        m_admission = std::move(admission);
    }
    else if (acResponse.Operation == CampaignProtocolOperation::Leave)
    {
        if (m_store)
        {
            const auto removed = m_store->RemoveBinding(
                acResponse.CampaignId.c_str());
            if (!removed)
            {
                m_storageError = removed.Message;
                m_bindingCacheAvailable = false;
                spdlog::error("[STRE][CampaignIdentity] {}", removed.Message);
            }
        }
        m_admission.reset();
        m_latestSnapshot.reset();
    }
}

void CampaignService::OnSnapshot(
    const NotifyCampaignSnapshot& acNotification) noexcept
{
    if (!acNotification.IsValid())
    {
        spdlog::error(
            "[STRE][CampaignProtocol] ignored malformed campaign snapshot");
        return;
    }
    if (m_latestSnapshot &&
        m_latestSnapshot->CampaignId == acNotification.Snapshot.CampaignId &&
        m_latestSnapshot->StateVersion > acNotification.Snapshot.StateVersion)
    {
        return;
    }
    m_latestSnapshot = acNotification.Snapshot;
}

void CampaignService::OnDisconnected(const DisconnectedEvent&) noexcept
{
    m_admission.reset();
    m_latestSnapshot.reset();
    m_lastCommandOutcome.reset();
}
