#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

enum class CampaignNativeLoadInvokeResult
{
    BoundaryUnavailable,
    NativeRejected,
    NativeAccepted
};

// Owns only the exact local BGSSaveLoadManager native invocation. Artifact
// validation, campaign authority, and recovery policy remain outside this API.
class CampaignNativeLoad final
{
public:
    // Returns an owned copy of the native save-list target selected by the
    // Journal. This is observation only; it grants no load authority.
    [[nodiscard]] static std::optional<std::string> InspectSaveListTarget(
        std::uint32_t aSelectionIndex) noexcept;

    [[nodiscard]] static CampaignNativeLoadInvokeResult InvokeValidated(
        std::string_view acNativeSaveIdentity) noexcept;
};
