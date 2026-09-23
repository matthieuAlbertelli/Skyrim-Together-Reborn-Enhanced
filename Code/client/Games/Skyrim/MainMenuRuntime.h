#pragma once

#include <cstddef>

// Uses Skyrim's top-level resetGame request. The distinct fullReset content
// reset is deliberately left untouched. Completion is still observed through
// CampaignMainMenuEnteredEvent before STRE clears local runtime state.
[[nodiscard]] bool RequestSkyrimMainMenu() noexcept;

struct RenderSystemD3D11;
struct ImguiService;
struct InputEvent;

namespace MainMenuRuntime
{
namespace Detail
{
// Shared memory preflight for this feature's hooks and optional native prompt.
bool Readable(const void* apAddress, std::size_t aSize, bool aExecutable = false);
} // namespace Detail

void InitializePresentation(RenderSystemD3D11& aRenderer, ImguiService& aImgui);
void EndPresentationFrame();
void ResetPresentation();
[[nodiscard]] bool ConsumePresentationInput(const InputEvent* apEvents) noexcept;
} // namespace MainMenuRuntime
