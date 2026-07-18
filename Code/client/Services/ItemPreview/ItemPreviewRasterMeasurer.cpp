#include <TiltedOnlinePCH.h>

#include <Services/ItemPreview/ItemPreviewRasterMeasurer.h>

#include <algorithm>
#include <cmath>

namespace
{
template <class T>
void SafeRelease(T*& apObject) noexcept
{
    if (!apObject)
        return;

    apObject->Release();
    apObject = nullptr;
}

std::uint32_t GetBytesPerPixel(DXGI_FORMAT aFormat) noexcept
{
    switch (aFormat)
    {
    case DXGI_FORMAT_R8G8B8A8_TYPELESS:
    case DXGI_FORMAT_R8G8B8A8_UNORM:
    case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
    case DXGI_FORMAT_R10G10B10A2_TYPELESS:
    case DXGI_FORMAT_R10G10B10A2_UNORM:
    case DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM:
    case DXGI_FORMAT_B8G8R8A8_UNORM:
    case DXGI_FORMAT_B8G8R8X8_UNORM:
    case DXGI_FORMAT_B8G8R8A8_TYPELESS:
    case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
    case DXGI_FORMAT_B8G8R8X8_TYPELESS:
    case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
        return 4;

    case DXGI_FORMAT_R16G16B16A16_TYPELESS:
    case DXGI_FORMAT_R16G16B16A16_FLOAT:
    case DXGI_FORMAT_R16G16B16A16_UNORM:
    case DXGI_FORMAT_R16G16B16A16_UINT:
    case DXGI_FORMAT_R16G16B16A16_SNORM:
    case DXGI_FORMAT_R16G16B16A16_SINT:
        return 8;

    case DXGI_FORMAT_R32G32B32A32_TYPELESS:
    case DXGI_FORMAT_R32G32B32A32_FLOAT:
    case DXGI_FORMAT_R32G32B32A32_UINT:
    case DXGI_FORMAT_R32G32B32A32_SINT:
        return 16;

    default:
        return 0;
    }
}


}

void ItemPreviewRasterMeasurer::ReleaseCapture(
    ItemPreviewRasterCapture& aCapture) noexcept
{
    SafeRelease(aCapture.after);
    SafeRelease(aCapture.before);
    SafeRelease(aCapture.source);
    aCapture = {};
}

bool ItemPreviewRasterMeasurer::BeginCapture(
    ID3D11Device* apDevice,
    ID3D11DeviceContext* apContext,
    const ItemPreviewRasterCaptureRequest& aTelemetry,
    ItemPreviewRasterCapture& aCapture) noexcept
{
    if (!apDevice || !apContext || !aTelemetry.valid ||
        !aTelemetry.captureRequested)
    {
        return false;
    }

    ID3D11RenderTargetView* pRenderTargetView = nullptr;
    apContext->OMGetRenderTargets(1, &pRenderTargetView, nullptr);
    if (!pRenderTargetView)
        return false;

    ID3D11Resource* pResource = nullptr;
    pRenderTargetView->GetResource(&pResource);
    SafeRelease(pRenderTargetView);
    if (!pResource)
        return false;

    ID3D11Texture2D* pSource = nullptr;
    const HRESULT textureResult = pResource->QueryInterface(
        __uuidof(ID3D11Texture2D),
        reinterpret_cast<void**>(&pSource));
    SafeRelease(pResource);
    if (FAILED(textureResult) || !pSource)
        return false;

    D3D11_TEXTURE2D_DESC sourceDescription{};
    pSource->GetDesc(&sourceDescription);
    const std::uint32_t bytesPerPixel =
        GetBytesPerPixel(sourceDescription.Format);
    if (sourceDescription.Width == 0 ||
        sourceDescription.Height == 0 ||
        sourceDescription.ArraySize != 1 ||
        sourceDescription.SampleDesc.Count != 1 ||
        bytesPerPixel == 0)
    {
        spdlog::warn(
            "Trade preview raster projection unavailable projectionProbe=true item={:016X} reason=unsupportedTarget target={}x{} format={} arraySize={} samples={} bpp={}",
            aTelemetry.selectedItem,
            sourceDescription.Width,
            sourceDescription.Height,
            static_cast<std::uint32_t>(sourceDescription.Format),
            sourceDescription.ArraySize,
            sourceDescription.SampleDesc.Count,
            bytesPerPixel);
        SafeRelease(pSource);
        return false;
    }

    D3D11_TEXTURE2D_DESC stagingDescription = sourceDescription;
    stagingDescription.MipLevels = 1;
    stagingDescription.ArraySize = 1;
    stagingDescription.SampleDesc.Count = 1;
    stagingDescription.SampleDesc.Quality = 0;
    stagingDescription.Usage = D3D11_USAGE_STAGING;
    stagingDescription.BindFlags = 0;
    stagingDescription.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    stagingDescription.MiscFlags = 0;

    ID3D11Texture2D* pBefore = nullptr;
    ID3D11Texture2D* pAfter = nullptr;
    const HRESULT beforeResult = apDevice->CreateTexture2D(
        &stagingDescription,
        nullptr,
        &pBefore);
    const HRESULT afterResult = apDevice->CreateTexture2D(
        &stagingDescription,
        nullptr,
        &pAfter);
    if (FAILED(beforeResult) || FAILED(afterResult) ||
        !pBefore || !pAfter)
    {
        spdlog::warn(
            "Trade preview raster projection unavailable projectionProbe=true item={:016X} reason=stagingCreation beforeResult={:08X} afterResult={:08X}",
            aTelemetry.selectedItem,
            static_cast<std::uint32_t>(beforeResult),
            static_cast<std::uint32_t>(afterResult));
        SafeRelease(pAfter);
        SafeRelease(pBefore);
        SafeRelease(pSource);
        return false;
    }

    apContext->CopyResource(pBefore, pSource);

    aCapture.source = pSource;
    aCapture.before = pBefore;
    aCapture.after = pAfter;
    aCapture.description = sourceDescription;
    aCapture.telemetry = aTelemetry;
    return true;
}

