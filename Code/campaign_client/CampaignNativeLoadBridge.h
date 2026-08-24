#pragma once

#include <array>
#include <string_view>

namespace STRE::Campaign
{
inline constexpr std::string_view kCampaignNativeLoadFunction =
    "campaignNativeLoad";
inline constexpr std::string_view kCampaignNativeLoadReleaseFunction =
    "campaignNativeLoadRelease";
inline constexpr std::string_view kCampaignNativeLoadResumeFunction =
    "campaignNativeLoadResume";

inline constexpr std::array<std::string_view, 3>
    kCampaignNativeLoadCefFunctions{
        kCampaignNativeLoadFunction,
        kCampaignNativeLoadReleaseFunction,
        kCampaignNativeLoadResumeFunction};
}
