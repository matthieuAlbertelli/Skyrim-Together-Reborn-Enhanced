#pragma once

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
    [[nodiscard]] static CampaignNativeLoadInvokeResult InvokeValidated(
        std::string_view acNativeSaveIdentity) noexcept;
};
