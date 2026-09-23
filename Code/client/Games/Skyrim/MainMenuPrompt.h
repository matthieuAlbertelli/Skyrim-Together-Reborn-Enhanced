#pragma once

#include <Interface/IMenu.h>
#include <string>

namespace MainMenuRuntime
{
// Private, display-only Scaleform host. Never registered on Skyrim's menu
// stack, never receives input and never changes the Main Menu movie.
class PresentationMovie : public IMenu
{
public:
    PresentationMovie();
    ~PresentationMovie() override;
    void ReleaseMovie();

    void PostCreate() override {}
    void Unk_03() override {}
    UI_MESSAGE_RESULTS ProcessMessage(UIMessage&) override { return UI_MESSAGE_RESULTS::kIgnore; }
    void AdvanceMovie(float, std::uint32_t) override {}
    void PostDisplay() override {}
    void PreDisplay() override {}
    void RefreshPlatform() override {}

protected:
    bool LoadLibrary();
    bool CopyViewport(void* apMainMenuMovie, int& aWidth, int& aHeight);
};

class IntroPrompt final : public PresentationMovie
{
public:
    IntroPrompt(std::uint32_t aScanCode, std::string aAction);
    void Render(void* apMainMenuMovie);

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

class MenuSubtitle final : public PresentationMovie
{
public:
    explicit MenuSubtitle(std::string aText);
    void Render(void* apMainMenuMovie, float aOpacity);

private:
    bool Layout(int aWidth, int aHeight);
    void Fail();
    const std::string m_text;
    bool m_attempted{};
    int m_width{}, m_height{};
};
} // namespace MainMenuRuntime
