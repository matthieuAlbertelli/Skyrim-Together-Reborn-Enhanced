#pragma once

#include <Interface/IMenu.h>
#include <string>

namespace MainMenuRuntime
{
// Private, display-only Scaleform host. Never registered on Skyrim's menu
// stack, never receives input and never changes the Main Menu movie.
class IntroPrompt final : public IMenu
{
public:
    IntroPrompt(std::uint32_t aScanCode, std::string aAction);
    ~IntroPrompt() override;
    void Render(void* apMainMenuMovie);
    void ReleaseMovie();

    void PostCreate() override {}
    void Unk_03() override {}
    UI_MESSAGE_RESULTS ProcessMessage(UIMessage&) override { return UI_MESSAGE_RESULTS::kIgnore; }
    void AdvanceMovie(float, std::uint32_t) override {}
    void PostDisplay() override {}
    void PreDisplay() override {}
    void RefreshPlatform() override {}

private:
    bool Initialize();
    bool Layout(int aWidth, int aHeight);
    void Fail();

    const std::uint32_t m_scanCode;
    const std::string m_action;
    bool m_attempted{};
    std::uint64_t m_started{};
    int m_width{};
    int m_height{};
    double m_keyHeight{};
};
} // namespace MainMenuRuntime
