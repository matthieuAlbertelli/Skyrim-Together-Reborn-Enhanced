#pragma once

#include <Interface/IMenu.h>
#include <Misc/BSFixedString.h>

class CampaignGateMenu final : public IMenu
{
public:
    static const BSFixedString& GetName() noexcept;
    static bool Register() noexcept;
    static void Show() noexcept;
    static void Hide() noexcept;
    static bool IsInstance(const IMenu* apMenu) noexcept;

    CampaignGateMenu() noexcept;
    ~CampaignGateMenu() override;

    void Accept(CallbackProcessor*) override;
    void PostCreate() override;
    void Unk_03() override;
    UI_MESSAGE_RESULTS ProcessMessage(UIMessage& aMessage) override;
    void AdvanceMovie(float, std::uint32_t) override;
    void PostDisplay() override;
    void PreDisplay() override;
    void RefreshPlatform() override;

private:
    static IMenu* Create(UIMessage*);

    bool m_postDisplayLogged{};
};
