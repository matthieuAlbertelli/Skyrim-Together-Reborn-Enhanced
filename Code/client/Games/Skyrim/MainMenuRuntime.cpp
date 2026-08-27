#include <TiltedOnlinePCH.h>

#include <Games/Skyrim/MainMenuRuntime.h>

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
}

bool RequestSkyrimMainMenu() noexcept
{
    // CommonLibSSE-NG RE::Offset::Main::Singleton, AE Address Library ID.
    POINTER_SKYRIMSE(SkyrimMainResetBoundary*, s_pMain, 403449);
    SkyrimMainResetBoundary** const ppMain = s_pMain.Get();
    if (!ppMain)
    {
        spdlog::error(
            "[STRE][MainMenuRuntime] REQUEST_FAILED reason=singleton-address-unavailable");
        return false;
    }

    SkyrimMainResetBoundary* const pMain = *ppMain;
    if (!pMain)
    {
        spdlog::error(
            "[STRE][MainMenuRuntime] REQUEST_FAILED reason=singleton-unavailable");
        return false;
    }

    const bool alreadyRequested = pMain->ResetGame;
    spdlog::info(
        "[STRE][MainMenuRuntime] REQUEST resetGameBefore={} fullResetBefore={} action=set-reset-game-only",
        alreadyRequested, pMain->FullReset);
    pMain->ResetGame = true;
    return true;
}
