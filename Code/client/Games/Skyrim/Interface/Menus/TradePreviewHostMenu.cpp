#include <TiltedOnlinePCH.h>

#include <Interface/Menus/TradePreviewHostMenu.h>

#include <Interface/Inventory3DManager.h>

#include <BSGraphics/BSGraphicsRenderer.h>
#include <Systems/RenderSystemD3D11.h>

#include <d3d11.h>

#include <algorithm>
#include <cmath>

#include <Services/TradeItemPreviewService.h>
#include <World.h>

namespace
{
constexpr const char* kTradePreviewHostMenuName =
    "STRETradePreviewHostMenu";
constexpr const char* kHostScaleformMovieName = "CraftingMenu";
constexpr const char* kHudMenuName = "HUD Menu";
constexpr std::uint32_t kGFxScaleModeExactFit = 2;
constexpr std::size_t kGFxMovieViewAdvanceVtableIndex = 0x25;
constexpr std::size_t kGFxMovieViewDisplayVtableIndex = 0x26;
constexpr std::size_t kGFxMovieViewDisplayPrePassVtableIndex = 0x27;
constexpr std::size_t kGFxMovieViewReleaseVtableIndex = 0x45;

using TLoadScaleformMovie = bool(__fastcall)(
    void*,
    IMenu*,
    void**,
    const char*,
    std::uint32_t,
    float);

const BSFixedString& GetHudMenuName() noexcept
{
    static BSFixedString s_name{kHudMenuName};
    return s_name;
}

void* GetScaleformManager() noexcept
{
    POINTER_SKYRIMSE(void*, scaleformManagerSingleton, 402775);
    void** const ppManager = scaleformManagerSingleton.Get();
    return ppManager ? *ppManager : nullptr;
}

bool LoadHostScaleformMovie(IMenu* apMenu) noexcept
{
    if (!apMenu)
        return false;

    void* const pManager = GetScaleformManager();
    if (!pManager)
        return false;

    POINTER_SKYRIMSE(TLoadScaleformMovie, loadMovie, 82325);
    auto* const pLoadMovie = loadMovie.Get();
    if (!pLoadMovie)
        return false;

    return pLoadMovie(
        pManager,
        apMenu,
        &apMenu->uiMovie,
        kHostScaleformMovieName,
        kGFxScaleModeExactFit,
        0.0F);
}

void** GetScaleformMovieVtable(void* apMovie) noexcept
{
    return apMovie ? *reinterpret_cast<void***>(apMovie) : nullptr;
}

bool AdvanceScaleformMovie(
    void* apMovie,
    float aInterval,
    std::uint32_t aCurrentTime) noexcept
{
    void** const pVtable = GetScaleformMovieVtable(apMovie);
    if (!pVtable)
        return false;

    using TAdvance = float(__fastcall)(void*, float, std::uint32_t);
    auto* const pAdvance = reinterpret_cast<TAdvance*>(
        pVtable[kGFxMovieViewAdvanceVtableIndex]);
    if (!pAdvance)
        return false;

    pAdvance(apMovie, aInterval, aCurrentTime);
    return true;
}

[[maybe_unused]] bool DisplayScaleformMovie(void* apMovie) noexcept
{
    void** const pVtable = GetScaleformMovieVtable(apMovie);
    if (!pVtable)
        return false;

    using TDisplay = void(__fastcall)(void*);
    auto* const pDisplay = reinterpret_cast<TDisplay*>(
        pVtable[kGFxMovieViewDisplayVtableIndex]);
    if (!pDisplay)
        return false;

    pDisplay(apMovie);
    return true;
}

bool DisplayScaleformMoviePrePass(void* apMovie) noexcept
{
    void** const pVtable = GetScaleformMovieVtable(apMovie);
    if (!pVtable)
        return false;

    using TDisplayPrePass = void(__fastcall)(void*);
    auto* const pDisplayPrePass = reinterpret_cast<TDisplayPrePass*>(
        pVtable[kGFxMovieViewDisplayPrePassVtableIndex]);
    if (!pDisplayPrePass)
        return false;

    pDisplayPrePass(apMovie);
    return true;
}

void ReleaseScaleformMovie(void*& arpMovie) noexcept
{
    void* const pMovie = arpMovie;
    arpMovie = nullptr;

    void** const pVtable = GetScaleformMovieVtable(pMovie);
    if (!pVtable)
        return;

    using TRelease = void(__fastcall)(void*);
    auto* const pRelease = reinterpret_cast<TRelease*>(
        pVtable[kGFxMovieViewReleaseVtableIndex]);
    if (pRelease)
        pRelease(pMovie);
}

void ReleaseScaleformDelegate(void*& arpDelegate) noexcept
{
    void* const pDelegate = arpDelegate;
    arpDelegate = nullptr;
    if (!pDelegate)
        return;

    auto* const pRefCount = reinterpret_cast<std::uint32_t*>(
        reinterpret_cast<std::byte*>(pDelegate) + 0x8);
    std::atomic_ref<std::uint32_t> refCount{*pRefCount};
    if (refCount.fetch_sub(1, std::memory_order_acq_rel) == 1)
        delete reinterpret_cast<GRefCountImpl*>(pDelegate);
}

template <class T>
void SafeRelease(T*& apObject) noexcept
{
    if (!apObject)
        return;

    apObject->Release();
    apObject = nullptr;
}

struct D3DRenderStateSnapshot
{
    std::uintptr_t renderTargetView{};
    std::uintptr_t renderTargetResource{};
    std::uintptr_t depthStencilView{};
    std::uintptr_t swapChainRenderTargetView{};
    std::uintptr_t swapChainTexture{};
    bool matchesSwapChainView{};
    bool matchesSwapChainTexture{};
    bool hasTargetDescription{};
    std::uint32_t targetWidth{};
    std::uint32_t targetHeight{};
    std::uint32_t targetFormat{};
    std::uint32_t targetSampleCount{};
    std::uint32_t targetSampleQuality{};
    std::uint32_t windowWidth{};
    std::uint32_t windowHeight{};
    std::uint32_t viewportCount{};
    float viewportX{};
    float viewportY{};
    float viewportWidth{};
    float viewportHeight{};
    float viewportMinDepth{};
    float viewportMaxDepth{};
    std::uint32_t scissorCount{};
    long scissorLeft{};
    long scissorTop{};
    long scissorRight{};
    long scissorBottom{};
};

D3DRenderStateSnapshot CaptureRenderState(
    ID3D11DeviceContext* apContext) noexcept
{
    D3DRenderStateSnapshot state{};
    if (!apContext)
        return state;

    ID3D11RenderTargetView* pRenderTargetView = nullptr;
    ID3D11DepthStencilView* pDepthStencilView = nullptr;
    apContext->OMGetRenderTargets(
        1,
        &pRenderTargetView,
        &pDepthStencilView);

    state.renderTargetView =
        reinterpret_cast<std::uintptr_t>(pRenderTargetView);
    state.depthStencilView =
        reinterpret_cast<std::uintptr_t>(pDepthStencilView);

    BSGraphics::RendererWindow* const pWindow =
        BSGraphics::GetMainWindow();
    if (pWindow)
    {
        state.swapChainRenderTargetView = reinterpret_cast<std::uintptr_t>(
            pWindow->SwapChainRenderTarget.pRTView);
        state.swapChainTexture = reinterpret_cast<std::uintptr_t>(
            pWindow->SwapChainRenderTarget.pTexture);
        state.windowWidth = static_cast<std::uint32_t>(pWindow->uiWindowWidth);
        state.windowHeight = static_cast<std::uint32_t>(pWindow->uiWindowHeight);
        state.matchesSwapChainView =
            pRenderTargetView == pWindow->SwapChainRenderTarget.pRTView;
    }

    ID3D11Resource* pRenderTargetResource = nullptr;
    if (pRenderTargetView)
        pRenderTargetView->GetResource(&pRenderTargetResource);

    state.renderTargetResource =
        reinterpret_cast<std::uintptr_t>(pRenderTargetResource);
    if (pWindow)
    {
        state.matchesSwapChainTexture =
            pRenderTargetResource == pWindow->SwapChainRenderTarget.pTexture;
    }

    if (pRenderTargetResource)
    {
        D3D11_RESOURCE_DIMENSION dimension = D3D11_RESOURCE_DIMENSION_UNKNOWN;
        pRenderTargetResource->GetType(&dimension);
        if (dimension == D3D11_RESOURCE_DIMENSION_TEXTURE2D)
        {
            ID3D11Texture2D* pTexture = nullptr;
            const HRESULT queryResult = pRenderTargetResource->QueryInterface(
                __uuidof(ID3D11Texture2D),
                reinterpret_cast<void**>(&pTexture));
            if (SUCCEEDED(queryResult) && pTexture)
            {
                D3D11_TEXTURE2D_DESC description{};
                pTexture->GetDesc(&description);
                state.hasTargetDescription = true;
                state.targetWidth = description.Width;
                state.targetHeight = description.Height;
                state.targetFormat =
                    static_cast<std::uint32_t>(description.Format);
                state.targetSampleCount = description.SampleDesc.Count;
                state.targetSampleQuality = description.SampleDesc.Quality;
            }

            SafeRelease(pTexture);
        }
    }

    D3D11_VIEWPORT viewports[
        D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE]{};
    UINT viewportCount =
        D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;
    apContext->RSGetViewports(&viewportCount, viewports);
    state.viewportCount = viewportCount;
    if (viewportCount > 0)
    {
        state.viewportX = viewports[0].TopLeftX;
        state.viewportY = viewports[0].TopLeftY;
        state.viewportWidth = viewports[0].Width;
        state.viewportHeight = viewports[0].Height;
        state.viewportMinDepth = viewports[0].MinDepth;
        state.viewportMaxDepth = viewports[0].MaxDepth;
    }

    D3D11_RECT scissorRects[
        D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE]{};
    UINT scissorCount =
        D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;
    apContext->RSGetScissorRects(&scissorCount, scissorRects);
    state.scissorCount = scissorCount;
    if (scissorCount > 0)
    {
        state.scissorLeft = scissorRects[0].left;
        state.scissorTop = scissorRects[0].top;
        state.scissorRight = scissorRects[0].right;
        state.scissorBottom = scissorRects[0].bottom;
    }

    SafeRelease(pRenderTargetResource);
    SafeRelease(pDepthStencilView);
    SafeRelease(pRenderTargetView);
    return state;
}

void LogRenderState(
    const char* apPhase,
    ID3D11DeviceContext* apContext) noexcept
{
    const D3DRenderStateSnapshot state =
        CaptureRenderState(apContext);

    spdlog::info(
        "Trade preview D3D state phase={} targetProbe=true context={:p} rtv={:p} resource={:p} dsv={:p} swapRtv={:p} swapTexture={:p} matchesSwapRtv={} matchesSwapTexture={} targetDesc={} target={}x{} format={} samples={}:{} window={}x{} viewportCount={} viewport0=({:.3f},{:.3f},{:.3f},{:.3f},{:.3f},{:.3f}) scissorCount={} scissor0=({},{},{},{})",
        apPhase,
        static_cast<void*>(apContext),
        reinterpret_cast<void*>(state.renderTargetView),
        reinterpret_cast<void*>(state.renderTargetResource),
        reinterpret_cast<void*>(state.depthStencilView),
        reinterpret_cast<void*>(state.swapChainRenderTargetView),
        reinterpret_cast<void*>(state.swapChainTexture),
        state.matchesSwapChainView,
        state.matchesSwapChainTexture,
        state.hasTargetDescription,
        state.targetWidth,
        state.targetHeight,
        state.targetFormat,
        state.targetSampleCount,
        state.targetSampleQuality,
        state.windowWidth,
        state.windowHeight,
        state.viewportCount,
        state.viewportX,
        state.viewportY,
        state.viewportWidth,
        state.viewportHeight,
        state.viewportMinDepth,
        state.viewportMaxDepth,
        state.scissorCount,
        state.scissorLeft,
        state.scissorTop,
        state.scissorRight,
        state.scissorBottom);
}

bool ShouldLogManagerState(std::uint32_t aRenderCall) noexcept
{
    switch (aRenderCall)
    {
    case 1:
    case 2:
    case 5:
    case 15:
    case 30:
    case 60:
    case 120:
    case 300:
    case 600:
        return true;
    default:
        return false;
    }
}

void LogManagerState(
    const char* apPhase,
    const Inventory3DManager* apManager,
    std::uint32_t aRenderCall) noexcept
{
    if (!apManager)
        return;

    const Inventory3DManagerDebugState state =
        apManager->CaptureDebugState();

    spdlog::info(
        "Trade preview Inventory3D state phase={} loadStateProbe=true layout=AE1170 frame={} manager={:p} posCopy=({:.3f},{:.3f},{:.3f}) pos=({:.3f},{:.3f},{:.3f}) scaleCopy={:.3f} scale={:.3f} light={} tempRef={:p} loadedCapacityFlags={:08X} loadedCapacity={} loadedLocal={} loadedSize={} loadedData={:p} firstItemBase={:p} firstModelObject={:p} firstSceneObject={:p} zoom={:.3f} loadTask={:p} stateFlags={:08X} tailFlags={:08X}",
        apPhase,
        aRenderCall,
        static_cast<const void*>(apManager),
        state.itemPosCopyX,
        state.itemPosCopyY,
        state.itemPosCopyZ,
        state.itemPosX,
        state.itemPosY,
        state.itemPosZ,
        state.itemScaleCopy,
        state.itemScale,
        state.lightScheme,
        reinterpret_cast<void*>(state.tempRef),
        state.loadedModelsCapacityFlags,
        state.loadedModelsCapacity,
        state.loadedModelsLocal,
        state.loadedModelsSize,
        reinterpret_cast<void*>(state.loadedModelsData),
        reinterpret_cast<void*>(state.firstItemBase),
        reinterpret_cast<void*>(state.firstModelObject),
        reinterpret_cast<void*>(state.firstSceneObject),
        state.zoomProgress,
        reinterpret_cast<void*>(state.loadTask),
        state.stateFlags,
        state.tailFlags);
}

struct ProjectionRasterCapture
{
    ID3D11Texture2D* source{};
    ID3D11Texture2D* before{};
    ID3D11Texture2D* after{};
    D3D11_TEXTURE2D_DESC description{};
    TradePreviewProjectionTelemetryState telemetry{};
};

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

void ReleaseProjectionRasterCapture(
    ProjectionRasterCapture& aCapture) noexcept
{
    SafeRelease(aCapture.after);
    SafeRelease(aCapture.before);
    SafeRelease(aCapture.source);
    aCapture = {};
}

bool BeginProjectionRasterCapture(
    ID3D11Device* apDevice,
    ID3D11DeviceContext* apContext,
    const TradePreviewProjectionTelemetryState& aTelemetry,
    ProjectionRasterCapture& aCapture) noexcept
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

bool CompleteProjectionRasterCapture(
    ID3D11DeviceContext* apContext,
    ProjectionRasterCapture& aCapture,
    TradePreviewRasterMeasurement& aMeasurement) noexcept
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
        ReleaseProjectionRasterCapture(aCapture);
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
        ReleaseProjectionRasterCapture(aCapture);
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
        ReleaseProjectionRasterCapture(aCapture);
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

