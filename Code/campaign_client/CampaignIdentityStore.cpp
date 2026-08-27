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
constexpr char kCheckpointArtifactHeader[] =
    "stre-campaign-checkpoint-artifact-v1";
constexpr char kCampaignSaveMarkerHeader[] = "stre-campaign-save-v1";
constexpr std::uintmax_t kMaximumPlayerIdentityFileSize = 256;
constexpr std::uintmax_t kMaximumBindingCacheFileSize = 64 * 1024;
constexpr std::uintmax_t kMaximumCheckpointArtifactFileSize = 2048;
constexpr std::uintmax_t kMaximumCampaignSaveMarkerFileSize = 1024;
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

std::string BytesToHex(std::span<const std::uint8_t> acBytes)
{
    static constexpr std::array<char, 16> cHex{
        '0', '1', '2', '3', '4', '5', '6', '7',
        '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};
    std::string result;
    result.reserve(acBytes.size() * 2);
    for (std::uint8_t byte : acBytes)
    {
        result.push_back(cHex[byte >> 4]);
        result.push_back(cHex[byte & 0x0F]);
    }
    return result;
}

bool HexToBytes(
    std::string_view acValue,
    std::size_t aMaximumBytes,
    std::vector<std::uint8_t>& aBytes) noexcept
{
    aBytes.clear();
    if (acValue.size() % 2 != 0 ||
        acValue.size() > aMaximumBytes * 2 ||
        !std::all_of(acValue.begin(), acValue.end(), IsHexLower))
    {
        return false;
    }
    auto nibble = [](char aValue) -> std::uint8_t
    {
        return static_cast<std::uint8_t>(
            aValue <= '9' ? aValue - '0' : aValue - 'a' + 10);
    };
    aBytes.reserve(acValue.size() / 2);
    for (std::size_t index = 0; index < acValue.size(); index += 2)
    {
        aBytes.push_back(static_cast<std::uint8_t>(
            (nibble(acValue[index]) << 4) | nibble(acValue[index + 1])));
    }
    return true;
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

LocalStoreValueResult<std::vector<CampaignBindingCacheEntry>>
CampaignIdentityStore::ListBindings() noexcept
{
    LocalStoreValueResult<std::vector<CampaignBindingCacheEntry>> result;
    try
    {
        auto bindings = LoadBindings();
        result.Error = bindings.Error;
        result.Message = std::move(bindings.Message);
        if (!bindings)
            return result;

        result.Value.reserve(bindings.Value.size());
        for (auto& binding : bindings.Value)
            result.Value.push_back(std::move(binding.second));
        std::sort(
            result.Value.begin(), result.Value.end(),
            [](const CampaignBindingCacheEntry& acLeft,
               const CampaignBindingCacheEntry& acRight)
            {
                return acLeft.CampaignId < acRight.CampaignId;
            });
    }
    catch (...)
    {
        result.Error = LocalIdentityError::IoFailure;
        result.Message = "STRE campaign binding enumeration failed safely";
    }
    return result;
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


LocalStoreValueResult<std::optional<NativeSaveBundleArtifact>>
CampaignIdentityStore::LoadCheckpointArtifact(
    const std::string& acCampaignId,
    const std::string& acCheckpointId) noexcept
{
    LocalStoreValueResult<std::optional<NativeSaveBundleArtifact>> result;
    try
    {
        if (!IsValidCacheId(acCampaignId) ||
            !IsValidCacheId(acCheckpointId))
        {
            result.Error = LocalIdentityError::Malformed;
            result.Message = "invalid local checkpoint artifact identity";
            return result;
        }
        const std::string key = acCampaignId + "\n" + acCheckpointId;
        NativeSaveSha256 keyHash{};
        if (!ComputeNativeSaveSha256(
                std::span<const std::uint8_t>(
                    reinterpret_cast<const std::uint8_t*>(key.data()),
                    key.size()),
                keyHash))
        {
            result.Error = LocalIdentityError::IoFailure;
            result.Message = "failed to derive local checkpoint artifact key";
            return result;
        }
        const std::filesystem::path path = m_directory /
            ("stre-checkpoint-" + NativeSaveSha256ToHex(keyHash) + ".txt");
        std::error_code error;
        if (!std::filesystem::exists(path, error))
        {
            if (error)
            {
                result.Error = LocalIdentityError::IoFailure;
                result.Message = "failed to inspect local checkpoint artifact";
            }
            return result;
        }
        const auto size = std::filesystem::file_size(path, error);
        if (error)
        {
            result.Error = LocalIdentityError::IoFailure;
            result.Message = "failed to inspect local checkpoint artifact size";
            return result;
        }
        if (size > kMaximumCheckpointArtifactFileSize)
        {
            result.Error = LocalIdentityError::Malformed;
            result.Message =
                "existing local checkpoint artifact is oversized and was preserved";
            return result;
        }

        std::ifstream stream(path, std::ios::binary);
        std::string header;
        std::string campaign;
        std::string checkpoint;
        std::string identity;
        std::string fingerprintHex;
        std::string metadataHex;
        std::string trailing;
        if (!stream || !std::getline(stream, header) ||
            !std::getline(stream, campaign) ||
            !std::getline(stream, checkpoint) ||
            !std::getline(stream, identity) ||
            !std::getline(stream, fingerprintHex) ||
            !std::getline(stream, metadataHex) ||
            std::getline(stream, trailing) || stream.bad() ||
            header != kCheckpointArtifactHeader ||
            campaign != acCampaignId || checkpoint != acCheckpointId ||
            identity != "stre-" + acCheckpointId)
        {
            result.Error = LocalIdentityError::Malformed;
            result.Message =
                "existing local checkpoint artifact is malformed or unsupported and was preserved";
            return result;
        }
        std::vector<std::uint8_t> fingerprint;
        std::vector<std::uint8_t> metadata;
        if (!HexToBytes(
                fingerprintHex, kNativeSaveSha256Size, fingerprint) ||
            fingerprint.size() != kNativeSaveSha256Size ||
            !HexToBytes(
                metadataHex, kMaximumNativeSaveMetadataSize, metadata))
        {
            result.Error = LocalIdentityError::Malformed;
            result.Message =
                "existing local checkpoint artifact contains invalid hex data and was preserved";
            return result;
        }
        NativeSaveBundleArtifactParseResult parsed =
            ParseNativeSaveBundleArtifact(identity, fingerprint, metadata);
        if (!parsed)
        {
            result.Error = LocalIdentityError::Malformed;
            result.Message =
                "existing local checkpoint artifact failed canonical validation and was preserved";
            return result;
        }
        result.Value = std::move(parsed.Value);
    }
    catch (...)
    {
        result.Error = LocalIdentityError::IoFailure;
        result.Message = "local checkpoint artifact load failed safely";
    }
    return result;
}

LocalStoreResult CampaignIdentityStore::SaveCheckpointArtifact(
    const std::string& acCampaignId,
    const std::string& acCheckpointId,
    const NativeSaveBundleArtifact& acArtifact) noexcept
{
    try
    {
        const NativeSaveBundleArtifactParseResult parsed =
            ParseNativeSaveBundleArtifact(
                acArtifact.Bundle.LogicalIdentity,
                acArtifact.Fingerprint,
                acArtifact.Metadata);
        if (!IsValidCacheId(acCampaignId) ||
            !IsValidCacheId(acCheckpointId) ||
            acArtifact.Bundle.LogicalIdentity != "stre-" + acCheckpointId ||
            !parsed || parsed.Value != acArtifact)
        {
            return Failure(
                LocalIdentityError::Malformed,
                "refused to persist an invalid local checkpoint artifact");
        }
        auto existing = LoadCheckpointArtifact(acCampaignId, acCheckpointId);
        if (!existing)
            return {existing.Error, std::move(existing.Message)};
        if (existing.Value)
        {
            if (*existing.Value == acArtifact)
                return {};
            return Failure(
                LocalIdentityError::Malformed,
                "existing local checkpoint artifact conflicts and was preserved");
        }

        const std::string key = acCampaignId + "\n" + acCheckpointId;
        NativeSaveSha256 keyHash{};
        if (!ComputeNativeSaveSha256(
                std::span<const std::uint8_t>(
                    reinterpret_cast<const std::uint8_t*>(key.data()),
                    key.size()),
                keyHash))
        {
            return Failure(
                LocalIdentityError::IoFailure,
                "failed to derive local checkpoint artifact key");
        }
        std::ostringstream contents;
        contents << kCheckpointArtifactHeader << '\n'
                 << acCampaignId << '\n'
                 << acCheckpointId << '\n'
                 << acArtifact.Bundle.LogicalIdentity << '\n'
                 << NativeSaveSha256ToHex(acArtifact.Fingerprint) << '\n'
                 << BytesToHex(acArtifact.Metadata) << '\n';
        return WriteAtomically(
            m_directory /
                ("stre-checkpoint-" + NativeSaveSha256ToHex(keyHash) +
                 ".txt"),
            contents.str());
    }
    catch (...)
    {
        return Failure(
            LocalIdentityError::IoFailure,
            "local checkpoint artifact persistence failed safely");
    }
}

LocalStoreValueResult<std::filesystem::path>
CampaignIdentityStore::CampaignSaveMarkerPath(
    const std::string& acNativeSaveIdentity) const noexcept
{
    LocalStoreValueResult<std::filesystem::path> result;
    try
    {
        if (!IsValidCacheId(acNativeSaveIdentity))
        {
            result.Error = LocalIdentityError::Malformed;
            result.Message = "invalid native save identity for an STRE marker";
            return result;
        }
        NativeSaveSha256 identityHash{};
        if (!ComputeNativeSaveSha256(
                std::span<const std::uint8_t>(
                    reinterpret_cast<const std::uint8_t*>(
                        acNativeSaveIdentity.data()),
                    acNativeSaveIdentity.size()),
                identityHash))
        {
            result.Error = LocalIdentityError::IoFailure;
            result.Message = "failed to derive the STRE save marker key";
            return result;
        }
        result.Value = m_directory /
            ("stre-save-" + NativeSaveSha256ToHex(identityHash) + ".txt");
    }
    catch (...)
    {
        result.Error = LocalIdentityError::IoFailure;
        result.Message = "STRE save marker path derivation failed safely";
    }
    return result;
}

LocalStoreValueResult<std::optional<CampaignSaveMarker>>
CampaignIdentityStore::LoadCampaignSaveMarker(
    const std::string& acNativeSaveIdentity) noexcept
{
    LocalStoreValueResult<std::optional<CampaignSaveMarker>> result;
    try
    {
        auto markerPath = CampaignSaveMarkerPath(acNativeSaveIdentity);
        if (!markerPath)
        {
            result.Error = markerPath.Error;
            result.Message = std::move(markerPath.Message);
            return result;
        }
        std::error_code error;
        if (!std::filesystem::exists(markerPath.Value, error))
        {
            if (error)
            {
                result.Error = LocalIdentityError::IoFailure;
                result.Message = "failed to inspect the STRE save marker";
            }
            return result;
        }
        const auto size = std::filesystem::file_size(markerPath.Value, error);
        if (error || size > kMaximumCampaignSaveMarkerFileSize)
        {
            result.Error = error ? LocalIdentityError::IoFailure
                                 : LocalIdentityError::Malformed;
            result.Message = error
                ? "failed to inspect the STRE save marker size"
                : "the STRE save marker is oversized";
            return result;
        }

        std::ifstream stream(markerPath.Value, std::ios::binary);
        std::string header;
        CampaignSaveMarker marker;
        std::string trailing;
        if (!stream || !std::getline(stream, header) ||
            !std::getline(stream, marker.CampaignId) ||
            !std::getline(stream, marker.CampaignSlotId) ||
            !std::getline(stream, marker.CharacterBindingId) ||
            !std::getline(stream, marker.CheckpointId) ||
            !std::getline(stream, marker.NativeSaveIdentity) ||
            std::getline(stream, trailing) || stream.bad() ||
            header != kCampaignSaveMarkerHeader ||
            marker.NativeSaveIdentity != acNativeSaveIdentity ||
            marker.NativeSaveIdentity != "stre-" + marker.CheckpointId ||
            !IsValidCacheId(marker.CampaignId) ||
            !IsValidCacheId(marker.CampaignSlotId) ||
            !IsValidCacheId(marker.CharacterBindingId) ||
            !IsValidCacheId(marker.CheckpointId))
        {
            result.Error = LocalIdentityError::Malformed;
            result.Message =
                "the STRE save marker is malformed, unsupported, or mismatched";
            return result;
        }
        result.Value = std::move(marker);
    }
    catch (...)
    {
        result.Error = LocalIdentityError::IoFailure;
        result.Message = "STRE save marker load failed safely";
    }
    return result;
}

LocalStoreResult CampaignIdentityStore::SaveCampaignSaveMarker(
    const CampaignSaveMarker& acMarker) noexcept
{
    try
    {
        if (!IsValidCacheId(acMarker.CampaignId) ||
            !IsValidCacheId(acMarker.CampaignSlotId) ||
            !IsValidCacheId(acMarker.CharacterBindingId) ||
            !IsValidCacheId(acMarker.CheckpointId) ||
            !IsValidCacheId(acMarker.NativeSaveIdentity) ||
            acMarker.NativeSaveIdentity != "stre-" + acMarker.CheckpointId)
        {
            return Failure(LocalIdentityError::Malformed,
                "refused to persist an invalid STRE save marker");
        }
        auto markerPath = CampaignSaveMarkerPath(acMarker.NativeSaveIdentity);
        if (!markerPath)
            return {markerPath.Error, std::move(markerPath.Message)};
        std::ostringstream contents;
        contents << kCampaignSaveMarkerHeader << '\n'
                 << acMarker.CampaignId << '\n'
                 << acMarker.CampaignSlotId << '\n'
                 << acMarker.CharacterBindingId << '\n'
                 << acMarker.CheckpointId << '\n'
                 << acMarker.NativeSaveIdentity << '\n';
        return WriteAtomically(markerPath.Value, contents.str());
    }
    catch (...)
    {
        return Failure(LocalIdentityError::IoFailure,
            "STRE save marker serialization failed safely");
    }
}
}
