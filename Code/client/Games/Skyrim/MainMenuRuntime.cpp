#include <TiltedOnlinePCH.h>

#include <Games/Skyrim/MainMenuRuntime.h>
#include <Games/Skyrim/MainMenuPrompt.h>
#include <Interface/IMenu.h>
#include <Interface/UI.h>
#include <MainMenu/MainMenuPresentation.h>
#include <MainMenu/Localization.h>
#include <Games/TES.h>
#include "../../../immersive_launcher/Launcher.h"

#include <array>
#include <atomic>
#include <limits>

namespace
{
// CommonLibSSE-NG RE::Main layout (AE 1.6.1170): two event-sink bases occupy
// 0x10 bytes, followed by the three distinct runtime request flags.
struct SkyrimMainResetBoundary
{
    std::byte EventSinkBases[0x10];
    bool QuitGame;
    bool ResetGame;
    bool FullReset;
};

static_assert(offsetof(SkyrimMainResetBoundary, ResetGame) == 0x11);
static_assert(offsetof(SkyrimMainResetBoundary, FullReset) == 0x12);
} // namespace

bool RequestSkyrimMainMenu() noexcept
{
    // CommonLibSSE-NG RE::Offset::Main::Singleton, AE Address Library ID.
    POINTER_SKYRIMSE(SkyrimMainResetBoundary*, s_pMain, 403449);
    SkyrimMainResetBoundary** const ppMain = s_pMain.Get();
    if (!ppMain)
    {
        spdlog::error("[STRE][MainMenuRuntime] REQUEST_FAILED reason=singleton-address-unavailable");
        return false;
    }

    SkyrimMainResetBoundary* const pMain = *ppMain;
    if (!pMain)
    {
        spdlog::error("[STRE][MainMenuRuntime] REQUEST_FAILED reason=singleton-unavailable");
        return false;
    }

    const bool alreadyRequested = pMain->ResetGame;
    spdlog::info("[STRE][MainMenuRuntime] REQUEST resetGameBefore={} fullResetBefore={} action=set-reset-game-only", alreadyRequested, pMain->FullReset);
    pMain->ResetGame = true;
    return true;
}

