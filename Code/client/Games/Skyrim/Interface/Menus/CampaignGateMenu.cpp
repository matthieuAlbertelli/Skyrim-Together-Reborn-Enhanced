#include <TiltedOnlinePCH.h>

#include <Interface/Menus/CampaignGateMenu.h>

#include <Interface/UI.h>
#include <Services/CampaignRuntimeGateService.h>

namespace
{
constexpr const char* kCampaignGateMenuName = "STRECampaignGateMenu";
constexpr std::uint32_t kCampaignGateFlags =
    IMenu::kPausesGame |
    IMenu::kModal |
    IMenu::kDisablePauseMenu;

static_assert((kCampaignGateFlags & IMenu::kAllowSaving) == 0);

const IMenu* s_pCampaignGateMenu{};
}

const BSFixedString& CampaignGateMenu::GetName() noexcept
{
    static BSFixedString s_name{kCampaignGateMenuName};
    return s_name;
}

bool CampaignGateMenu::Register() noexcept
{
    UI* const pUI = UI::Get();
    return pUI &&
        pUI->RegisterMenu(GetName(), &CampaignGateMenu::Create);
}

void CampaignGateMenu::Show() noexcept
{
    UI* const pUI = UI::Get();
    if (!pUI)
        return;

    if (!pUI->HasMenuRegistration(GetName()) && !Register())
    {
        spdlog::error(
            "[STRE][CampaignGate] GuardMenuRegistrationFailed");
        return;
    }

    pUI->QueueMessage(GetName(), UIMessage::kShow);
}

void CampaignGateMenu::Hide() noexcept
{
    UI* const pUI = UI::Get();
    if (pUI && pUI->HasMenuRegistration(GetName()))
        pUI->QueueMessage(GetName(), UIMessage::kHide);
}

bool CampaignGateMenu::IsInstance(const IMenu* apMenu) noexcept
{
    return apMenu && apMenu == s_pCampaignGateMenu;
}

CampaignGateMenu::CampaignGateMenu() noexcept
{
    uiMovie = nullptr;
    depthPriority = 0;
    eInputContext = 0;
    uiMenuFlags = kCampaignGateFlags;
    s_pCampaignGateMenu = this;
}

CampaignGateMenu::~CampaignGateMenu()
{
    if (s_pCampaignGateMenu == this)
        s_pCampaignGateMenu = nullptr;

    if (CampaignRuntimeGateService* const pGate =
            CampaignRuntimeGateService::TryGet())
    {
        pGate->OnGuardMenuDestroyed();
    }
}

void CampaignGateMenu::Accept(CallbackProcessor*)
{
}

void CampaignGateMenu::PostCreate()
{
}

void CampaignGateMenu::Unk_03()
{
}

UI_MESSAGE_RESULTS CampaignGateMenu::ProcessMessage(UIMessage& aMessage)
{
    if ((aMessage.eType == UIMessage::kHide ||
         aMessage.eType == UIMessage::kForceHide) &&
        CampaignRuntimeGateService::TryGet() &&
        CampaignRuntimeGateService::TryGet()->IsLocked())
    {
        return UI_MESSAGE_RESULTS::kIgnore;
    }

    return UI_MESSAGE_RESULTS::kPassOn;
}

void CampaignGateMenu::AdvanceMovie(float, std::uint32_t)
{
}

void CampaignGateMenu::PostDisplay()
{
    if (m_postDisplayLogged)
        return;

    m_postDisplayLogged = true;
    if (CampaignRuntimeGateService* const pGate =
            CampaignRuntimeGateService::TryGet())
    {
        pGate->OnGuardMenuPostDisplay();
    }
}

void CampaignGateMenu::PreDisplay()
{
}

void CampaignGateMenu::RefreshPlatform()
{
}

IMenu* CampaignGateMenu::Create(UIMessage*)
{
    return new CampaignGateMenu();
}