    ReleaseProjectionRasterCapture(aCapture);
    return true;
}

}

const BSFixedString& TradePreviewHostMenu::GetName() noexcept
{
    static BSFixedString s_name{kTradePreviewHostMenuName};
    return s_name;
}

bool TradePreviewHostMenu::Register() noexcept
{
    UI* const pUI = UI::Get();
    if (!pUI)
    {
        spdlog::warn(
            "Trade preview host menu registration deferred: UI unavailable");
        return false;
    }

    const bool registered =
        pUI->RegisterMenu(GetName(), &TradePreviewHostMenu::Create);

    spdlog::info(
        "Trade preview host menu registration attempt result={}",
        registered);

    return registered;
}

void TradePreviewHostMenu::Show() noexcept
{
    UI* const pUI = UI::Get();
    if (!pUI)
    {
        spdlog::warn(
            "Trade preview host menu show skipped: UI unavailable");
        return;
    }

    if (!pUI->HasMenuRegistration(GetName()) && !Register())
    {
        spdlog::error(
            "Trade preview host menu show aborted: registration unavailable");
        return;
    }

    spdlog::info("Trade preview host menu show requested");
    pUI->QueueMessage(GetName(), UIMessage::kShow);
}

void TradePreviewHostMenu::Hide() noexcept
{
    UI* const pUI = UI::Get();
    if (!pUI || !pUI->HasMenuRegistration(GetName()))
        return;

    spdlog::info("Trade preview host menu hide requested");
    pUI->QueueMessage(GetName(), UIMessage::kHide);
}