bool ItemPreviewRasterMeasurer::CompleteCapture(
    ID3D11DeviceContext* apContext,
    ItemPreviewRasterCapture& aCapture,
    ItemPreviewRasterMeasurement& aMeasurement) noexcept
{
    aMeasurement = {};
    aMeasurement.selectionRevision =
        aCapture.telemetry.selectionRevision;
    aMeasurement.regionRevision =
        aCapture.telemetry.regionRevision;
    aMeasurement.solverRevision =
        aCapture.telemetry.solverRevision;
    aMeasurement.selectedItem =
        aCapture.telemetry.selectedItem;
    if (!apContext || !aCapture.source ||
        !aCapture.before || !aCapture.after)
    {
        ItemPreviewRasterMeasurer::ReleaseCapture(aCapture);
        return false;
    }

    apContext->CopyResource(aCapture.after, aCapture.source);

    D3D11_MAPPED_SUBRESOURCE beforeMap{};
    D3D11_MAPPED_SUBRESOURCE afterMap{};
    const HRESULT beforeResult = apContext->Map(
        aCapture.before,
        0,
        D3D11_MAP_READ,
        0,
        &beforeMap);
    const HRESULT afterResult = apContext->Map(
        aCapture.after,
        0,
        D3D11_MAP_READ,
        0,
        &afterMap);

    if (FAILED(beforeResult) || FAILED(afterResult))
    {
        if (SUCCEEDED(afterResult))
            apContext->Unmap(aCapture.after, 0);
        if (SUCCEEDED(beforeResult))
            apContext->Unmap(aCapture.before, 0);

        spdlog::warn(
            "Trade preview raster projection unavailable projectionProbe=true item={:016X} reason=mapFailed beforeResult={:08X} afterResult={:08X}",
            aCapture.telemetry.selectedItem,
            static_cast<std::uint32_t>(beforeResult),
            static_cast<std::uint32_t>(afterResult));
        ItemPreviewRasterMeasurer::ReleaseCapture(aCapture);
        return false;
    }

    const std::uint32_t width = aCapture.description.Width;
    const std::uint32_t height = aCapture.description.Height;
    const std::uint32_t bytesPerPixel =
        GetBytesPerPixel(aCapture.description.Format);

    std::uint32_t minimumX = width;
    std::uint32_t minimumY = height;
    std::uint32_t maximumX = 0;
    std::uint32_t maximumY = 0;
    std::uint64_t changedPixels = 0;
    std::uint8_t maximumChannelDelta = 0;

    constexpr std::uint8_t kChannelDeltaThreshold = 1;
    for (std::uint32_t y = 0; y < height; ++y)
    {
        const auto* const pBeforeRow =
            static_cast<const std::uint8_t*>(beforeMap.pData) +
            static_cast<std::size_t>(y) * beforeMap.RowPitch;
        const auto* const pAfterRow =
            static_cast<const std::uint8_t*>(afterMap.pData) +
            static_cast<std::size_t>(y) * afterMap.RowPitch;

        for (std::uint32_t x = 0; x < width; ++x)
        {
            const auto* const pBeforePixel =
                pBeforeRow + static_cast<std::size_t>(x) * bytesPerPixel;
            const auto* const pAfterPixel =
                pAfterRow + static_cast<std::size_t>(x) * bytesPerPixel;

            std::uint8_t pixelDelta = 0;
            for (std::uint32_t component = 0;
                 component < bytesPerPixel;
                 ++component)
            {
                const int delta =
                    static_cast<int>(pAfterPixel[component]) -
                    static_cast<int>(pBeforePixel[component]);
                pixelDelta = std::max(
                    pixelDelta,
                    static_cast<std::uint8_t>(std::abs(delta)));
            }

            maximumChannelDelta = std::max(
                maximumChannelDelta,
                pixelDelta);
            if (pixelDelta <= kChannelDeltaThreshold)
                continue;

            ++changedPixels;
            minimumX = std::min(minimumX, x);
            minimumY = std::min(minimumY, y);
            maximumX = std::max(maximumX, x);
            maximumY = std::max(maximumY, y);
        }
    }

    apContext->Unmap(aCapture.after, 0);
    apContext->Unmap(aCapture.before, 0);

    const auto clampPixel = [](float aNormalized, std::uint32_t aExtent) {
        const double scaled =
            static_cast<double>(std::clamp(aNormalized, 0.0F, 1.0F)) *
            static_cast<double>(aExtent);
        return static_cast<std::uint32_t>(std::clamp(
            scaled,
            0.0,
            static_cast<double>(aExtent)));
    };

    const std::uint32_t targetLeft = clampPixel(
        aCapture.telemetry.left,
        width);
    const std::uint32_t targetTop = clampPixel(
        aCapture.telemetry.top,
        height);
    const std::uint32_t targetRight = clampPixel(
        aCapture.telemetry.left + aCapture.telemetry.width,
        width);
    const std::uint32_t targetBottom = clampPixel(
        aCapture.telemetry.top + aCapture.telemetry.height,
        height);

    const std::uint32_t targetWidth =
        targetRight > targetLeft ? targetRight - targetLeft : 0;
    const std::uint32_t targetHeight =
        targetBottom > targetTop ? targetBottom - targetTop : 0;
    const std::uint32_t insetX =
        static_cast<std::uint32_t>(std::lround(targetWidth * 0.06));
    const std::uint32_t insetY =
        static_cast<std::uint32_t>(std::lround(targetHeight * 0.06));
    const std::uint32_t safeLeft = std::min(
        targetRight,
        targetLeft + insetX);
    const std::uint32_t safeTop = std::min(
        targetBottom,
        targetTop + insetY);
    const std::uint32_t safeRight =
        targetRight > insetX ? targetRight - insetX : targetLeft;
    const std::uint32_t safeBottom =
        targetBottom > insetY ? targetBottom - insetY : targetTop;

    if (changedPixels == 0)
    {
        spdlog::warn(
            "Trade preview raster projection projectionProbe=true item={:016X} selectionRevision={} regionRevision={} target={}x{} format={} targetPx=({},{},{},{}) safePx=({},{},{},{}) changedPixels=0 maximumChannelDelta={} result=noRasterDifference",
            aCapture.telemetry.selectedItem,
            aCapture.telemetry.selectionRevision,
            aCapture.telemetry.regionRevision,
            width,
            height,
            static_cast<std::uint32_t>(aCapture.description.Format),
            targetLeft,
            targetTop,
            targetRight,
            targetBottom,
            safeLeft,
            safeTop,
            safeRight,
            safeBottom,
            maximumChannelDelta);
        ItemPreviewRasterMeasurer::ReleaseCapture(aCapture);
        return true;
    }

    const std::uint32_t modelRight = maximumX + 1;
    const std::uint32_t modelBottom = maximumY + 1;
    const std::uint32_t modelWidth = modelRight - minimumX;
    const std::uint32_t modelHeight = modelBottom - minimumY;

    const double modelCenterX =
        (static_cast<double>(minimumX) + modelRight) * 0.5;
    const double modelCenterY =
        (static_cast<double>(minimumY) + modelBottom) * 0.5;
    const double targetCenterX =
        (static_cast<double>(targetLeft) + targetRight) * 0.5;
    const double targetCenterY =
        (static_cast<double>(targetTop) + targetBottom) * 0.5;
    const double safeCenterX =
        (static_cast<double>(safeLeft) + safeRight) * 0.5;
    const double safeCenterY =
        (static_cast<double>(safeTop) + safeBottom) * 0.5;

    const std::uint32_t overflowLeft =
        minimumX < targetLeft ? targetLeft - minimumX : 0;
    const std::uint32_t overflowTop =
        minimumY < targetTop ? targetTop - minimumY : 0;
    const std::uint32_t overflowRight =
        modelRight > targetRight ? modelRight - targetRight : 0;
    const std::uint32_t overflowBottom =
        modelBottom > targetBottom ? modelBottom - targetBottom : 0;

    const std::uint32_t safeOverflowLeft =
        minimumX < safeLeft ? safeLeft - minimumX : 0;
    const std::uint32_t safeOverflowTop =
        minimumY < safeTop ? safeTop - minimumY : 0;
    const std::uint32_t safeOverflowRight =
        modelRight > safeRight ? modelRight - safeRight : 0;
    const std::uint32_t safeOverflowBottom =
        modelBottom > safeBottom ? modelBottom - safeBottom : 0;

    const std::uint32_t safeWidth =
        safeRight > safeLeft ? safeRight - safeLeft : 0;
    const std::uint32_t safeHeight =
        safeBottom > safeTop ? safeBottom - safeTop : 0;
    const double widthScale = modelWidth > 0
        ? static_cast<double>(safeWidth) / modelWidth
        : 0.0;
    const double heightScale = modelHeight > 0
        ? static_cast<double>(safeHeight) / modelHeight
        : 0.0;
    const double containScale = std::min(widthScale, heightScale);

    const bool touchesScreenEdge =
        minimumX == 0 || minimumY == 0 ||
        modelRight == width || modelBottom == height;
    const bool insideTarget =
        overflowLeft == 0 && overflowTop == 0 &&
        overflowRight == 0 && overflowBottom == 0;
    const bool insideSafeTarget =
        safeOverflowLeft == 0 && safeOverflowTop == 0 &&
        safeOverflowRight == 0 && safeOverflowBottom == 0;

    aMeasurement.valid = true;
    aMeasurement.targetWidth = width;
    aMeasurement.targetHeight = height;
    aMeasurement.modelLeft = minimumX;
    aMeasurement.modelTop = minimumY;
    aMeasurement.modelRight = modelRight;
    aMeasurement.modelBottom = modelBottom;
    aMeasurement.safeLeft = safeLeft;
    aMeasurement.safeTop = safeTop;
    aMeasurement.safeRight = safeRight;
    aMeasurement.safeBottom = safeBottom;
    aMeasurement.safeOverflowLeft = safeOverflowLeft;
    aMeasurement.safeOverflowTop = safeOverflowTop;
    aMeasurement.safeOverflowRight = safeOverflowRight;
    aMeasurement.safeOverflowBottom = safeOverflowBottom;
    aMeasurement.modelCenterX = static_cast<float>(modelCenterX);
    aMeasurement.modelCenterY = static_cast<float>(modelCenterY);
    aMeasurement.safeCenterX = static_cast<float>(safeCenterX);
    aMeasurement.safeCenterY = static_cast<float>(safeCenterY);
    aMeasurement.containScale = static_cast<float>(containScale);
    aMeasurement.insideSafeTarget = insideSafeTarget;
    aMeasurement.touchesScreenEdge = touchesScreenEdge;

    spdlog::info(
        "Trade preview raster projection projectionProbe=true item={:016X} selectionRevision={} regionRevision={} target={}x{} format={} bpp={} threshold={} changedPixels={} maximumChannelDelta={} modelPx=({},{},{},{}) modelSize={}x{} modelCenter=({:.3f},{:.3f}) targetPx=({},{},{},{}) targetCenter=({:.3f},{:.3f}) safePx=({},{},{},{}) safeCenter=({:.3f},{:.3f}) centerError=({:.3f},{:.3f}) safeCenterError=({:.3f},{:.3f}) overflow=({},{},{},{}) safeOverflow=({},{},{},{}) containScale={:.6f} insideTarget={} insideSafeTarget={} touchesScreenEdge={}",
        aCapture.telemetry.selectedItem,
        aCapture.telemetry.selectionRevision,
        aCapture.telemetry.regionRevision,
        width,
        height,
        static_cast<std::uint32_t>(aCapture.description.Format),
        bytesPerPixel,
        kChannelDeltaThreshold,
        changedPixels,
        maximumChannelDelta,
        minimumX,
        minimumY,
        modelRight,
        modelBottom,
        modelWidth,
        modelHeight,
        modelCenterX,
        modelCenterY,
        targetLeft,
        targetTop,
        targetRight,
        targetBottom,
        targetCenterX,
        targetCenterY,
        safeLeft,
        safeTop,
        safeRight,
        safeBottom,
        safeCenterX,
        safeCenterY,
        modelCenterX - targetCenterX,
        modelCenterY - targetCenterY,
        modelCenterX - safeCenterX,
        modelCenterY - safeCenterY,
        overflowLeft,
        overflowTop,
        overflowRight,
        overflowBottom,
        safeOverflowLeft,
        safeOverflowTop,
        safeOverflowRight,
        safeOverflowBottom,
        containScale,
        insideTarget,
        insideSafeTarget,
        touchesScreenEdge);

    ItemPreviewRasterMeasurer::ReleaseCapture(aCapture);
    return true;
}

