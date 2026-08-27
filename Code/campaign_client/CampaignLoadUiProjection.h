#pragma once

#include <CampaignLoadPolicy.h>

#include <cstdint>

namespace STRE::Campaign
{
enum class CampaignLoadUiAction : std::uint8_t
{
    ForwardNative,
    ConsumeAndCloseJournal,
    ConsumeAndRebuildMainMenu
};

enum class CampaignLoadUiHost : std::uint8_t
{
    None,
    Journal,
    MainMenu
};

// Projects the common policy decision onto the owning Skyrim menu of the
// semantic callback. This is deliberately stateless: each proven semantic
// seam owns its exact attempt correlation and Load_Impl remains the final
// safety boundary.
[[nodiscard]] constexpr CampaignLoadUiAction ProjectCampaignLoadUiAction(
    CampaignLoadDecision aDecision,
    CampaignLoadUiHost aHost = CampaignLoadUiHost::Journal) noexcept
{
    if (aDecision != CampaignLoadDecision::BlockPlayerLoad &&
        aDecision != CampaignLoadDecision::BlockUnprovenCampaignTarget)
    {
        return CampaignLoadUiAction::ForwardNative;
    }
    return aHost == CampaignLoadUiHost::MainMenu
        ? CampaignLoadUiAction::ConsumeAndRebuildMainMenu
        : CampaignLoadUiAction::ConsumeAndCloseJournal;
}
}
