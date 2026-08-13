#include <TiltedOnlinePCH.h>

#include <Services/CampaignRuntimeGateService.h>

struct BGSSaveLoadManager;

TP_THIS_FUNCTION(
    TCampaignGateLoadGame,
    bool,
    BGSSaveLoadManager,
    const char*,
    std::int32_t,
    std::uint32_t,
    bool);

static TCampaignGateLoadGame* s_realLoadGame{};

bool TP_MAKE_THISCALL(
    HookCampaignGateLoadGame,
    BGSSaveLoadManager,
    const char* apSaveName,
    std::int32_t aDeviceId,
    std::uint32_t aOutputStats,
    bool aCheckForMods)
{
    CampaignRuntimeGateService* const pGate =
        CampaignRuntimeGateService::TryGet();
    const bool managed =
        pGate && pGate->OnNativeLoadEnter(apSaveName);

    const bool result = TiltedPhoques::ThisCall(
        s_realLoadGame,
        apThis,
        apSaveName,
        aDeviceId,
        aOutputStats,
        aCheckForMods);

    if (pGate)
        pGate->OnNativeLoadReturn(managed, result);

    return result;
}

static TiltedPhoques::Initializer s_campaignSaveLoadGateHook(
    []()
    {
        // CommonLibSSE-NG BGSSaveLoadManager::Load_Impl, AE Address Library ID.
        POINTER_SKYRIMSE(
            TCampaignGateLoadGame,
            loadGame,
            35728);
        s_realLoadGame = loadGame.Get();
        TP_HOOK(&s_realLoadGame, HookCampaignGateLoadGame);
    });
