#pragma once

#include <d3d11.h>

#include <Services/ItemPreview/ItemPreviewRasterTypes.h>

struct ItemPreviewRasterCapture
{
    ID3D11Texture2D* source{};
    ID3D11Texture2D* before{};
    ID3D11Texture2D* after{};
    D3D11_TEXTURE2D_DESC description{};
    ItemPreviewRasterCaptureRequest telemetry{};
};

class ItemPreviewRasterMeasurer final
{
public:
    static bool BeginCapture(
        ID3D11Device* apDevice,
        ID3D11DeviceContext* apContext,
        const ItemPreviewRasterCaptureRequest& aTelemetry,
        ItemPreviewRasterCapture& aCapture) noexcept;

    static bool CompleteCapture(
        ID3D11DeviceContext* apContext,
        ItemPreviewRasterCapture& aCapture,
        ItemPreviewRasterMeasurement& aMeasurement) noexcept;

    static void ReleaseCapture(
        ItemPreviewRasterCapture& aCapture) noexcept;
};
