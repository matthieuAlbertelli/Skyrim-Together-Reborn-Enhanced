#pragma once

#include <cstdint>

namespace STRE::Campaign
{
// CommonLibSSE-NG BGSSaveLoadManager::Save_Impl ABI. Keep this distinct from
// Load_Impl, whose filename is the first argument after self.
template <class TSelf>
using CampaignNativeSaveFunction = bool(
    TSelf*,
    std::int32_t,
    std::uint32_t,
    const char*);

struct CampaignNativeSaveArguments
{
    std::int32_t DeviceId{};
    std::uint32_t OutputStats{};
    const char* FileName{};
};

template <class TSelf>
[[nodiscard]] bool InvokeCampaignNativeSave(
    CampaignNativeSaveFunction<TSelf>* apFunction,
    TSelf* apSelf,
    const CampaignNativeSaveArguments& acArguments) noexcept
{
    return apFunction(
        apSelf,
        acArguments.DeviceId,
        acArguments.OutputStats,
        acArguments.FileName);
}
}
