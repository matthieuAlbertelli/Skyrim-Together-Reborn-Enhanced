#include <TiltedOnlinePCH.h>

#include "MainMenuPresentation.h"

#include <BSInput/InputEvent.h>
#include <Services/ImguiService.h>
#include <Systems/RenderSystemD3D11.h>

namespace STRE::MainMenu
{
namespace
{
double Now() noexcept
{
    return static_cast<double>(GetTickCount64()) / 1000.0;
}

bool HasFile(const std::filesystem::path& aFile)
{
    std::error_code ec;
    return std::filesystem::is_regular_file(aFile, ec) && !ec;
}
} // namespace

Presentation::Presentation(RenderSystemD3D11& aRenderer, ImguiService& aImgui, std::filesystem::path aDirectory, Config aConfig)
    : m_renderer(aRenderer)
    , m_imgui(aImgui)
    , m_directory(std::move(aDirectory))
    , m_config(aConfig)
    , m_introAvailable(HasFile(m_directory / cIntroFile))
    , m_backgroundAvailable(HasFile(m_directory / cBackgroundFile))
    , m_controller(aConfig.Enabled)
{
    // Arm before MainMenu's music trigger runs; no engine music is stopped or
    // user volume changed. The native check resumes when this lease expires.
    m_captureUntil = GetTickCount64() + 2000;
    m_muteMenu = aConfig.Enabled && aConfig.IntroAudio && m_introAvailable;
    spdlog::info(
        "[STRE][MainMenu] configured enabled={} intro={} background={} audio={} keys={}/{}", aConfig.Enabled, m_introAvailable, m_backgroundAvailable, aConfig.IntroAudio,
        aConfig.SkipKeyboard, aConfig.SkipGamepad);
}

void Presentation::MenuChanged(bool aOpen) noexcept
{
    const bool wasOpen = m_open.exchange(aOpen);
    if (aOpen && !m_introAttempted && m_introAvailable && m_config.Enabled && !m_disabled)
    {
        m_captureUntil = GetTickCount64() + 2000;
        m_muteMenu = m_config.IntroAudio;
    }
    else if (!aOpen)
    {
        if (wasOpen)
        {
            m_introAttempted = true;
            ++m_closedEpoch;
        }
        m_captureUntil = 0;
        m_muteMenu = false;
    }
}

bool Presentation::CapturesInput() const noexcept
{
    return m_config.Enabled && m_introAvailable && !m_disabled && m_open && GetTickCount64() < m_captureUntil.load();
}

bool Presentation::SuppressesMusic() const noexcept
{
    return !m_disabled && m_muteMenu && GetTickCount64() < m_captureUntil.load();
}

bool Presentation::ConsumeInput(const InputEvent* apEvent) noexcept
{
    const bool capture = CapturesInput();
    bool consume = capture;
    // Skyrim owns this list. Never mutate it or retain engine event pointers.
    for (auto* event = apEvent; event; event = event->pNext)
    {
        if (event->EventType != 0)
            continue;
        const auto* button = static_cast<const ButtonInputEvent*>(event);
        consume = m_inputLatch.Button(capture, button->Device, button->IdCode, button->Value > 0.0f) || consume;
        if (capture && button->Value > 0.0f && button->HeldDownSeconds == 0.0f &&
            ((button->Device == 0 && button->IdCode == m_config.SkipKeyboard) || (button->Device == 2 && button->IdCode == m_config.SkipGamepad)))
            m_skip = true;
    }
    return consume;
}

void Presentation::OpenCurrent()
{
    const bool intro = m_controller.GetState() == State::PlayingIntro;
    if (!intro && m_controller.GetState() != State::PlayingBackground)
    {
        m_video.Stop();
        return;
    }
    const auto file = m_directory / (intro ? cIntroFile : cBackgroundFile);
    const bool opened = m_video.Open(m_renderer.GetDevice(), file, !intro, intro && m_config.IntroAudio && !m_audioRetry);
    spdlog::info("[STRE][MainMenu] open clip={} result={} hr={:08X}", intro ? "intro" : "background", opened, static_cast<std::uint32_t>(m_video.Error()));
}

void Presentation::Publish()
{
    const bool intro = m_controller.BlocksMenu();
    m_captureUntil = intro ? GetTickCount64() + 2000 : 0;
    m_muteMenu = intro && m_config.IntroAudio && !m_audioRetry && !m_video.AudioFailed();
}

bool Presentation::Render()
{
    if (m_disabled || !m_open)
        return false;
    const double now = Now();
    m_lastRender = now;
    const auto epoch = m_closedEpoch.load();
    if (epoch != m_observedEpoch)
    {
        m_observedEpoch = epoch;
        m_controller.Leave();
        m_video.Stop();
        m_transitionFrame.Reset();
        m_opened = false;
    }
    if (!m_opened)
    {
        m_opened = true;
        m_controller.Enter(now, m_introAvailable && !m_introAttempted, m_backgroundAvailable);
        m_introAttempted = m_controller.IntroAttempted();
        OpenCurrent();
    }
    auto state = m_controller.GetState();
    if (state != State::PlayingIntro && state != State::PlayingBackground)
        return false;
    const auto result = m_video.Update();
    if (result == Playback::Failed && state == State::PlayingIntro && m_config.IntroAudio && !m_audioRetry)
    {
        // One bounded silent retry also covers an unusable audio endpoint.
        // The controller's original load/stall deadline is NOT restarted.
        m_audioRetry = true;
        spdlog::warn("[STRE][MainMenu] intro retry=muted hr={:08X}", static_cast<std::uint32_t>(m_video.Error()));
        OpenCurrent();
    }
    else
    {
        const bool skip = m_skip.exchange(false);
        m_controller.Update(now, result, skip);
        if (m_controller.GetState() != state)
        {
            spdlog::info(
                "[STRE][MainMenu] transition from={} to={} playback={} skip={} hr={:08X}", static_cast<int>(state), static_cast<int>(m_controller.GetState()),
                static_cast<int>(result), skip, static_cast<std::uint32_t>(m_video.Error()));
            m_controller.AdvanceTransition(now);
            if (state == State::PlayingIntro && m_controller.GetState() == State::PlayingBackground)
            {
                m_transitionFrame = m_video.Texture();
                m_transitionWidth = m_video.Width();
                m_transitionHeight = m_video.Height();
            }
            OpenCurrent();
        }
    }
    if (m_video.AudioFailed() && !m_audioFailureLogged)
    {
        m_audioFailureLogged = true;
        spdlog::warn("[STRE][MainMenu] audio unavailable; video and menu remain usable");
    }
    Publish();
    state = m_controller.GetState();
    if (m_video.Texture() || state != State::PlayingBackground)
        m_transitionFrame.Reset();
    const bool intro = state == State::PlayingIntro;
    auto* texture = m_video.Texture();
    const bool retained = !texture && m_transitionFrame;
    if (retained)
        texture = m_transitionFrame.Get();
    if (intro || (state == State::PlayingBackground && texture))
    {
        if (!m_imgui.RenderMainMenuTexture(
                texture, retained ? m_transitionWidth : m_video.Width(), retained ? m_transitionHeight : m_video.Height(), m_renderer.GetDeviceContext()))
        {
            spdlog::warn("[STRE][MainMenu] fallback reason=render-unavailable");
            Disable();
            return false;
        }
    }
    return intro;
}

void Presentation::EndFrame()
{
    if (!m_open && m_opened)
    {
        m_controller.Leave();
        m_video.Stop();
        m_transitionFrame.Reset();
        m_opened = false;
        m_skip = false;
        Publish();
    }
    // A lost/replaced PostDisplay hook must not leave audio/input captured.
    if (m_opened && Now() - m_lastRender > 2.0)
    {
        spdlog::warn("[STRE][MainMenu] fallback reason=render-heartbeat-lost");
        Disable();
    }
}

void Presentation::Disable()
{
    m_disabled = true;
    m_controller.Disable();
    m_video.Stop();
    m_transitionFrame.Reset();
    Publish();
}
} // namespace STRE::MainMenu