TradePreviewHostMenu::TradePreviewHostMenu() noexcept
{
    uiMovie = nullptr;
    depthPriority = 0;
    eInputContext = 0;

    uiMenuFlags =
        kDontHideCursorWhenTopmost |
        kInventoryItemMenu |
        kRequiresUpdate |
        kUpdateUsesCursor |
        kDisablePauseMenu |
        kUsesMenuContext;

    m_hostMovieLoaded = LoadHostScaleformMovie(this);

    const std::uint32_t flagsBeforeCursorSuppression = uiMenuFlags;
    uiMenuFlags &= ~(kUsesCursor | kDontHideCursorWhenTopmost);
    spdlog::info(
        "Trade preview native cursor flags suppressed cursorProbe=true flagsBefore={:08X} flagsAfter={:08X} usesCursor={} updateUsesCursor={} dontHideCursor={}",
        flagsBeforeCursorSuppression,
        uiMenuFlags,
        (uiMenuFlags & kUsesCursor) != 0,
        (uiMenuFlags & kUpdateUsesCursor) != 0,
        (uiMenuFlags & kDontHideCursorWhenTopmost) != 0);

    spdlog::info(
        "Trade preview host menu constructed lifecycleProbe=true hostMovieProbe=true menuFlagProbe=true flags={:08X} movieName={} movieLoaded={} movie={:p} fxDelegate={:p}",
        uiMenuFlags,
        kHostScaleformMovieName,
        m_hostMovieLoaded,
        uiMovie,
        fxDelegate);
}

