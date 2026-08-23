#pragma once

#include <array>
#include <cstddef>
#include <string_view>

namespace STRE::Campaign
{
inline constexpr std::string_view kCampaignBootstrapActionFunction =
    "campaignBootstrapAction";
inline constexpr std::size_t kCampaignBootstrapMaximumArgumentCount = 5;
inline constexpr std::size_t kCampaignBootstrapDisplayNameArgumentIndex = 4;

inline constexpr std::array<std::string_view, 1>
    kCampaignBootstrapCefFunctions{
        kCampaignBootstrapActionFunction};

enum class CampaignBootstrapAction
{
    Unknown,
    Solo,
    ShowCreate,
    ShowJoin,
    Create,
    Join,
    Start,
    Back
};

[[nodiscard]] constexpr CampaignBootstrapAction ParseCampaignBootstrapAction(
    std::string_view acAction) noexcept
{
    if (acAction == "solo")
        return CampaignBootstrapAction::Solo;
    if (acAction == "showCreate")
        return CampaignBootstrapAction::ShowCreate;
    if (acAction == "showJoin")
        return CampaignBootstrapAction::ShowJoin;
    if (acAction == "create")
        return CampaignBootstrapAction::Create;
    if (acAction == "join")
        return CampaignBootstrapAction::Join;
    if (acAction == "start")
        return CampaignBootstrapAction::Start;
    if (acAction == "back")
        return CampaignBootstrapAction::Back;
    return CampaignBootstrapAction::Unknown;
}
}
