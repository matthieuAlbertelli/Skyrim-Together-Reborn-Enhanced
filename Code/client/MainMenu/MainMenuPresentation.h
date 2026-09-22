#pragma once

#include "PresentationPolicy.h"
#include "VideoPlayer.h"

#include <atomic>
#include <filesystem>
#include <d3d11.h>
#include <wrl/client.h>

struct InputEvent;
struct ImguiService;
struct RenderSystemD3D11;

namespace STRE::MainMenu
{
class Presentation
{
public:
    Presentation(RenderSystemD3D11& aRenderer, ImguiService& aImgui, std::filesystem::path aDirectory, Config aConfig);
    // Native menu/input thread: atomics + input-thread-owned latch only.
    void MenuChanged(bool aOpen) noexcept;
    [[nodiscard]] bool ConsumeInput(const InputEvent* apEvent) noexcept;
    [[nodiscard]] bool CapturesInput() const noexcept;
    [[nodiscard]] bool SuppressesMusic() const noexcept;

    // Rendering thread only. True means the intro covered the vanilla menu.
    [[nodiscard]] bool Render();
    void EndFrame();
    void Disable();

private:
    void OpenCurrent();
    void Publish();

    RenderSystemD3D11& m_renderer;
    ImguiService& m_imgui;
    const std::filesystem::path m_directory;
    const Config m_config;
    const bool m_introAvailable;
    const bool m_backgroundAvailable;
    Controller m_controller;
    VideoPlayer m_video;
    InputLatch m_inputLatch;
    std::atomic<bool> m_open{};
    std::atomic<bool> m_introAttempted{};
    std::atomic<bool> m_skip{};
    std::atomic<bool> m_muteMenu{};
    std::atomic<bool> m_disabled{};
    std::atomic<std::uint64_t> m_captureUntil{};
    std::atomic<std::uint64_t> m_closedEpoch{};
    std::uint64_t m_observedEpoch{};
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_transitionFrame;
    unsigned m_transitionWidth{};
    unsigned m_transitionHeight{};
    bool m_opened{};
    bool m_audioRetry{};
    bool m_audioFailureLogged{};
    double m_lastRender{};
};
} // namespace STRE::MainMenu