TradePreviewHostMenu::~TradePreviewHostMenu()
{
    const bool wasStarted = m_started;
    StopPreview();
    ResetRenderQueries();

    void* const pMovie = uiMovie;
    void* const pDelegate = fxDelegate;
    ReleaseScaleformDelegate(fxDelegate);
    ReleaseScaleformMovie(uiMovie);

    spdlog::info(
        "Trade preview host menu destroyed lifecycleProbe=true hostMovieProbe=true wasStarted={} movieLoaded={} releasedMovie={:p} releasedDelegate={:p}",
        wasStarted,
        m_hostMovieLoaded,
        pMovie,
        pDelegate);
}

void TradePreviewHostMenu::Accept(CallbackProcessor*)
{
}

void TradePreviewHostMenu::PostCreate()
{
}

void TradePreviewHostMenu::Unk_03()
{
}

UI_MESSAGE_RESULTS TradePreviewHostMenu::ProcessMessage(
    UIMessage& aMessage)
{
    switch (aMessage.eType)
    {
    case UIMessage::kUpdate:
        ++m_updateMessageCount;
        if (!m_updateMessageLogged)
        {
            m_updateMessageLogged = true;
            spdlog::info(
                "Trade preview host menu ProcessMessage firstUpdate lifecycleProbe=true uiThreadReloadProbe=true");
        }
        if (m_started && m_begin3DInvoked)
        {
            World::Get()
                .ctx()
                .at<TradeItemPreviewService>()
                .ProcessPendingFitReloadOnUiThread();
        }
        return UI_MESSAGE_RESULTS::kPassOn;

    case UIMessage::kShow:
    case UIMessage::kReshow:
        spdlog::info(
            "Trade preview host menu ProcessMessage type={} lifecycleProbe=true",
            static_cast<std::uint32_t>(aMessage.eType));
        StartPreview();
        return UI_MESSAGE_RESULTS::kHandled;

    case UIMessage::kHide:
    case UIMessage::kForceHide:
        spdlog::info(
            "Trade preview host menu ProcessMessage type={} lifecycleProbe=true",
            static_cast<std::uint32_t>(aMessage.eType));
        StopPreview();
        return UI_MESSAGE_RESULTS::kHandled;

    default:
        return UI_MESSAGE_RESULTS::kPassOn;
    }
}

