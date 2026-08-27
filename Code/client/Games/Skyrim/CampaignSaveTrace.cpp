#include <TiltedOnlinePCH.h>

#include <CampaignSaveTrace.h>

#include <atomic>

namespace
{
std::atomic<std::uint64_t> s_sequence{};
std::atomic<std::uint64_t> s_frame{};
thread_local std::uint32_t s_continueCallbackDepth{};
}

void CampaignSaveTrace::AdvanceFrame() noexcept
{
    (void)s_frame.fetch_add(1, std::memory_order_relaxed);
}

CampaignSaveTrace::Context CampaignSaveTrace::Capture() noexcept
{
    return {
        s_sequence.fetch_add(1, std::memory_order_relaxed) + 1,
        s_frame.load(std::memory_order_relaxed),
        GetCurrentThreadId()};
}

void CampaignSaveTrace::EnterContinueCallback() noexcept
{
    ++s_continueCallbackDepth;
}

void CampaignSaveTrace::ExitContinueCallback() noexcept
{
    if (s_continueCallbackDepth > 0)
        --s_continueCallbackDepth;
}

bool CampaignSaveTrace::IsContinueCallbackActive() noexcept
{
    return s_continueCallbackDepth > 0;
}
