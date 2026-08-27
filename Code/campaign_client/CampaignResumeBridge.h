#pragma once

#include <array>
#include <cstddef>
#include <string_view>

namespace STRE::Campaign
{
inline constexpr std::string_view kCampaignResumeActionFunction =
    "campaignResumeAction";
inline constexpr std::size_t kCampaignResumeMaximumArgumentCount = 2;

inline constexpr std::array<std::string_view, 1> kCampaignResumeCefFunctions{
    kCampaignResumeActionFunction};

enum class CampaignResumeAction
{
    Unknown,
    Refresh,
    Retry,
    Select,
    StayAndRecover,
    ReturnToMainMenu
};

[[nodiscard]] constexpr CampaignResumeAction ParseCampaignResumeAction(
    std::string_view acAction) noexcept
{
    if (acAction == "refresh")
        return CampaignResumeAction::Refresh;
    if (acAction == "retry")
        return CampaignResumeAction::Retry;
    if (acAction == "select")
        return CampaignResumeAction::Select;
    if (acAction == "stayAndRecover")
        return CampaignResumeAction::StayAndRecover;
    if (acAction == "returnToMainMenu")
        return CampaignResumeAction::ReturnToMainMenu;
    return CampaignResumeAction::Unknown;
}
}