void TradePreviewHostMenu::AdvanceMovie(
    float aInterval,
    std::uint32_t)
{
    static std::atomic_bool s_advanceMovieLogged{false};
    if (!s_advanceMovieLogged.exchange(
            true,
            std::memory_order_relaxed))
    {
        spdlog::info(
            "Trade preview host menu AdvanceMovie first frame uiThreadReloadProbe=true");
    }

    if (m_started && m_begin3DInvoked)
    {
        World::Get()
            .ctx()
            .at<TradeItemPreviewService>()
            .ProcessPendingFitReloadOnUiThread();
    }

    if (uiMovie)
        AdvanceScaleformMovie(uiMovie, aInterval, 2);
}

void TradePreviewHostMenu::PostDisplay()
{
    if (!m_started)
        return;

    if (!m_postDisplayLogged)
    {
        m_postDisplayLogged = true;
        spdlog::info(
            "Trade preview host menu PostDisplay first frame renderProbe=true hostMovieProbe=true movieLoaded={} movie={:p}",
            m_hostMovieLoaded,
            uiMovie);
    }

    if (!m_begin3DInvoked)
        return;

    Inventory3DManager* const pManager =
        Inventory3DManager::GetSingleton();
    if (!pManager)
    {
        if (!m_renderUnavailableLogged)
        {
            m_renderUnavailableLogged = true;
            spdlog::warn(
                "Trade preview host menu Render skipped renderProbe=true managerUnavailable=true");
        }
        return;
    }

    TradeItemPreviewService& previewService =
        World::Get().ctx().at<TradeItemPreviewService>();
    previewService.UpdatePreviewPlacement();
    const TradePreviewProjectionTelemetryState projectionTelemetry =
        previewService.CaptureProjectionTelemetryState();

    RenderSystemD3D11& renderSystem =
        World::Get().ctx().at<RenderSystemD3D11>();
    ID3D11Device* const pDevice = renderSystem.GetDevice();
    ID3D11DeviceContext* const pContext =
        renderSystem.GetDeviceContext();

    PollRenderQueries(pContext);

    const bool projectionTelemetryPending =
        projectionTelemetry.valid &&
        projectionTelemetry.captureRequested &&
        (projectionTelemetry.selectionRevision !=
             m_projectionTelemetrySelectionRevision ||
         projectionTelemetry.regionRevision !=
             m_projectionTelemetryRegionRevision ||
         projectionTelemetry.solverRevision !=
             m_projectionTelemetrySolverRevision);
    ProjectionRasterCapture projectionCapture{};
    bool projectionCaptureStarted = false;
    if (projectionTelemetryPending)
    {
        // Mark the revision before attempting the readback so an unsupported
        // render-target format cannot produce one warning per frame.
        m_projectionTelemetrySelectionRevision =
            projectionTelemetry.selectionRevision;
        m_projectionTelemetryRegionRevision =
            projectionTelemetry.regionRevision;
        m_projectionTelemetrySolverRevision =
            projectionTelemetry.solverRevision;
        projectionCaptureStarted = BeginProjectionRasterCapture(
            pDevice,
            pContext,
            projectionTelemetry,
            projectionCapture);
        if (!projectionCaptureStarted)
        {
            TradePreviewRasterMeasurement measurement{};
            measurement.selectionRevision =
                projectionTelemetry.selectionRevision;
            measurement.regionRevision =
                projectionTelemetry.regionRevision;
            measurement.solverRevision =
                projectionTelemetry.solverRevision;
            measurement.selectedItem =
                projectionTelemetry.selectedItem;
            previewService.SubmitProjectionMeasurement(measurement);
        }
    }

    const bool firstRenderProbe = m_renderCallCount == 0;
    if (firstRenderProbe)
    {
        spdlog::info(
            "Trade preview external viewport disabled transformProbe=true reason=Inventory3DManagerOverridesRasterState");
        LogRenderState("before-model-only-render", pContext);
        BeginRenderQueries(pDevice, pContext);
    }

    const std::uint32_t renderResult = pManager->Render();

    if (projectionCaptureStarted)
    {
        TradePreviewRasterMeasurement measurement{};
        (void)CompleteProjectionRasterCapture(
            pContext,
            projectionCapture,
            measurement);
        previewService.SubmitProjectionMeasurement(measurement);
    }

    // v23.10: keep the host-owned movie loaded so this remains a genuine
    // inventory-style menu, but deliberately suppress its 2D Display pass.
    // This isolates the native 3D renderer and prevents CraftingMenu.swf from
    // drawing its incomplete inventory panel and item-card shell.
    if (!m_hostMovieDisplayLogged)
    {
        m_hostMovieDisplayLogged = true;
        spdlog::info(
            "Trade preview host movie Display suppressed modelOnlyProbe=true hostMovieProbe=true movieName={} movieLoaded={} movie={:p}",
            kHostScaleformMovieName,
            m_hostMovieLoaded,
            uiMovie);
    }

    if (firstRenderProbe)
    {
        EndRenderQueries(pContext);
        LogRenderState("after-model-only-render", pContext);
    }

    ++m_renderCallCount;
    if (renderResult != 0)
        ++m_renderNonZeroCount;

    if (ShouldLogManagerState(m_renderCallCount))
    {
        LogManagerState(
            "post-render",
            pManager,
            m_renderCallCount);
    }

    if (!m_firstRenderLogged)
    {
        m_firstRenderLogged = true;
        m_firstRenderResult = renderResult;
        spdlog::info(
            "Trade preview host menu Render first call renderProbe=true modelOnlyProbe=true hostMovieProbe=true manager={:p} result={} movieDisplaySuppressed=true",
            static_cast<void*>(pManager),
            renderResult);
    }
}

