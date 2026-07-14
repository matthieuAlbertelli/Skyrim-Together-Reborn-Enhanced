#pragma once

#include <cstdint>

struct OverlayService;

enum class UiSurface : std::uint8_t
{
    None,
    SkyrimTogether,
    Trade
};

/**
 * @brief Owns the single CEF overlay surface and arbitrates which UI is visible.
 */
struct UiSurfaceService
{
    explicit UiSurfaceService(OverlayService& aOverlayService) noexcept;
    ~UiSurfaceService() noexcept = default;

    TP_NOCOPYMOVE(UiSurfaceService);

    void SetSurface(UiSurface aSurface) noexcept;
    void ToggleSkyrimTogether() noexcept;
    void RefreshInputCapture() noexcept;

    [[nodiscard]] UiSurface GetSurface() const noexcept
    {
        return m_surface;
    }

    [[nodiscard]] bool IsInteractive() const noexcept
    {
        return m_surface != UiSurface::None;
    }

    [[nodiscard]] OverlayService& GetOverlayService() noexcept
    {
        return m_overlayService;
    }

private:
    void ApplyInputCapture(bool aInteractive) noexcept;
    void PublishSurface() noexcept;

    OverlayService& m_overlayService;
    UiSurface m_surface{UiSurface::None};
};
