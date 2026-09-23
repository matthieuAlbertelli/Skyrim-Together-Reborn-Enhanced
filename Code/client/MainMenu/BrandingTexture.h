#pragma once

#include <d3d11.h>
#include <filesystem>
#include <wrl/client.h>

namespace STRE::MainMenu
{
// Optional PNG, decoded once on the render thread using Windows WIC. Straight
// RGBA matches the existing ImGui backend's non-premultiplied blend state.
struct BrandingTexture
{
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> View;
    unsigned Width{}, Height{};
    // Keep the full canvas for feathered backdrops; trim export margins for logos.
    [[nodiscard]] HRESULT Load(ID3D11Device* apDevice, const std::filesystem::path& aPath, bool aTrimMargins = true);
    [[nodiscard]] float Aspect() const noexcept { return Height ? static_cast<float>(Width) / Height : 0.0f; }
};
} // namespace STRE::MainMenu