void TradePreviewHostMenu::PreDisplay()
{
    if (!m_started || !uiMovie)
        return;

    const bool displayed = DisplayScaleformMoviePrePass(uiMovie);
    if (!m_hostMoviePrePassLogged)
    {
        m_hostMoviePrePassLogged = true;
        spdlog::info(
            "Trade preview host movie DisplayPrePass hostMovieProbe=true movieName={} movie={:p} result={} vtableIndex={:02X}",
            kHostScaleformMovieName,
            uiMovie,
            displayed,
            kGFxMovieViewDisplayPrePassVtableIndex);
    }
}

void TradePreviewHostMenu::RefreshPlatform()
{
}

IMenu* TradePreviewHostMenu::Create(UIMessage*)
{
    spdlog::info("Trade preview host menu factory called");
    return new TradePreviewHostMenu();
}

void TradePreviewHostMenu::StartPreview() noexcept
{
    if (m_started)
        return;

    m_started = true;
    m_postDisplayLogged = false;
    m_updateMessageLogged = false;
    m_updateMessageCount = 0;
    m_begin3DInvoked = false;
    m_firstRenderLogged = false;
    m_renderUnavailableLogged = false;
    m_hostMovieDisplayLogged = false;
    m_hostMovieDisplayReturnedLogged = false;
    m_hostMovieUnavailableLogged = false;
    m_hostMoviePrePassLogged = false;
    m_hostMovieDisplayCalls = 0;
    m_renderCallCount = 0;
    m_renderNonZeroCount = 0;
    m_firstRenderResult = 0;
    m_projectionTelemetrySelectionRevision = 0;
    m_projectionTelemetryRegionRevision = 0;
    m_projectionTelemetrySolverRevision = 0;
    m_hudMenuWasOpen = false;
    m_hudHideQueued = false;
    ResetRenderQueries();

    if (UI* const pUI = UI::Get())
    {
        m_hudMenuWasOpen = pUI->GetMenuOpen(GetHudMenuName());
        if (m_hudMenuWasOpen)
        {
            pUI->QueueMessage(
                GetHudMenuName(),
                UIMessage::kHide);
            m_hudHideQueued = true;
        }

        spdlog::info(
            "Trade preview HUD suppression requested hudProbe=true hudWasOpen={} hideQueued={}",
            m_hudMenuWasOpen,
            m_hudHideQueued);
    }

    Inventory3DManager* const pManager =
        Inventory3DManager::GetSingleton();
    if (pManager)
    {
        spdlog::info(
            "Trade preview host menu Begin3D calling beginEndProbe=true manager={:p} lightScheme=1",
            static_cast<void*>(pManager));
        pManager->Begin3D(1);
        m_begin3DInvoked = true;
        spdlog::info(
            "Trade preview host menu Begin3D returned beginEndProbe=true");
    }
    else
    {
        spdlog::warn(
            "Trade preview host menu Begin3D skipped beginEndProbe=true managerUnavailable=true");
    }

    World::Get()
        .ctx()
        .at<TradeItemPreviewService>()
        .OnHostMenuShown(true);

    spdlog::info(
        "Trade preview host menu started lifecycleProbe=true beginEndProbe=true begin3DInvoked={}",
        m_begin3DInvoked);
}

