#include <CampaignSavePolicy.h>

#include <algorithm>
#include <cctype>
#include <mutex>
#include <unordered_map>

namespace STRE::Campaign
{
namespace
{
thread_local std::uint32_t s_internalCheckpointDepth{};
thread_local std::uint32_t s_quickActionDepth{};
thread_local std::uint32_t s_manualNewSlotDepth{};
thread_local bool s_manualNewSlotConsumed{};
thread_local const void* s_armedQuickRequest{};

enum class QuickRequestState : std::uint8_t
{
    Queued,
    Popped
};

std::mutex s_quickRequestsMutex;
std::unordered_map<const void*, QuickRequestState> s_quickRequests;

void ClearArmedQuickRequestLocked() noexcept
{
    if (!s_armedQuickRequest)
        return;
    const auto it = s_quickRequests.find(s_armedQuickRequest);
    if (it != s_quickRequests.end() &&
        it->second == QuickRequestState::Popped)
    {
        s_quickRequests.erase(it);
    }
    s_armedQuickRequest = nullptr;
}

bool StartsWithInsensitive(
    std::string_view acValue,
    std::string_view acPrefix) noexcept
{
    return acValue.size() >= acPrefix.size() && std::equal(
        acPrefix.begin(), acPrefix.end(), acValue.begin(),
        [](unsigned char aLeft, unsigned char aRight)
        {
            return std::tolower(aLeft) == std::tolower(aRight);
        });
}
}

CampaignSaveOrigin ClassifyCampaignNativeSaveName(
    std::string_view acNativeSaveName) noexcept
{
    if (StartsWithInsensitive(acNativeSaveName, "quicksave"))
        return CampaignSaveOrigin::Quick;
    if (StartsWithInsensitive(acNativeSaveName, "autosave"))
        return CampaignSaveOrigin::Auto;
    if (StartsWithInsensitive(acNativeSaveName, "save"))
        return CampaignSaveOrigin::Manual;
    return CampaignSaveOrigin::Unknown;
}

CampaignSaveDecision EvaluateCampaignSavePolicy(
    CampaignSaveOrigin aOrigin,
    const CampaignSavePolicyContext& acContext) noexcept
{
    if (acContext.InternalCheckpoint)
        return CampaignSaveDecision::AllowInternalCheckpoint;
    if (!acContext.InCampaign)
        return CampaignSaveDecision::AllowVanilla;
    if (aOrigin == CampaignSaveOrigin::Auto)
        return CampaignSaveDecision::BlockAutosave;
    if (aOrigin == CampaignSaveOrigin::Unknown)
        return CampaignSaveDecision::BlockUnknown;
    if (acContext.RuntimeFenced)
        return CampaignSaveDecision::BlockUnavailable;
    if (acContext.RuntimeState == CampaignSaveRuntimeState::Active)
        return CampaignSaveDecision::RequestCollectiveCheckpoint;
    if (acContext.RuntimeState == CampaignSaveRuntimeState::Checkpointing)
        return CampaignSaveDecision::CoalesceWithCheckpoint;
    return CampaignSaveDecision::BlockUnavailable;
}

ScopedCampaignQuickSaveAction::ScopedCampaignQuickSaveAction(
    bool aActionable) noexcept
    : m_active(aActionable)
{
    if (m_active)
        ++s_quickActionDepth;
}

ScopedCampaignQuickSaveAction::~ScopedCampaignQuickSaveAction() noexcept
{
    if (m_active && s_quickActionDepth != 0)
        --s_quickActionDepth;
}

ScopedCampaignQuickSaveProcessBoundary::~ScopedCampaignQuickSaveProcessBoundary()
    noexcept
{
    CampaignSaveProvenance::FinishQuickProcessBoundary();
}

ScopedCampaignManualNewSlotSave::ScopedCampaignManualNewSlotSave() noexcept
{
    if (s_manualNewSlotDepth++ == 0)
        s_manualNewSlotConsumed = false;
}

ScopedCampaignManualNewSlotSave::~ScopedCampaignManualNewSlotSave() noexcept
{
    if (s_manualNewSlotDepth != 0 && --s_manualNewSlotDepth == 0)
        s_manualNewSlotConsumed = false;
}

void CampaignSaveProvenance::ObserveQuickRequestPush(
    const void* apRequest,
    std::uint32_t aOperationCode,
    bool aSucceeded) noexcept
{
    if (!apRequest)
        return;
    try
    {
        const std::scoped_lock lock(s_quickRequestsMutex);
        const auto existing = s_quickRequests.find(apRequest);
        if (!aSucceeded ||
            aOperationCode != QuickRequestOperationCode)
        {
            if (existing != s_quickRequests.end())
                s_quickRequests.erase(existing);
            if (s_armedQuickRequest == apRequest)
                s_armedQuickRequest = nullptr;
            return;
        }

        if (existing != s_quickRequests.end())
        {
            existing->second = QuickRequestState::Queued;
            if (s_armedQuickRequest == apRequest)
                s_armedQuickRequest = nullptr;
            return;
        }

        if (s_quickActionDepth != 0)
            s_quickRequests.emplace(apRequest, QuickRequestState::Queued);
    }
    catch (...)
    {
        if (s_armedQuickRequest == apRequest)
            s_armedQuickRequest = nullptr;
    }
}

void CampaignSaveProvenance::ObserveQuickRequestPop(
    const void* apRequest,
    std::uint32_t aOperationCode) noexcept
{
    try
    {
        const std::scoped_lock lock(s_quickRequestsMutex);
        // Reaching another successful pop proves that a previously popped
        // request completed without reaching Save_Impl or being requeued.
        ClearArmedQuickRequestLocked();
        if (!apRequest)
            return;

        const auto it = s_quickRequests.find(apRequest);
        if (it == s_quickRequests.end())
            return;
        if (aOperationCode != QuickRequestOperationCode)
        {
            s_quickRequests.erase(it);
            return;
        }

        it->second = QuickRequestState::Popped;
        s_armedQuickRequest = apRequest;
    }
    catch (...)
    {
        s_armedQuickRequest = nullptr;
    }
}

std::optional<CampaignSaveOrigin> CampaignSaveProvenance::Consume() noexcept
{
    if (s_manualNewSlotDepth != 0 && !s_manualNewSlotConsumed)
    {
        s_manualNewSlotConsumed = true;
        return CampaignSaveOrigin::Manual;
    }
    if (!s_armedQuickRequest)
        return std::nullopt;

    try
    {
        const std::scoped_lock lock(s_quickRequestsMutex);
        const auto it = s_quickRequests.find(s_armedQuickRequest);
        if (it == s_quickRequests.end() ||
            it->second != QuickRequestState::Popped)
        {
            s_armedQuickRequest = nullptr;
            return std::nullopt;
        }
        s_quickRequests.erase(it);
        s_armedQuickRequest = nullptr;
        return CampaignSaveOrigin::Quick;
    }
    catch (...)
    {
        s_armedQuickRequest = nullptr;
        return std::nullopt;
    }
}

void CampaignSaveProvenance::FinishQuickProcessBoundary() noexcept
{
    try
    {
        const std::scoped_lock lock(s_quickRequestsMutex);
        ClearArmedQuickRequestLocked();
    }
    catch (...)
    {
        s_armedQuickRequest = nullptr;
    }
}

ScopedCampaignCheckpointNativeSave::ScopedCampaignCheckpointNativeSave()
    noexcept
{
    ++s_internalCheckpointDepth;
}

ScopedCampaignCheckpointNativeSave::~ScopedCampaignCheckpointNativeSave()
    noexcept
{
    if (s_internalCheckpointDepth != 0)
        --s_internalCheckpointDepth;
}

bool ScopedCampaignCheckpointNativeSave::IsActive() noexcept
{
    return s_internalCheckpointDepth != 0;
}
}
