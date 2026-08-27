#pragma once

#include <Structs/NativeSaveBundle.h>

#include <filesystem>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace STRE::Campaign
{
enum class LocalIdentityError
{
    None,
    PathUnavailable,
    Malformed,
    IoFailure
};

struct LocalStoreResult
{
    LocalIdentityError Error{LocalIdentityError::None};
    std::string Message;

    [[nodiscard]] bool Succeeded() const noexcept
    {
        return Error == LocalIdentityError::None;
    }

    explicit operator bool() const noexcept { return Succeeded(); }
};

template <class T> struct LocalStoreValueResult : LocalStoreResult
{
    T Value{};
};

struct CampaignBindingCacheEntry
{
    std::string CampaignId;
    std::string CampaignSlotId;
    std::string CharacterBindingId;

    bool operator==(const CampaignBindingCacheEntry&) const noexcept = default;
};

struct CampaignSaveMarker
{
    std::string CampaignId;
    std::string CampaignSlotId;
    std::string CharacterBindingId;
    std::string CheckpointId;
    std::string NativeSaveIdentity;

    bool operator==(const CampaignSaveMarker&) const noexcept = default;
};

class CampaignIdentityStore final
{
public:
    explicit CampaignIdentityStore(std::filesystem::path aDirectory) noexcept;

    [[nodiscard]] LocalStoreValueResult<std::string> LoadOrCreatePlayerId() noexcept;
    [[nodiscard]] LocalStoreValueResult<std::optional<CampaignBindingCacheEntry>>
    LoadBinding(const std::string& acCampaignId) noexcept;
    [[nodiscard]] LocalStoreValueResult<std::vector<CampaignBindingCacheEntry>>
    ListBindings() noexcept;
    [[nodiscard]] LocalStoreResult SaveBinding(
        const CampaignBindingCacheEntry& acBinding) noexcept;
    [[nodiscard]] LocalStoreResult RemoveBinding(
        const std::string& acCampaignId) noexcept;
    [[nodiscard]] LocalStoreValueResult<
        std::optional<NativeSaveBundleArtifact>>
    LoadCheckpointArtifact(
        const std::string& acCampaignId,
        const std::string& acCheckpointId) noexcept;
    [[nodiscard]] LocalStoreResult SaveCheckpointArtifact(
        const std::string& acCampaignId,
        const std::string& acCheckpointId,
        const NativeSaveBundleArtifact& acArtifact) noexcept;
    [[nodiscard]] LocalStoreValueResult<std::optional<CampaignSaveMarker>>
    LoadCampaignSaveMarker(
        const std::string& acNativeSaveIdentity) noexcept;
    [[nodiscard]] LocalStoreResult SaveCampaignSaveMarker(
        const CampaignSaveMarker& acMarker) noexcept;

    [[nodiscard]] static LocalStoreValueResult<std::filesystem::path>
    ResolveDefaultDirectory() noexcept;
    [[nodiscard]] static std::string GenerateOpaqueId(std::size_t aByteCount = 32);
    [[nodiscard]] static bool IsValidPlayerId(const std::string& acValue) noexcept;
    [[nodiscard]] static bool IsValidCacheId(const std::string& acValue) noexcept;

private:
    using BindingMap = std::unordered_map<std::string, CampaignBindingCacheEntry>;

    [[nodiscard]] LocalStoreValueResult<BindingMap> LoadBindings() noexcept;
    [[nodiscard]] LocalStoreResult SaveBindings(const BindingMap& acBindings) noexcept;
    [[nodiscard]] LocalStoreResult WriteAtomically(
        const std::filesystem::path& acTarget,
        const std::string& acContents) noexcept;
    [[nodiscard]] LocalStoreValueResult<std::filesystem::path>
    CampaignSaveMarkerPath(
        const std::string& acNativeSaveIdentity) const noexcept;

    std::filesystem::path m_directory;
};
}
