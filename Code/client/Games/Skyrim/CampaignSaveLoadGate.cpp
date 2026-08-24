#include <TiltedOnlinePCH.h>

#include <CampaignNativeLoad.h>
#include <Services/CampaignNativeLoadService.h>
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
static BGSSaveLoadManager** s_ppSaveLoadManager{};

bool TP_MAKE_THISCALL(
    HookCampaignGateLoadGame,
    BGSSaveLoadManager,
    const char* apSaveName,
    std::int32_t aDeviceId,
    std::uint32_t aOutputStats,
    bool aCheckForMods)
{
    CampaignNativeLoadService* const pLoad =
        CampaignNativeLoadService::TryGet();
    const bool correlated =
        pLoad && pLoad->OnNativeLoadEnter(apSaveName);
    CampaignRuntimeGateService* const pGate =
        CampaignRuntimeGateService::TryGet();
    const bool managed =
        correlated && pGate && pGate->OnNativeLoadEnter(apSaveName);
    if (correlated && !managed)
    {
        pLoad->OnGateArmFailure();
        return false;
    }

    const bool result = TiltedPhoques::ThisCall(
        s_realLoadGame,
        apThis,
        apSaveName,
        aDeviceId,
        aOutputStats,
        aCheckForMods);

    if (pGate)
        pGate->OnNativeLoadReturn(managed, result);
    if (pLoad)
        pLoad->OnNativeLoadReturn(managed, result);

    return result;
}

CampaignNativeLoadInvokeResult CampaignNativeLoad::InvokeValidated(
    std::string_view acNativeSaveIdentity) noexcept
{
    try
    {
        if (acNativeSaveIdentity.empty() || !s_realLoadGame ||
            !s_ppSaveLoadManager || !*s_ppSaveLoadManager)
        {
            return CampaignNativeLoadInvokeResult::BoundaryUnavailable;
        }

        const std::string identity{
            acNativeSaveIdentity.data(), acNativeSaveIdentity.size()};
        const bool accepted = HookCampaignGateLoadGame(
            *s_ppSaveLoadManager,
            identity.c_str(),
            -1,
            0,
            true);
        return accepted
            ? CampaignNativeLoadInvokeResult::NativeAccepted
            : CampaignNativeLoadInvokeResult::NativeRejected;
    }
    catch (...)
    {
        return CampaignNativeLoadInvokeResult::BoundaryUnavailable;
    }
}

static TiltedPhoques::Initializer s_campaignSaveLoadGateHook(
    []()
    {
        // CommonLibSSE-NG BGSSaveLoadManager::Load_Impl, AE Address Library ID.
        POINTER_SKYRIMSE(
            TCampaignGateLoadGame,
            loadGame,
            35728);
        POINTER_SKYRIMSE(
            BGSSaveLoadManager*,
            saveLoadManager,
            403340);
        s_realLoadGame = loadGame.Get();
        s_ppSaveLoadManager = saveLoadManager.Get();
        TP_HOOK(&s_realLoadGame, HookCampaignGateLoadGame);
    });
