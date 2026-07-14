#include <TiltedOnlinePCH.h>

#include <Services/UiSurfaceService.h>

#include <Services/OverlayService.h>

#include <DInputHook.hpp>
#include <OverlayApp.hpp>

namespace
{
const char* SurfaceName(UiSurface aSurface) noexcept
{
    switch (aSurface)
    {
    case UiSurface::SkyrimTogether:
        return "str";
    case UiSurface::Trade:
        return "trade";
    case UiSurface::None:
    default:
        return "none";
    }
}
}

UiSurfaceService::UiSurfaceService(
    OverlayService& aOverlayService) noexcept
    : m_overlayService(aOverlayService)
{
}

void UiSurfaceService::SetSurface(UiSurface aSurface) noexcept
{
    if (aSurface != UiSurface::None &&
        !m_overlayService.GetInGame())
    {
        return;
    }

    if (m_surface == aSurface)
    {
        RefreshInputCapture();
        return;
    }

    m_surface = aSurface;
    const bool interactive = IsInteractive();

    m_overlayService.SetActive(interactive);

    if (interactive)
    {
        m_overlayService.SetVersion(BUILD_COMMIT);

        if (auto* const pApp = m_overlayService.GetOverlayApp())
            pApp->ExecuteAsync("enterGame");
    }

    PublishSurface();
    ApplyInputCapture(interactive);
}

void UiSurfaceService::ToggleSkyrimTogether() noexcept
{
    if (m_surface == UiSurface::Trade)
        return;

    SetSurface(
        m_surface == UiSurface::SkyrimTogether
            ? UiSurface::None
            : UiSurface::SkyrimTogether);
}

void UiSurfaceService::RefreshInputCapture() noexcept
{
    const bool interactive = IsInteractive();

    m_overlayService.SetActive(interactive);
    ApplyInputCapture(interactive);
    PublishSurface();
}

void UiSurfaceService::ApplyInputCapture(bool aInteractive) noexcept
{
    TiltedPhoques::DInputHook::Get().SetEnabled(aInteractive);

    const auto pApp = m_overlayService.GetOverlayApp();
    if (pApp)
    {
        const auto pClient = pApp->GetClient();
        if (pClient)
        {
            const auto pRenderer = pClient->GetOverlayRenderHandler();
            if (pRenderer)
                pRenderer->SetCursorVisible(aInteractive);
        }
    }

    while (ShowCursor(FALSE) >= 0)
        ;
}

void UiSurfaceService::PublishSurface() noexcept
{
    const auto pApp = m_overlayService.GetOverlayApp();
    if (!pApp)
        return;

    auto arguments = CefListValue::Create();
    arguments->SetString(0, SurfaceName(m_surface));
    pApp->ExecuteAsync("uiSurface", arguments);
}
