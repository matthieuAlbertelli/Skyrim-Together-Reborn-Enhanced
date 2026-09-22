#ifdef _WIN32
#include "../client/MainMenu/BrandingTexture.h"

#include <catch2/catch.hpp>
#include <wincodec.h>
#include <fstream>
#include <vector>

using Microsoft::WRL::ComPtr;
using STRE::MainMenu::BrandingTexture;

namespace
{
struct PngFixture
{
    std::filesystem::path Path =
        std::filesystem::temp_directory_path() / (L"stre-branding-" + std::to_wstring(GetCurrentProcessId()) + L"-" + std::to_wstring(GetTickCount64()) + L".png");
    HRESULT Apartment{CoInitializeEx(nullptr, COINIT_MULTITHREADED)};
    ~PngFixture()
    {
        std::error_code error;
        std::filesystem::remove(Path, error);
        if (SUCCEEDED(Apartment))
            CoUninitialize();
    }

    void Write(unsigned aWidth, unsigned aHeight, const std::vector<BYTE>& aPixels)
    {
        ComPtr<IWICImagingFactory> factory;
        REQUIRE(SUCCEEDED(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&factory))));
        ComPtr<IWICStream> stream;
        REQUIRE(SUCCEEDED(factory->CreateStream(&stream)));
        REQUIRE(SUCCEEDED(stream->InitializeFromFilename(Path.c_str(), GENERIC_WRITE)));
        ComPtr<IWICBitmapEncoder> encoder;
        REQUIRE(SUCCEEDED(factory->CreateEncoder(GUID_ContainerFormatPng, nullptr, &encoder)));
        REQUIRE(SUCCEEDED(encoder->Initialize(stream.Get(), WICBitmapEncoderNoCache)));
        ComPtr<IWICBitmapFrameEncode> frame;
        REQUIRE(SUCCEEDED(encoder->CreateNewFrame(&frame, nullptr)));
        REQUIRE(SUCCEEDED(frame->Initialize(nullptr)));
        REQUIRE(SUCCEEDED(frame->SetSize(aWidth, aHeight)));
        GUID format = GUID_WICPixelFormat32bppBGRA;
        REQUIRE(SUCCEEDED(frame->SetPixelFormat(&format)));
        REQUIRE(format == GUID_WICPixelFormat32bppBGRA);
        auto encodedPixels = aPixels;
        for (std::size_t i = 0; i < encodedPixels.size(); i += 4)
            std::swap(encodedPixels[i], encodedPixels[i + 2]);
        REQUIRE(SUCCEEDED(frame->WritePixels(aHeight, aWidth * 4, static_cast<UINT>(encodedPixels.size()), encodedPixels.data())));
        REQUIRE(SUCCEEDED(frame->Commit()));
        REQUIRE(SUCCEEDED(encoder->Commit()));
    }
};

ComPtr<ID3D11Device> Device()
{
    ComPtr<ID3D11Device> device;
    REQUIRE(SUCCEEDED(D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, 0, nullptr, 0, D3D11_SDK_VERSION, &device, nullptr, nullptr)));
    return device;
}
} // namespace

TEST_CASE("Optional branding PNG preserves straight alpha and fits its visible content", "[main-menu][main-menu-branding]")
{
    PngFixture fixture;
    std::vector<BYTE> pixels(64 * 32 * 4);
    for (unsigned y = 10; y < 22; ++y)
        for (unsigned x = 20; x < 44; ++x)
        {
            auto* pixel = pixels.data() + (y * 64 + x) * 4;
            pixel[0] = 255;
            pixel[1] = 200;
            pixel[2] = 100;
            pixel[3] = 128;
        }
    pixels[3] = 1; // almost invisible export noise does not determine the crop
    fixture.Write(64, 32, pixels);
    auto device = Device();
    BrandingTexture branding;
    REQUIRE(SUCCEEDED(branding.Load(device.Get(), fixture.Path)));
    REQUIRE(branding.View);
    REQUIRE(branding.Width == 26);
    REQUIRE(branding.Height == 14);
    REQUIRE(branding.Aspect() == Approx(26.0 / 14.0));
    ComPtr<ID3D11Resource> resource;
    branding.View->GetResource(&resource);
    ComPtr<ID3D11Texture2D> source;
    REQUIRE(SUCCEEDED(resource.As(&source)));
    D3D11_TEXTURE2D_DESC description;
    source->GetDesc(&description);
    description.Usage = D3D11_USAGE_STAGING;
    description.BindFlags = 0;
    description.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    ComPtr<ID3D11Texture2D> readback;
    REQUIRE(SUCCEEDED(device->CreateTexture2D(&description, nullptr, &readback)));
    ComPtr<ID3D11DeviceContext> context;
    device->GetImmediateContext(&context);
    context->CopyResource(readback.Get(), source.Get());
    D3D11_MAPPED_SUBRESOURCE mapped;
    REQUIRE(SUCCEEDED(context->Map(readback.Get(), 0, D3D11_MAP_READ, 0, &mapped)));
    const auto* pixel = static_cast<const BYTE*>(mapped.pData) + mapped.RowPitch + 4;
    CHECK(pixel[0] == 255);
    CHECK(pixel[1] == 200);
    CHECK(pixel[2] == 100);
    CHECK(pixel[3] == 128);
    context->Unmap(readback.Get(), 0);
    // A later failure releases the old texture rather than exposing stale art.
    REQUIRE(FAILED(branding.Load(device.Get(), fixture.Path.wstring() + L".missing")));
    REQUIRE_FALSE(branding.View);
    REQUIRE(branding.Width == 0);
}

TEST_CASE("Missing corrupt opaque empty and oversized branding fails independently", "[main-menu][main-menu-branding]")
{
    PngFixture fixture;
    auto device = Device();
    BrandingTexture branding;
    SECTION("missing")
    {
    }
    SECTION("corrupt")
    {
        std::ofstream(fixture.Path) << "not a PNG";
    }
    SECTION("opaque")
    {
        fixture.Write(4, 4, std::vector<BYTE>(4 * 4 * 4, 255));
    }
    SECTION("empty")
    {
        fixture.Write(4, 4, std::vector<BYTE>(4 * 4 * 4));
    }
    SECTION("dimension bound")
    {
        fixture.Write(4097, 1, std::vector<BYTE>(4097 * 4, 255));
    }
    SECTION("pixel bound")
    {
        fixture.Write(2049, 2048, std::vector<BYTE>(2049 * 2048 * 4, 255));
    }
    SECTION("compressed byte bound")
    {
        std::ofstream file(fixture.Path, std::ios::binary);
        file.seekp(16 * 1024 * 1024);
        file.put('x');
    }
    REQUIRE(FAILED(branding.Load(device.Get(), fixture.Path)));
    REQUIRE_FALSE(branding.View);
    REQUIRE(branding.Aspect() == 0);
    REQUIRE(branding.Load(nullptr, fixture.Path) == E_POINTER);
}

TEST_CASE("Shipped branding assets decode on the existing device", "[.][main-menu-branding-assets]")
{
    auto device = Device();
    for (auto name : {"emblem.png", "skyrim-wordmark.png"})
    {
        BrandingTexture texture;
        const auto file = std::filesystem::path("GameFiles/Skyrim/STRE/MainMenu/Branding") / name;
        INFO(file.string());
        REQUIRE(SUCCEEDED(texture.Load(device.Get(), file)));
        REQUIRE(texture.View);
        REQUIRE(texture.Width > 0);
        REQUIRE(texture.Height > 0);
        WARN(name << " cropped texture: " << texture.Width << "x" << texture.Height);
    }
}
#endif
