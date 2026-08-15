#include <CampaignIdentityStore.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <random>
#include <sstream>
#include <system_error>
#include <vector>

#if defined(_WIN32)
#include <Windows.h>
#endif

namespace STRE::Campaign
{
namespace
{
constexpr char kPlayerIdentityHeader[] = "stre-player-id-v1";
constexpr char kBindingCacheHeader[] = "stre-campaign-bindings-v1";
constexpr char kPlayerIdentityFilename[] = "stre-player-id-v1.txt";
constexpr char kBindingCacheFilename[] = "stre-campaign-bindings-v1.txt";
constexpr std::uintmax_t kMaximumPlayerIdentityFileSize = 256;
constexpr std::uintmax_t kMaximumBindingCacheFileSize = 64 * 1024;
constexpr std::size_t kMaximumBindingCacheEntries = 256;

LocalStoreResult Failure(LocalIdentityError aError, std::string aMessage)
{
    return {aError, std::move(aMessage)};
}

std::optional<std::string> ReadEnvironment(const char* apName)
{
#if defined(_WIN32)
    char* pValue = nullptr;
    std::size_t length = 0;
    if (_dupenv_s(&pValue, &length, apName) != 0 || !pValue || length <= 1)
    {
        std::free(pValue);
        return std::nullopt;
    }
    std::string result(pValue);
    std::free(pValue);
    return result;
#else
    const char* const pValue = std::getenv(apName);
    if (!pValue || *pValue == '\0')
        return std::nullopt;
    return std::string(pValue);
#endif
}

bool IsHexLower(char aValue) noexcept
{
    return (aValue >= '0' && aValue <= '9') ||
        (aValue >= 'a' && aValue <= 'f');
}
}

CampaignIdentityStore::CampaignIdentityStore(
    std::filesystem::path aDirectory) noexcept
    : m_directory(std::move(aDirectory))
{
}

std::string CampaignIdentityStore::GenerateOpaqueId(std::size_t aByteCount)
{
    static constexpr std::array<char, 16> cHex{
        '0', '1', '2', '3', '4', '5', '6', '7',
        '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};

    std::random_device random;
    std::string result;
    result.reserve(aByteCount * 2);
    for (std::size_t index = 0; index < aByteCount; ++index)
    {
        const auto value = static_cast<unsigned char>(random());
        result.push_back(cHex[value >> 4]);
        result.push_back(cHex[value & 0x0F]);
    }
    return result;
}

bool CampaignIdentityStore::IsValidPlayerId(
    const std::string& acValue) noexcept
{
    return acValue.size() == 64 &&
        std::all_of(acValue.begin(), acValue.end(), IsHexLower);
}

bool CampaignIdentityStore::IsValidCacheId(
    const std::string& acValue) noexcept
{
    if (acValue.empty() || acValue.size() > 128)
        return false;
    return std::all_of(
        acValue.begin(), acValue.end(), [](char aValue)
        {
            return (aValue >= 'a' && aValue <= 'z') ||
                (aValue >= 'A' && aValue <= 'Z') ||
                (aValue >= '0' && aValue <= '9') ||
                aValue == '-' || aValue == '_';
        });
}

LocalStoreValueResult<std::filesystem::path>
CampaignIdentityStore::ResolveDefaultDirectory() noexcept
{
    LocalStoreValueResult<std::filesystem::path> result;
    try
    {
#if defined(_WIN32)
        const auto base = ReadEnvironment("LOCALAPPDATA");
        if (!base)
        {
            result.Error = LocalIdentityError::PathUnavailable;
            result.Message = "LOCALAPPDATA is unavailable; STRE cannot locate its identity store";
            return result;
        }
        result.Value = std::filesystem::path(*base) /
            "SkyrimTogetherRebornEnhanced";
#else
        auto base = ReadEnvironment("XDG_CONFIG_HOME");
        if (base)
        {
            result.Value = std::filesystem::path(*base) /
                "SkyrimTogetherRebornEnhanced";
        }
        else
        {
            base = ReadEnvironment("HOME");
            if (!base)
            {
                result.Error = LocalIdentityError::PathUnavailable;
                result.Message = "no user configuration directory is available for the STRE identity store";
                return result;
            }
            result.Value = std::filesystem::path(*base) / ".config" /
                "SkyrimTogetherRebornEnhanced";
        }
#endif
    }
    catch (...)
    {
        result.Error = LocalIdentityError::PathUnavailable;
        result.Message = "failed to resolve the STRE identity store directory";
    }
    return result;
}

LocalStoreResult CampaignIdentityStore::WriteAtomically(
    const std::filesystem::path& acTarget,
    const std::string& acContents) noexcept
{
    try
    {
        std::error_code error;
        std::filesystem::create_directories(m_directory, error);
        if (error)
        {
            return Failure(
                LocalIdentityError::IoFailure,
                "failed to create the STRE identity directory: " + error.message());
        }

        std::filesystem::path temporary = acTarget;
        temporary += ".tmp-" + GenerateOpaqueId(8);
        {
            std::ofstream stream(
                temporary, std::ios::binary | std::ios::trunc);
            if (!stream)
                return Failure(LocalIdentityError::IoFailure,
                    "failed to open a temporary STRE identity file");
            stream.write(acContents.data(),
                static_cast<std::streamsize>(acContents.size()));
            stream.flush();
            if (!stream)
            {
                stream.close();
                std::filesystem::remove(temporary, error);
                return Failure(LocalIdentityError::IoFailure,
                    "failed to write a temporary STRE identity file");
            }
        }

#if defined(_WIN32)
        if (!MoveFileExW(
                temporary.c_str(), acTarget.c_str(),
                MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
        {
            const DWORD code = GetLastError();
            std::filesystem::remove(temporary, error);
            return Failure(
                LocalIdentityError::IoFailure,
                "failed to atomically replace the STRE identity file (Windows error " +
                    std::to_string(code) + ")");
        }
#else
        std::filesystem::rename(temporary, acTarget, error);
        if (error)
        {
            std::filesystem::remove(temporary, error);
            return Failure(LocalIdentityError::IoFailure,
                "failed to atomically replace the STRE identity file");
        }
#endif
        return {};
    }
    catch (...)
    {
        return Failure(LocalIdentityError::IoFailure,
            "atomic STRE identity write failed safely");
    }
}

LocalStoreValueResult<std::string>
CampaignIdentityStore::LoadOrCreatePlayerId() noexcept
{
    LocalStoreValueResult<std::string> result;
    try
    {
        const std::filesystem::path path =
            m_directory / kPlayerIdentityFilename;
        std::error_code error;
        const bool exists = std::filesystem::exists(path, error);
        if (error)
        {
            result.Error = LocalIdentityError::IoFailure;
            result.Message = "failed to inspect the STRE player identity file";
            return result;
        }
        if (!exists)
        {
            result.Value = GenerateOpaqueId();
            const LocalStoreResult written = WriteAtomically(
                path, std::string(kPlayerIdentityHeader) + "\n" +
                    result.Value + "\n");
            if (!written)
            {
                result.Error = written.Error;
                result.Message = written.Message;
            }
            return result;
        }

        const std::uintmax_t fileSize = std::filesystem::file_size(path, error);
        if (error)
        {
            result.Error = LocalIdentityError::IoFailure;
            result.Message = "failed to inspect the STRE player identity file size";
            return result;
        }
        if (fileSize > kMaximumPlayerIdentityFileSize)
        {
            result.Error = LocalIdentityError::Malformed;
            result.Message = "the existing STRE player identity file is oversized; it was preserved and no replacement identity was generated";
            return result;
        }

        std::ifstream stream(path, std::ios::binary);
        std::string header;
        std::string playerId;
        std::string trailing;
        if (!stream || !std::getline(stream, header) ||
            !std::getline(stream, playerId) || std::getline(stream, trailing) ||
            stream.bad() || header != kPlayerIdentityHeader ||
            !IsValidPlayerId(playerId))
        {
            result.Error = LocalIdentityError::Malformed;
            result.Message = "the existing STRE player identity file is malformed; it was preserved and no replacement identity was generated";
            return result;
        }
        result.Value = std::move(playerId);
    }
    catch (...)
    {
        result.Error = LocalIdentityError::IoFailure;
        result.Message = "STRE player identity load failed safely";
    }
    return result;
}

LocalStoreValueResult<CampaignIdentityStore::BindingMap>
CampaignIdentityStore::LoadBindings() noexcept
{
    LocalStoreValueResult<BindingMap> result;
    try
    {
        const std::filesystem::path path = m_directory / kBindingCacheFilename;
        std::error_code error;
        if (!std::filesystem::exists(path, error))
        {
            if (error)
            {
                result.Error = LocalIdentityError::IoFailure;
                result.Message = "failed to inspect the STRE campaign binding cache";
            }
            return result;
        }

        const std::uintmax_t fileSize = std::filesystem::file_size(path, error);
        if (error)
        {
            result.Error = LocalIdentityError::IoFailure;
            result.Message = "failed to inspect the STRE campaign binding cache size";
            return result;
        }
        if (fileSize > kMaximumBindingCacheFileSize)
        {
            result.Error = LocalIdentityError::Malformed;
            result.Message = "the existing STRE campaign binding cache is oversized";
            return result;
        }

        std::ifstream stream(path, std::ios::binary);
        std::string line;
        if (!stream || !std::getline(stream, line) ||
            line != kBindingCacheHeader)
        {
            result.Error = LocalIdentityError::Malformed;
            result.Message = "the existing STRE campaign binding cache is malformed";
            return result;
        }

        while (std::getline(stream, line))
        {
            if (line.empty())
                continue;
            if (result.Value.size() >= kMaximumBindingCacheEntries)
            {
                result.Error = LocalIdentityError::Malformed;
                result.Message = "the existing STRE campaign binding cache contains too many records";
                return result;
            }
            const std::size_t first = line.find('\t');
            const std::size_t second = first == std::string::npos
                ? std::string::npos : line.find('\t', first + 1);
            if (first == std::string::npos || second == std::string::npos ||
                line.find('\t', second + 1) != std::string::npos)
            {
                result.Error = LocalIdentityError::Malformed;
                result.Message = "the existing STRE campaign binding cache contains a malformed record";
                return result;
            }
            CampaignBindingCacheEntry entry{
                line.substr(0, first),
                line.substr(first + 1, second - first - 1),
                line.substr(second + 1)};
            if (!IsValidCacheId(entry.CampaignId) ||
                !IsValidCacheId(entry.CampaignSlotId) ||
                !IsValidCacheId(entry.CharacterBindingId) ||
                result.Value.contains(entry.CampaignId))
            {
                result.Error = LocalIdentityError::Malformed;
                result.Message = "the existing STRE campaign binding cache contains invalid or duplicate identities";
                return result;
            }
            result.Value.emplace(entry.CampaignId, std::move(entry));
        }
        if (stream.bad())
        {
            result.Error = LocalIdentityError::IoFailure;
            result.Message = "failed while reading the STRE campaign binding cache";
            return result;
        }
    }
    catch (...)
    {
        result.Error = LocalIdentityError::IoFailure;
        result.Message = "STRE campaign binding cache load failed safely";
    }
    return result;
}

LocalStoreResult CampaignIdentityStore::SaveBindings(
    const BindingMap& acBindings) noexcept
{
    try
    {
        if (acBindings.size() > kMaximumBindingCacheEntries)
        {
            return Failure(LocalIdentityError::Malformed,
                "refused to persist too many STRE campaign bindings");
        }

        std::vector<std::string> keys;
        keys.reserve(acBindings.size());
        for (const auto& [key, unused] : acBindings)
        {
            (void)unused;
            keys.push_back(key);
        }
        std::sort(keys.begin(), keys.end());

        std::ostringstream contents;
        contents << kBindingCacheHeader << '\n';
        for (const std::string& key : keys)
        {
            const CampaignBindingCacheEntry& entry = acBindings.at(key);
            contents << entry.CampaignId << '\t' << entry.CampaignSlotId << '\t'
                     << entry.CharacterBindingId << '\n';
        }
        return WriteAtomically(
            m_directory / kBindingCacheFilename, contents.str());
    }
    catch (...)
    {
        return Failure(LocalIdentityError::IoFailure,
            "STRE campaign binding cache serialization failed safely");
    }
}

LocalStoreValueResult<std::optional<CampaignBindingCacheEntry>>
CampaignIdentityStore::LoadBinding(const std::string& acCampaignId) noexcept
{
    try
    {
        auto bindings = LoadBindings();
        LocalStoreValueResult<std::optional<CampaignBindingCacheEntry>> result;
        result.Error = bindings.Error;
        result.Message = std::move(bindings.Message);
        if (!bindings)
            return result;
        const auto it = bindings.Value.find(acCampaignId);
        if (it != bindings.Value.end())
            result.Value = it->second;
        return result;
    }
    catch (...)
    {
        LocalStoreValueResult<std::optional<CampaignBindingCacheEntry>> result;
        result.Error = LocalIdentityError::IoFailure;
        result.Message = "STRE campaign binding lookup failed safely";
        return result;
    }
}

LocalStoreResult CampaignIdentityStore::SaveBinding(
    const CampaignBindingCacheEntry& acBinding) noexcept
{
    try
    {
        if (!IsValidCacheId(acBinding.CampaignId) ||
            !IsValidCacheId(acBinding.CampaignSlotId) ||
            !IsValidCacheId(acBinding.CharacterBindingId))
        {
            return Failure(LocalIdentityError::Malformed,
                "refused to persist an invalid STRE campaign binding");
        }
        auto bindings = LoadBindings();
        if (!bindings)
            return {bindings.Error, std::move(bindings.Message)};
        bindings.Value[acBinding.CampaignId] = acBinding;
        return SaveBindings(bindings.Value);
    }
    catch (...)
    {
        return Failure(LocalIdentityError::IoFailure,
            "STRE campaign binding update failed safely");
    }
}

LocalStoreResult CampaignIdentityStore::RemoveBinding(
    const std::string& acCampaignId) noexcept
{
    try
    {
        auto bindings = LoadBindings();
        if (!bindings)
            return {bindings.Error, std::move(bindings.Message)};
        bindings.Value.erase(acCampaignId);
        return SaveBindings(bindings.Value);
    }
    catch (...)
    {
        return Failure(LocalIdentityError::IoFailure,
            "STRE campaign binding removal failed safely");
    }
}
}
