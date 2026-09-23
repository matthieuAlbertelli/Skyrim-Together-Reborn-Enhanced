#include "PresentationPolicy.h"

#include <charconv>
#include <cmath>

namespace STRE::MainMenu
{
namespace
{
std::string_view Trim(std::string_view aValue) noexcept
{
    const auto first = aValue.find_first_not_of(" \t\r");
    if (first == std::string_view::npos)
        return {};
    const auto last = aValue.find_last_not_of(" \t\r");
    return aValue.substr(first, last - first + 1);
}
} // namespace

Config ParseConfig(std::string_view aText) noexcept
{
    Config result;
    if (aText.size() > 4096)
        return result;
    bool settings = false;
    bool branding = false;
    while (!aText.empty())
    {
        const auto end = aText.find('\n');
        auto line = Trim(aText.substr(0, end));
        aText = end == std::string_view::npos ? std::string_view{} : aText.substr(end + 1);
        line = Trim(line.substr(0, line.find_first_of(";#")));
        if (line.empty())
            continue;
        if (line.front() == '[')
        {
            settings = line == "[Presentation]";
            branding = line == "[Branding]";
            continue;
        }
        const auto equal = line.find('=');
        if ((!settings && !branding) || equal == std::string_view::npos)
            continue;
        const auto key = Trim(line.substr(0, equal));
        const auto value = Trim(line.substr(equal + 1));
        if (branding)
        {
            if (key == "BackdropEnabled")
            {
                if (value == "true" || value == "false")
                    result.Backdrop.Enabled = value == "true";
                continue;
            }
            float number{};
            const auto parsed = std::from_chars(value.data(), value.data() + value.size(), number);
            if (parsed.ec != std::errc{} || parsed.ptr != value.data() + value.size() || !std::isfinite(number))
                continue;
            if (key == "BackdropOpacity" && number >= 0 && number <= 1)
                result.Backdrop.Opacity = number;
            else if (key == "BackdropScale" && number >= 0.5f && number <= 1.5f)
                result.Backdrop.Scale = number;
            else if ((key == "BackdropOffsetX" || key == "BackdropOffsetY") && number >= -0.25f && number <= 0.25f)
                (key == "BackdropOffsetX" ? result.Backdrop.OffsetX : result.Backdrop.OffsetY) = number;
            continue;
        }
        if (key == "Enabled" || key == "IntroAudio")
        {
            if (value == "true" || value == "false")
                (key == "Enabled" ? result.Enabled : result.IntroAudio) = value == "true";
            continue;
        }
        std::uint32_t number{};
        const auto parsed = std::from_chars(value.data(), value.data() + value.size(), number);
        if (parsed.ec != std::errc{} || parsed.ptr != value.data() + value.size())
            continue;
        if (key == "SkipKeyboard" && number > 0 && number <= 255)
            result.SkipKeyboard = number;
        if (key == "SkipGamepad" && number > 0 && number <= 0x8000 && (number & (number - 1)) == 0)
            result.SkipGamepad = number;
    }
    return result;
}

Controller::Controller(bool aEnabled) noexcept
    : m_state(aEnabled ? State::WaitingForMainMenu : State::Disabled)
{
}

void Controller::Enter(double aNow, bool aIntroAvailable, bool aBackgroundAvailable) noexcept
{
    if (m_state != State::WaitingForMainMenu)
        return;
    m_backgroundAvailable = aBackgroundAvailable && !m_backgroundFailed;
    const bool intro = !m_introAttempted && aIntroAvailable;
    m_introAttempted = true;
    m_started = m_lastFrame = aNow;
    m_state = intro ? State::PlayingIntro : State::TransitionToMenu;
    if (!intro)
        AdvanceTransition(aNow);
}

void Controller::Leave() noexcept
{
    if (m_state != State::Disabled)
        m_state = State::WaitingForMainMenu;
}

void Controller::Update(double aNow, Playback aPlayback, bool aSkip) noexcept
{
    if (m_state != State::PlayingIntro && m_state != State::PlayingBackground)
        return;
    if (aPlayback == Playback::Frame)
        m_lastFrame = aNow;
    const bool expired = aNow - m_lastFrame >= cStallSeconds;
    if (m_state == State::PlayingIntro)
    {
        if (aSkip || aPlayback == Playback::Ended || aPlayback == Playback::Failed || expired || aNow - m_started >= cMaximumIntroSeconds)
            m_state = State::TransitionToMenu;
    }
    else if (aPlayback == Playback::Failed || aPlayback == Playback::Ended || expired)
    {
        m_backgroundFailed = true;
        m_state = State::VanillaFallback;
    }
}

void Controller::AdvanceTransition(double aNow) noexcept
{
    if (m_state != State::TransitionToMenu)
        return;
    m_state = m_backgroundAvailable ? State::PlayingBackground : State::VanillaFallback;
    m_started = m_lastFrame = aNow;
}

void Controller::Disable() noexcept
{
    m_introAttempted = true;
    m_state = State::Disabled;
}

bool InputLatch::Button(bool aCapture, std::uint32_t aDevice, std::uint32_t aCode, bool aDown) noexcept
{
    if (aDevice >= 3 || aCode >= 65536)
        return aCapture;
    const auto index = static_cast<std::size_t>(aDevice) * 65536 + aCode;
    const bool consume = aCapture || m_held[index];
    m_held[index] = aDown && consume;
    return consume;
}
} // namespace STRE::MainMenu