// Main Menu render ordering and the two music-predicate call sites are adapted
// from powerofthree's MainMenuVideo (GPL-3.0-or-later), commit
// ec692f0745972ba3b381e2b1df5c4c56218ee8e0, src/ImGui/Renderer.h and src/Hooks.cpp.
// STRE changes: reuse existing renderer/context/input seam, exact runtime gate,
// preflight all patches, independent originals, process-local fail-open policy.
// See NOTICE.md and GameFiles/Skyrim/STRE/Licenses/MainMenuVideo-GPL-3.0.txt.
namespace MainMenuRuntime
{
bool Detail::Readable(const void* apAddress, std::size_t aSize, bool aExecutable)
{
    MEMORY_BASIC_INFORMATION info{};
    if (!apAddress || !VirtualQuery(apAddress, &info, sizeof(info)) || info.State != MEM_COMMIT || (info.Protect & (PAGE_NOACCESS | PAGE_GUARD)) != 0)
        return false;
    const auto offset = reinterpret_cast<std::uintptr_t>(apAddress) - reinterpret_cast<std::uintptr_t>(info.BaseAddress);
    return offset < info.RegionSize && aSize <= info.RegionSize - offset &&
           (!aExecutable || (info.Protect & (PAGE_EXECUTE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY)) != 0);
}

namespace
{
using Detail::Readable;
using STRE::MainMenu::Presentation;
using ProcessMessage = UI_MESSAGE_RESULTS (*)(IMenu*, UIMessage&);
using PostDisplay = void (*)(IMenu*);
using MusicPredicate = bool (*)();
ProcessMessage s_processMessage{};
PostDisplay s_postDisplay{};
PostDisplay s_cursorPostDisplay{};
MusicPredicate s_musicAtCreate{};
MusicPredicate s_musicAtUpdate{};
bool s_hooksReady{};
std::unique_ptr<Presentation> s_owner;
std::unique_ptr<IntroPrompt> s_prompt;
std::atomic<Presentation*> s_presentation{};

UI_MESSAGE_RESULTS ProcessMessageHook(IMenu* apMenu, UIMessage& aMessage)
{
    auto* presentation = s_presentation.load();
    if (presentation)
    {
        if (aMessage.eType == UIMessage::kShow || aMessage.eType == UIMessage::kReshow)
            presentation->MenuChanged(true);
        else if (aMessage.eType == UIMessage::kHide || aMessage.eType == UIMessage::kForceHide)
            presentation->MenuChanged(false);
        else if ((aMessage.eType == UIMessage::kScaleformEvent || aMessage.eType == UIMessage::kUserEvent) && presentation->CapturesInput())
            return UI_MESSAGE_RESULTS::kHandled;
    }
    return s_processMessage(apMenu, aMessage);
}

void PostDisplayHook(IMenu* apMenu)
{
    auto* presentation = s_presentation.load();
    // Background is drawn BEFORE vanilla; intro alone suppresses its display.
    if (presentation && presentation->Render())
    {
        if (s_prompt && presentation->HidesCursor())
            s_prompt->Render(apMenu->uiMovie);
        return;
    }
    if (s_prompt)
        s_prompt->ReleaseMovie();
    s_postDisplay(apMenu);
}

void CursorPostDisplayHook(IMenu* apMenu)
{
    // CursorMenu is a separate native Scaleform movie, above MainMenu. Only
    // omit its draw: preserve lifecycle, position, visibility flags and all OS
    // / CEF cursor ownership. Every exit automatically chains to vanilla again.
    auto* presentation = s_presentation.load();
    if (!presentation || !presentation->HidesCursor())
        s_cursorPostDisplay(apMenu);
}

bool MusicAtCreateHook()
{
    auto* presentation = s_presentation.load();
    return (presentation && presentation->SuppressesMusic()) || s_musicAtCreate();
}

bool MusicAtUpdateHook()
{
    auto* presentation = s_presentation.load();
    return (presentation && presentation->SuppressesMusic()) || s_musicAtUpdate();
}

// Presentation patch addresses stay here; optional movie ABIs are isolated in
// MainMenuPrompt. Both adapters reject unknown runtimes. There is no 1.7 map.
bool InstallHooks()
{
    auto& database = VersionDb::Get();
    if (database.GetLoadedVersionString() != "1.6.1170.0")
    {
        spdlog::warn("[STRE][MainMenu] disabled unsupported-runtime={}", database.GetLoadedVersionString());
        return false;
    }
    // CommonLibSSE-NG VTABLE_MainMenu[0]. Slots 4/6 are ProcessMessage/PostDisplay.
    auto** table = static_cast<void**>(database.FindAddressById(215698));
    // CommonLibSSE-NG VTABLE_CursorMenu[0], inherited IMenu::PostDisplay (6).
    auto** cursorTable = static_cast<void**>(database.FindAddressById(215246));
    constexpr std::array<std::uint32_t, 2> ids{52110, 52137};
    constexpr std::array<std::size_t, 2> offsets{0x2A, 0x2DD};
    std::array<std::uint8_t*, 2> calls{};
    const std::array<MusicPredicate, 2> hooks{MusicAtCreateHook, MusicAtUpdateHook};
    if (!Readable(table, 7 * sizeof(void*)) || !Readable(table[4], 1, true) || !Readable(table[6], 1, true))
        return false;
    if (!Readable(cursorTable, 7 * sizeof(void*)) || !Readable(cursorTable[6], 1, true))
        return false;
    for (std::size_t i = 0; i < calls.size(); ++i)
    {
        auto* base = static_cast<std::uint8_t*>(database.FindAddressById(ids[i]));
        if (!base)
            return false;
        calls[i] = base + offsets[i];
        if (!Readable(calls[i], 5, true) || calls[i][0] != 0xE8)
            return false;
        std::int32_t displacement{};
        std::memcpy(&displacement, calls[i] + 1, sizeof(displacement));
        if (!Readable(calls[i] + 5 + displacement, 1, true))
            return false;
        const auto distance = reinterpret_cast<std::intptr_t>(hooks[i]) - reinterpret_cast<std::intptr_t>(calls[i] + 5);
        if (distance < std::numeric_limits<std::int32_t>::min() || distance > std::numeric_limits<std::int32_t>::max())
            return false;
    }
    // Establish all write permissions before changing any byte. Failure leaves
    // the original menu/music intact; no half-installed presentation is enabled.
    const std::array<void*, 4> addresses{table + 4, calls[0], calls[1], cursorTable + 6};
    constexpr std::array<std::size_t, 4> sizes{3 * sizeof(void*), 5, 5, sizeof(void*)};
    std::array<DWORD, 4> protections{};
    std::size_t writable{};
    for (; writable < addresses.size(); ++writable)
    {
        if (!VirtualProtect(addresses[writable], sizes[writable], PAGE_EXECUTE_READWRITE, &protections[writable]))
            break;
    }
    const bool ready = writable == addresses.size();
    if (ready)
    {
        s_processMessage = reinterpret_cast<ProcessMessage>(table[4]);
        s_postDisplay = reinterpret_cast<PostDisplay>(table[6]);
        s_cursorPostDisplay = reinterpret_cast<PostDisplay>(cursorTable[6]);
        TiltedPhoques::SwapCall(mem::pointer(calls[0]), s_musicAtCreate, &MusicAtCreateHook);
        TiltedPhoques::SwapCall(mem::pointer(calls[1]), s_musicAtUpdate, &MusicAtUpdateHook);
        table[4] = reinterpret_cast<void*>(&ProcessMessageHook);
        table[6] = reinterpret_cast<void*>(&PostDisplayHook);
        cursorTable[6] = reinterpret_cast<void*>(&CursorPostDisplayHook);
        FlushInstructionCache(GetCurrentProcess(), nullptr, 0);
    }
    while (writable)
    {
        --writable;
        DWORD ignored{};
        VirtualProtect(addresses[writable], sizes[writable], protections[writable], &ignored);
    }
    return ready;
}

template <std::size_t MaxBytes> std::string ReadBoundedText(const std::filesystem::path& aFile)
{
    std::ifstream file(aFile, std::ios::binary);
    std::array<char, MaxBytes + 1> text{};
    file.read(text.data(), text.size());
    return std::string(text.data(), static_cast<std::size_t>(file.gcount()));
}

std::string_view SkyrimLanguage()
{
    auto* settings = INISettingCollection::Get();
    // Read the existing native settings boundary; no CEF/localStorage access
    // or dependency on browser startup is needed for the first menu.
    auto* setting = settings ? settings->GetSetting("sLanguage:General") : nullptr;
    const auto* value = setting ? reinterpret_cast<const char*>(setting->data) : nullptr;
    return Readable(value, 64) ? std::string_view(value, strnlen_s(value, 64)) : std::string_view{};
}

static TiltedPhoques::Initializer s_installPresentation(
    []()
    {
        s_hooksReady = InstallHooks();
        spdlog::info("[STRE][MainMenu] hooks={} target=1.6.1170.0", s_hooksReady);
    });
} // namespace

void InitializePresentation(RenderSystemD3D11& aRenderer, ImguiService& aImgui)
{
    if (!s_hooksReady || s_owner || GetModuleHandleW(L"po3_MainMenuVideo.dll"))
        return;
    const auto* context = launcher::GetLaunchContext();
    if (!context)
        return;
    const auto directory = context->gamePath / "Data" / STRE::MainMenu::cAssetDirectory;
    const auto text = ReadBoundedText<4096>(directory / STRE::MainMenu::cConfigFile);
    const auto config = STRE::MainMenu::ParseConfig(text);
    auto action = STRE::MainMenu::ResolveSkipAction(ReadBoundedText<16384>(directory / STRE::MainMenu::cHintCatalogFile), text, SkyrimLanguage());
    s_prompt = std::make_unique<IntroPrompt>(config.SkipKeyboard, std::move(action));
    s_owner = std::make_unique<Presentation>(aRenderer, aImgui, directory, config);
    s_presentation.store(s_owner.get());
}

bool ConsumePresentationInput(const InputEvent* apEvents) noexcept
{
    auto* presentation = s_presentation.load();
    return presentation && presentation->ConsumeInput(apEvents);
}

void EndPresentationFrame()
{
    if (s_owner)
    {
        if (GetModuleHandleW(L"po3_MainMenuVideo.dll"))
            s_owner->Disable();
        s_owner->EndFrame();
        if (s_prompt && !s_owner->HidesCursor())
            s_prompt->ReleaseMovie();
    }
}

void ResetPresentation()
{
    if (s_owner)
        s_owner->Disable();
    if (s_prompt)
        s_prompt->ReleaseMovie();
}
} // namespace MainMenuRuntime
