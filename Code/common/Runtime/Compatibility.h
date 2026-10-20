#pragma once

#include <Runtime/AddressLibrary.h>
#include <Runtime/NativeContracts.h>
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

// No runtime/INI override. Every mandatory obligation must be qualified.
// The current evidence table deliberately keeps this false.
inline constexpr bool CampaignAbiVerified = NativeAdmissionReady(NativeContracts);
inline constexpr std::string_view CampaignAbiUnavailable =
    "STRE 1.7.104 experimental port: native admission is incomplete "
    "(target binaries, save selection/Continue callbacks, pause menu, final appearance and seating). "
    "Game startup is unavailable. See issue #99 and RUNTIME_1_7_NATIVE_CONTRACTS.md.";
} // namespace STRE::Runtime
