#include <TiltedOnlinePCH.h>
#include <ScriptExtender.h>
#include <Runtime/Compatibility.h>

namespace
{
HMODULE g_SKSEModuleHandle{nullptr};
bool g_startupHooksInstalled{};

bool ReadFileVersion(const std::filesystem::path& aPath, STRE::Runtime::Version& aVersion)
{
    DWORD ignored{};
    const auto size = GetFileVersionInfoSizeW(aPath.c_str(), &ignored);
    if (size == 0 || size > 1024 * 1024)
        return false;
    std::vector<char> buffer(size);
    if (!GetFileVersionInfoW(aPath.c_str(), 0, size, buffer.data()))
        return false;
    VS_FIXEDFILEINFO* info{};
    UINT length{};
    if (!VerQueryValueW(buffer.data(), L"\\", reinterpret_cast<void**>(&info), &length) ||
        !info || length < sizeof(*info) || info->dwSignature != 0xFEEF04BD)
        return false;
    aVersion = {info->dwFileVersionMS >> 16, info->dwFileVersionMS & 0xFFFF,
                info->dwFileVersionLS >> 16, info->dwFileVersionLS & 0xFFFF};
    return true;
}
} // namespace

bool IsScriptExtenderLoaded()
{
    // Existing AuthenticationRequest field means DLL presence. Plugin admission
    // still enumerates loaded modules separately; no ready signal is fabricated.
    return g_SKSEModuleHandle != nullptr;
}

bool ValidateScriptExtender(const std::filesystem::path& aGamePath, std::string& aError)
{
    aError.clear();
    const auto path = aGamePath / STRE::Runtime::RequiredSkseDll;
    STRE::Runtime::Version version{};
    if (!ReadFileVersion(path, version))
    {
        aError = "Missing/unreadable SKSE dependency: " + path.string();
        return false;
    }
    if (version != STRE::Runtime::RequiredSkseFileVersion)
    {
        aError = fmt::format("Incompatible SKSE dependency: {} has file version {}.{}.{}.{}; expected 0.2.3.1.",
                            path.string(), version[0], version[1], version[2], version[3]);
        return false;
    }
    return true;
}

bool LoadScriptExtender(const std::filesystem::path& aGamePath)
{
    if (g_startupHooksInstalled)
        return true;
    std::string error;
    if (!ValidateScriptExtender(aGamePath, error))
    {
        spdlog::error("[STRE][Dependency] {}", error);
        return false;
    }
    const auto path = aGamePath / STRE::Runtime::RequiredSkseDll;
    g_SKSEModuleHandle = LoadLibraryW(path.c_str());
    if (!g_SKSEModuleHandle)
    {
        spdlog::error("[STRE][Dependency] SKSE DLL load failed path={} win32Error={}", path.string(), GetLastError());
        return false;
    }
    const auto start = reinterpret_cast<void (*)()>(GetProcAddress(g_SKSEModuleHandle, "StartSKSE"));
    if (!start)
    {
        spdlog::error("[STRE][Dependency] SKSE DLL loaded but StartSKSE export is missing");
        FreeLibrary(g_SKSEModuleHandle);
        g_SKSEModuleHandle = nullptr;
        return false;
    }
    // TiltedEvolution PR #885 / 6f963497: install before entering Skyrim CRT.
    spdlog::info("SKSE 2.3.1 DLL loaded; installing startup hooks");
    start();
    g_startupHooksInstalled = true;
    spdlog::info("SKSE startup hooks installed; plugin initialization continues during game startup");
    return true;
}
