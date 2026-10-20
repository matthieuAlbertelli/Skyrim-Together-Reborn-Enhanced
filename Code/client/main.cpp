
#include <TiltedOnlineApp.h>
#include <TiltedOnlinePCH.h>
#include <ScriptExtender.h>
#include <Runtime/Compatibility.h>

#include <Commctrl.h>
#include <Windows.h>

#include <base/dialogues/win/TaskDialog.h>

std::unique_ptr<TiltedOnlineApp> g_appInstance{nullptr};

extern HICON g_SharedWindowIcon;

static void ShowAddressLibraryError(const wchar_t* apGamePath)
{
    const auto& error = VersionDb::Get().GetError();
    auto errorDetail = fmt::format(L"{}\\Data\\SKSE\\Plugins\n{}", apGamePath, std::wstring(error.begin(), error.end()));

    Base::TaskDialog dia(g_SharedWindowIcon, L"Error", L"Missing or invalid Skyrim Address Library",
                        L"Requires Address Library v13: versionlib-1-7-104-0.bin for Steam 1.7.104.0.", errorDetail.c_str());

    dia.AppendButton(0xBEED, L"Visit troubleshooting page on wiki.tiltedphoques.com");
    dia.AppendButton(0xBEEF, L"Visit Address Library modpage on nexusmods.com");
    const int result = dia.Show();
    if (result == 0xBEEF)
    {
        ShellExecuteW(nullptr, L"open", LR"(https://www.nexusmods.com/skyrimspecialedition/mods/32444?tab=files)", nullptr, nullptr, SW_SHOWNORMAL);
    }
    else if (result == 0xBEED)
    {
        ShellExecuteW(nullptr, L"open", LR"(https://wiki.tiltedphoques.com/tilted-online/guides/troubleshooting/address-library-error)", nullptr, nullptr, SW_SHOWNORMAL);
    }

    exit(4);
}

bool RunTiltedPreflight(const std::filesystem::path& aGamePath, const String& aExeVersion, uint32_t aImageSize)
{
    if (!VersionDb::Get().Load(aGamePath, aExeVersion.c_str(), aImageSize))
    {
        ShowAddressLibraryError(aGamePath.c_str());
    }
    std::string error;
    if (!ValidateScriptExtender(aGamePath, error))
    {
        const std::wstring detail(error.begin(), error.end());
        Base::TaskDialog(g_SharedWindowIcon, L"Error", L"Missing or incompatible dependency",
                         L"Steam Skyrim 1.7.104.0 requires SKSE64 2.3.1.", detail.c_str()).Show();
        return false;
    }
    if (!STRE::Runtime::CampaignAbiVerified)
    {
        const auto reason = STRE::Runtime::CampaignAbiUnavailable;
        const std::wstring detail(reason.begin(), reason.end());
        Base::TaskDialog(g_SharedWindowIcon, L"Experimental port", L"STRE native capability unavailable",
                         detail.c_str(), nullptr).Show();
        return false;
    }
    return true;
}

void RunTiltedInit(const std::filesystem::path& acGamePath, const String& aExeVersion)
{
    // Defense in depth; the launcher already performed read-only preflight.
    if (aExeVersion != STRE::Runtime::TargetVersionString || !STRE::Runtime::CampaignAbiVerified ||
        VersionDb::Get().GetLoadedVersionString() != aExeVersion.c_str())
        exit(4);

    g_appInstance = std::make_unique<TiltedOnlineApp>();

    TiltedOnlineApp::InstallHooks2();
    TP_HOOK_COMMIT;

    // PR #885: install SKSE startup hooks before the game's CRT/WinMain, after
    // STRE hooks. DLL load is not plugin initialization; it continues in startup.
    if (!LoadScriptExtender(acGamePath))
        exit(5);
}

void RunTiltedApp()
{
    g_appInstance->BeginMain();
}
