#pragma once

// Uses Skyrim's top-level resetGame request. The distinct fullReset content
// reset is deliberately left untouched. Completion is still observed through
// CampaignMainMenuEnteredEvent before STRE clears local runtime state.
[[nodiscard]] bool RequestSkyrimMainMenu() noexcept;
