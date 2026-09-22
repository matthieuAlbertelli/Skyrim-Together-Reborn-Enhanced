#ifdef _WIN32
#include "BrandingTexture.h"

#include <algorithm>
#include <fstream>
#include <vector>
#include <wincodec.h>

namespace STRE::MainMenu
{
HRESULT BrandingTexture::Load(ID3D11Device* apDevice, const std::filesystem::path& aPath, bool aTrimMargins)
{
    View.Reset();
    Width = Height = 0;
    if (!apDevice)
        return E_POINTER;
    // Bound the compressed source before giving it to the OS decoder. Reading
    // one bounded snapshot avoids file-size/check/use races and holds no file.
    std::ifstream file(aPath, std::ios::binary | std::ios::ate);
    const auto size = file.tellg();
    if (!file || size <= 0 || size > 16 * 1024 * 1024)
        return E_INVALIDARG;
    std::vector<BYTE> encoded(static_cast<std::size_t>(size));
    file.seekg(0);
    if (!file.read(reinterpret_cast<char*>(encoded.data()), static_cast<std::streamsize>(encoded.size())))
        return E_FAIL;

    // Balance only our own COM initialization; an existing STA is also usable.
    struct Apartment
    {
        HRESULT Result{CoInitializeEx(nullptr, COINIT_MULTITHREADED)};
        ~Apartment()
        {
            if (SUCCEEDED(Result))
                CoUninitialize();
        }
    } apartment;
    if (FAILED(apartment.Result) && apartment.Result != RPC_E_CHANGED_MODE)
        return apartment.Result;
    using Microsoft::WRL::ComPtr;
    ComPtr<IWICImagingFactory> factory;
    HRESULT hr = CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&factory));
    if (FAILED(hr))
        return hr;
    ComPtr<IWICStream> stream;
    ComPtr<IWICBitmapDecoder> decoder;
    ComPtr<IWICBitmapFrameDecode> frame;
    GUID container{};
    UINT width{}, height{}, frames{};
    if (FAILED(hr = factory->CreateStream(&stream)) || FAILED(hr = stream->InitializeFromMemory(encoded.data(), static_cast<DWORD>(encoded.size()))) ||
        FAILED(hr = factory->CreateDecoderFromStream(stream.Get(), nullptr, WICDecodeMetadataCacheOnDemand, &decoder)) || FAILED(hr = decoder->GetContainerFormat(&container)) ||
        container != GUID_ContainerFormatPng || FAILED(hr = decoder->GetFrameCount(&frames)) || frames != 1 || FAILED(hr = decoder->GetFrame(0, &frame)) ||
        FAILED(hr = frame->GetSize(&width, &height)))
        return FAILED(hr) ? hr : E_INVALIDARG;
    // At most 16 MiB of decoded pixels per image; reject instead of silently
    // resampling a huge/decompression-heavy file on Skyrim's render thread.
    if (!width || !height || width > 4096 || height > 4096 || static_cast<std::uint64_t>(width) * height > 4194304)
        return E_INVALIDARG;
    ComPtr<IWICFormatConverter> converter;
    if (FAILED(hr = factory->CreateFormatConverter(&converter)) ||
        FAILED(hr = converter->Initialize(frame.Get(), GUID_WICPixelFormat32bppRGBA, WICBitmapDitherTypeNone, nullptr, 0, WICBitmapPaletteTypeCustom)))
        return hr;
    std::vector<BYTE> pixels(static_cast<std::size_t>(width) * height * 4);
    if (FAILED(hr = converter->CopyPixels(nullptr, width * 4, static_cast<UINT>(pixels.size()), pixels.data())))
        return hr;
    unsigned left = width, top = height, right = 0, bottom = 0;
    bool transparent = false;
    for (unsigned y = 0; y < height; ++y)
        for (unsigned x = 0; x < width; ++x)
        {
            const auto alpha = pixels[(y * width + x) * 4 + 3];
            transparent = transparent || alpha == 0;
            // Ignore almost invisible export noise for layout only. Keep the
            // original RGBA pixels inside the crop and never rewrite the PNG.
            if (alpha >= 8)
            {
                left = std::min(left, x);
                top = std::min(top, y);
                right = std::max(right, x + 1);
                bottom = std::max(bottom, y + 1);
            }
        }
    if (!transparent || left >= right || top >= bottom)
        return E_INVALIDARG; // opaque rectangles/empty exports cannot cover the menu
    left = left ? left - 1 : 0;
    top = top ? top - 1 : 0;
    right = std::min(right + 1, width);
    bottom = std::min(bottom + 1, height);
    if (!aTrimMargins)
    {
        left = top = 0;
        right = width;
        bottom = height;
    }
    D3D11_TEXTURE2D_DESC description{};
    description.Width = right - left;
    description.Height = bottom - top;
    description.MipLevels = description.ArraySize = 1;
    description.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    description.SampleDesc.Count = 1;
    description.Usage = D3D11_USAGE_IMMUTABLE;
    description.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    const D3D11_SUBRESOURCE_DATA data{pixels.data() + (top * width + left) * 4, width * 4, 0};
    ComPtr<ID3D11Texture2D> texture;
    if (FAILED(hr = apDevice->CreateTexture2D(&description, &data, &texture)) || FAILED(hr = apDevice->CreateShaderResourceView(texture.Get(), nullptr, &View)))
        return hr;
    Width = description.Width;
    Height = description.Height;
    return S_OK;
}
} // namespace STRE::MainMenu
#endif
