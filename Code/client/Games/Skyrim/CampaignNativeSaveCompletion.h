#pragma once

#include <cstdint>
#include <string>

namespace CampaignNativeSaveDetail
{
class RequestSlot;
}

struct CampaignNativeSaveCompletionPaths
{
    std::string Ess;
    std::string Skse;
    std::string EssTemporary;
};

namespace CampaignNativeSaveCompletion
{
inline constexpr std::uint32_t kDeadlineMilliseconds = 30000;

[[nodiscard]] bool IsAvailable() noexcept;

[[nodiscard]] bool PrepareFresh(
    const std::string& acIdentity,
    CampaignNativeSaveCompletionPaths& aPaths,
    std::string& aFailureReason);

[[nodiscard]] bool Start(
    std::string aIdentity,
    CampaignNativeSaveCompletionPaths aPaths,
    CampaignNativeSaveDetail::RequestSlot& aRequestSlot);
}
