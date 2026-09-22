#pragma once

#include <imgui/ImGuiDriver.h>
#include <string_view>

struct RenderSystemD3D9;
struct RenderSystemD3D11;
struct ID3D11ShaderResourceView;
struct ID3D11DeviceContext;

/**
 * @brief Draws the ImGui UI.
 */
struct ImguiService
{
    using TCallback = void();

    ImguiService();
    ~ImguiService() noexcept;

    TP_NOCOPYMOVE(ImguiService);

    void Create(RenderSystemD3D11* apRenderSystem, HWND aHwnd);

    void Render() const;
    void Reset() const;

    // Feature-local draw list through the existing backend, without another
    // ImGui context/NewFrame or swapchain. Null texture draws an opaque cover.
    [[nodiscard]] bool RenderMainMenuTexture(
        ID3D11ShaderResourceView* apTexture, unsigned aWidth, unsigned aHeight, ID3D11DeviceContext* apContext, std::string_view aHint = {}, float aHintAlpha = 0.0f) const;

    LRESULT WndProcHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    void RawInputHandler(RAWINPUT& aRawinput);

    entt::sink<entt::sigh<TCallback>> OnDraw;

private:
    ImGuiImpl::ImGuiDriver m_imDriver;
    entt::sigh<TCallback> m_drawSignal;
    bool m_ready{};
};