void TradePreviewHostMenu::StopPreview() noexcept
{
    if (!m_started)
        return;

    if (m_begin3DInvoked)
    {
        RenderSystemD3D11& renderSystem =
            World::Get().ctx().at<RenderSystemD3D11>();
        PollRenderQueries(renderSystem.GetDeviceContext());

        Inventory3DManager* const pManager =
            Inventory3DManager::GetSingleton();
        if (pManager)
        {
            spdlog::info(
                "Trade preview host menu End3D calling beginEndProbe=true manager={:p}",
                static_cast<void*>(pManager));
            pManager->End3D();
            spdlog::info(
                "Trade preview host menu End3D returned beginEndProbe=true");
        }
        else
        {
            spdlog::warn(
                "Trade preview host menu End3D skipped beginEndProbe=true managerUnavailable=true");
        }

        m_begin3DInvoked = false;
    }

    World::Get()
        .ctx()
        .at<TradeItemPreviewService>()
        .OnHostMenuHidden();

    if (m_hudMenuWasOpen)
    {
        if (UI* const pUI = UI::Get())
        {
            pUI->QueueMessage(
                GetHudMenuName(),
                UIMessage::kShow);
            spdlog::info(
                "Trade preview HUD restore requested hudProbe=true showQueued=true");
        }
    }
    m_hudMenuWasOpen = false;
    m_hudHideQueued = false;

    m_started = false;
    spdlog::info(
        "Trade preview host menu stopped renderProbe=true targetProbe=true modelOnlyProbe=true hostMovieProbe=true transformProbe=true uiDrivenFit=true cursorProbe=true hudProbe=true beginEndProbe=true movieLoaded={} updateMessages={} renderCalls={} renderNonZero={} firstRenderResult={} hostMovieDisplayCalls={} queryPolls={} pipelineStatsLogged={} occlusionLogged={}",
        m_hostMovieLoaded,
        m_updateMessageCount,
        m_renderCallCount,
        m_renderNonZeroCount,
        m_firstRenderResult,
        m_hostMovieDisplayCalls,
        m_queryPollCount,
        m_pipelineStatsLogged,
        m_occlusionLogged);

    ResetRenderQueries();
}

