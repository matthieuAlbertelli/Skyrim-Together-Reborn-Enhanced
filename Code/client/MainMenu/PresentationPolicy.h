#pragma once

#include <bitset>
#include <cstdint>
#include <string_view>

namespace STRE::MainMenu
{
inline constexpr std::string_view cAssetDirectory = "STRE/MainMenu";
inline constexpr std::string_view cIntroFile = "intro.mp4";
inline constexpr std::string_view cBackgroundFile = "background.mp4";
inline constexpr std::string_view cConfigFile = "presentation.ini";

struct Config
{
    bool Enabled{true};
    bool IntroAudio{true};
    std::uint32_t SkipKeyboard{1};     // Skyrim keyboard scan code: Escape.
    std::uint32_t SkipGamepad{0x2000}; // Skyrim/XInput button mask: B.
};

// Bounded, non-throwing parser; invalid values retain defaults. Asset paths are
// deliberately not configurable: this presentation never accepts URLs.
[[nodiscard]] Config ParseConfig(std::string_view aText) noexcept;

enum class State
{
    Disabled,
    WaitingForMainMenu,
    PlayingIntro,
    TransitionToMenu,
    PlayingBackground,
    VanillaFallback
};

enum class Playback
{
    Pending,
    Frame,
    Ended,
    Failed
};

class Controller
{
public:
    explicit Controller(bool aEnabled = true) noexcept;
    void Enter(double aNow, bool aIntroAvailable, bool aBackgroundAvailable) noexcept;
    void Leave() noexcept;
    void Update(double aNow, Playback aPlayback, bool aSkip) noexcept;
    void AdvanceTransition(double aNow) noexcept;
    void Disable() noexcept;
    [[nodiscard]] State GetState() const noexcept { return m_state; }
    [[nodiscard]] bool IntroAttempted() const noexcept { return m_introAttempted; }
    [[nodiscard]] bool BlocksMenu() const noexcept { return m_state == State::PlayingIntro; }

    static constexpr double cStallSeconds = 8.0;
    static constexpr double cMaximumIntroSeconds = 600.0;

private:
    State m_state;
    bool m_introAttempted{};
    bool m_backgroundAvailable{};
    bool m_backgroundFailed{};
    double m_started{};
    double m_lastFrame{};
};

// Owned by the engine input thread. Consume buttons held across the transition,
// including their release, so the skip press cannot activate Continue/New Game.
class InputLatch
{
public:
    [[nodiscard]] bool Button(bool aCapture, std::uint32_t aDevice, std::uint32_t aCode, bool aDown) noexcept;
    void Clear() noexcept { m_held.reset(); }

private:
    std::bitset<3 * 65536> m_held;
};
} // namespace STRE::MainMenu
