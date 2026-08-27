#pragma once

#include <cstdint>

namespace CampaignSaveTrace
{
struct Context
{
    std::uint64_t Sequence{};
    std::uint64_t Frame{};
    std::uint32_t Thread{};
};

void AdvanceFrame() noexcept;
[[nodiscard]] Context Capture() noexcept;

// Exact call-stack correlation for the vanilla Main Menu
// GameDelegate.call("ContinueLastSavedGame") callback. This is diagnostic
// provenance only and never grants load authority.
void EnterContinueCallback() noexcept;
void ExitContinueCallback() noexcept;
[[nodiscard]] bool IsContinueCallbackActive() noexcept;
}