void TradePreviewHostMenu::ResetRenderQueries() noexcept
{
    SafeRelease(m_pipelineQuery);
    SafeRelease(m_occlusionQuery);
    m_renderQueriesIssued = false;
    m_pipelineStatsLogged = false;
    m_occlusionLogged = false;
    m_pipelineQueryFailed = false;
    m_occlusionQueryFailed = false;
    m_queryPollCount = 0;
}

void TradePreviewHostMenu::BeginRenderQueries(
    ID3D11Device* apDevice,
    ID3D11DeviceContext* apContext) noexcept
{
    if (!apDevice || !apContext || m_renderQueriesIssued)
        return;

    D3D11_QUERY_DESC description{};
    description.Query = D3D11_QUERY_PIPELINE_STATISTICS;
    HRESULT result = apDevice->CreateQuery(
        &description,
        &m_pipelineQuery);
    if (FAILED(result))
    {
        m_pipelineQueryFailed = true;
        spdlog::warn(
            "Trade preview pipeline query creation failed targetProbe=true hr={:08X}",
            static_cast<std::uint32_t>(result));
    }

    description.Query = D3D11_QUERY_OCCLUSION;
    result = apDevice->CreateQuery(
        &description,
        &m_occlusionQuery);
    if (FAILED(result))
    {
        m_occlusionQueryFailed = true;
        spdlog::warn(
            "Trade preview occlusion query creation failed targetProbe=true hr={:08X}",
            static_cast<std::uint32_t>(result));
    }

    if (m_pipelineQuery)
        apContext->Begin(m_pipelineQuery);
    if (m_occlusionQuery)
        apContext->Begin(m_occlusionQuery);

    m_renderQueriesIssued =
        m_pipelineQuery != nullptr || m_occlusionQuery != nullptr;

    spdlog::info(
        "Trade preview GPU queries begun targetProbe=true pipelineQuery={:p} occlusionQuery={:p}",
        static_cast<void*>(m_pipelineQuery),
        static_cast<void*>(m_occlusionQuery));
}

void TradePreviewHostMenu::EndRenderQueries(
    ID3D11DeviceContext* apContext) noexcept
{
    if (!apContext || !m_renderQueriesIssued)
        return;

    if (m_pipelineQuery)
        apContext->End(m_pipelineQuery);
    if (m_occlusionQuery)
        apContext->End(m_occlusionQuery);

    spdlog::info(
        "Trade preview GPU queries ended targetProbe=true");
}

void TradePreviewHostMenu::PollRenderQueries(
    ID3D11DeviceContext* apContext) noexcept
{
    if (!apContext || !m_renderQueriesIssued)
        return;

    ++m_queryPollCount;

    if (m_pipelineQuery && !m_pipelineStatsLogged)
    {
        D3D11_QUERY_DATA_PIPELINE_STATISTICS statistics{};
        const HRESULT result = apContext->GetData(
            m_pipelineQuery,
            &statistics,
            sizeof(statistics),
            D3D11_ASYNC_GETDATA_DONOTFLUSH);
        if (result == S_OK)
        {
            m_pipelineStatsLogged = true;
            spdlog::info(
                "Trade preview GPU pipeline statistics targetProbe=true polls={} iaVertices={} iaPrimitives={} vsInvocations={} gsInvocations={} gsPrimitives={} clipInvocations={} clipPrimitives={} psInvocations={}",
                m_queryPollCount,
                statistics.IAVertices,
                statistics.IAPrimitives,
                statistics.VSInvocations,
                statistics.GSInvocations,
                statistics.GSPrimitives,
                statistics.CInvocations,
                statistics.CPrimitives,
                statistics.PSInvocations);
        }
        else if (FAILED(result) && !m_pipelineQueryFailed)
        {
            m_pipelineQueryFailed = true;
            spdlog::warn(
                "Trade preview pipeline query read failed targetProbe=true hr={:08X}",
                static_cast<std::uint32_t>(result));
        }
    }

    if (m_occlusionQuery && !m_occlusionLogged)
    {
        std::uint64_t samplesPassed = 0;
        const HRESULT result = apContext->GetData(
            m_occlusionQuery,
            &samplesPassed,
            sizeof(samplesPassed),
            D3D11_ASYNC_GETDATA_DONOTFLUSH);
        if (result == S_OK)
        {
            m_occlusionLogged = true;
            spdlog::info(
                "Trade preview GPU occlusion result targetProbe=true polls={} samplesPassed={}",
                m_queryPollCount,
                samplesPassed);
        }
        else if (FAILED(result) && !m_occlusionQueryFailed)
        {
            m_occlusionQueryFailed = true;
            spdlog::warn(
                "Trade preview occlusion query read failed targetProbe=true hr={:08X}",
                static_cast<std::uint32_t>(result));
        }
    }
}

