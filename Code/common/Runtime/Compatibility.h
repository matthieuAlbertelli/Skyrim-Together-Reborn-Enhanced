#pragma once

#include <Runtime/AddressLibrary.h>
#include <string_view>

namespace STRE::Runtime
{
inline constexpr Version TargetVersion{1, 7, 104, 0};
inline constexpr std::string_view TargetVersionString = "1.7.104.0";
inline constexpr std::string_view RequiredSkseDll = "skse64_1_7_104.dll";
// SKSE v2.3.1 skse64_common/skse_version.h, not a guessed semver tuple.
inline constexpr Version RequiredSkseFileVersion{0, 2, 3, 1};

enum class Distribution { Unknown, Steam, Other };

inline constexpr bool Supports(std::string_view aVersion, Distribution aDistribution)
{
    return aVersion == TargetVersionString && aDistribution == Distribution::Steam;
}

// This partial port must not enter Skyrim while campaign-critical native
// save/load, creation and recovery boundaries still have only 1.6 ABI evidence.
// No INI/debug override: complete the audit and its tests before changing this.
inline constexpr bool CampaignAbiVerified = false;
inline constexpr std::string_view CampaignAbiUnavailable =
    "STRE 1.7.104 experimental port: campaign native save/load, creation and recovery ABI is not verified. "
    "Game startup is unavailable in this partial candidate. See issue #99.";
} // namespace STRE::Runtime
