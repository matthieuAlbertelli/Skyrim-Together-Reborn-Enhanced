#if defined(_WIN32)
#include "../client/MainMenu/VideoPlayer.h"

#include <Windows.h>
#include <d3d11.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <wrl/client.h>
#include <catch2/catch.hpp>

#include <chrono>
#include <fstream>
#include <thread>
#include <vector>

namespace
{
using Microsoft::WRL::ComPtr;
using namespace STRE::MainMenu;
constexpr unsigned cWidth = 320;
constexpr unsigned cHeight = 240;

// Explicit Windows smoke ([main-menu-media]), not a prerequisite for the
// portable/default suite. Generates its own tiny silent MP4; no shipped asset.
struct MediaFixture
{
    MediaFixture()
    {
        REQUIRE(SUCCEEDED(CoInitializeEx(nullptr, COINIT_MULTITHREADED)));
        REQUIRE(SUCCEEDED(MFStartup(MF_VERSION)));
        GUID id{};
        REQUIRE(SUCCEEDED(CoCreateGuid(&id)));
        wchar_t name[40]{};
        StringFromGUID2(id, name, 40);
        Directory = std::filesystem::temp_directory_path() / name;
        REQUIRE(std::filesystem::create_directory(Directory));
        REQUIRE(SUCCEEDED(D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, D3D11_CREATE_DEVICE_BGRA_SUPPORT, nullptr, 0, D3D11_SDK_VERSION, &Device, nullptr, &Context)));
    }

    ~MediaFixture()
    {
        Context.Reset();
        Device.Reset();
        MFShutdown();
        CoUninitialize();
        std::error_code ec;
        // Only the two exact files authored by this fixture are removed.
        std::filesystem::remove(Directory / "generated.mp4", ec);
        std::filesystem::remove(Directory / "corrupt.mp4", ec);
        std::filesystem::remove(Directory, ec);
    }

    std::filesystem::path MakeVideo()
    {
        const auto file = Directory / "generated.mp4";
        ComPtr<IMFSinkWriter> writer;
        REQUIRE(SUCCEEDED(MFCreateSinkWriterFromURL(file.c_str(), nullptr, nullptr, &writer)));
        ComPtr<IMFMediaType> output;
        REQUIRE(SUCCEEDED(MFCreateMediaType(&output)));
        REQUIRE(SUCCEEDED(output->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video)));
        REQUIRE(SUCCEEDED(output->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_H264)));
        REQUIRE(SUCCEEDED(output->SetUINT32(MF_MT_AVG_BITRATE, 100000)));
        REQUIRE(SUCCEEDED(output->SetUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive)));
        REQUIRE(SUCCEEDED(MFSetAttributeSize(output.Get(), MF_MT_FRAME_SIZE, cWidth, cHeight)));
        REQUIRE(SUCCEEDED(MFSetAttributeRatio(output.Get(), MF_MT_FRAME_RATE, 10, 1)));
        REQUIRE(SUCCEEDED(MFSetAttributeRatio(output.Get(), MF_MT_PIXEL_ASPECT_RATIO, 1, 1)));
        DWORD stream{};
        REQUIRE(SUCCEEDED(writer->AddStream(output.Get(), &stream)));
        ComPtr<IMFMediaType> input;
        REQUIRE(SUCCEEDED(MFCreateMediaType(&input)));
        REQUIRE(SUCCEEDED(input->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video)));
        REQUIRE(SUCCEEDED(input->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_NV12)));
        REQUIRE(SUCCEEDED(input->SetUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive)));
        REQUIRE(SUCCEEDED(MFSetAttributeSize(input.Get(), MF_MT_FRAME_SIZE, cWidth, cHeight)));
        REQUIRE(SUCCEEDED(MFSetAttributeRatio(input.Get(), MF_MT_FRAME_RATE, 10, 1)));
        REQUIRE(SUCCEEDED(MFSetAttributeRatio(input.Get(), MF_MT_PIXEL_ASPECT_RATIO, 1, 1)));
        const HRESULT inputResult = writer->SetInputMediaType(stream, input.Get(), nullptr);
        INFO("encoder input HRESULT: " << std::hex << inputResult);
        REQUIRE(SUCCEEDED(inputResult));
        REQUIRE(SUCCEEDED(writer->BeginWriting()));
        for (int frame = 0; frame < 10; ++frame)
        {
            ComPtr<IMFMediaBuffer> buffer;
            REQUIRE(SUCCEEDED(MFCreateMemoryBuffer(cWidth * cHeight * 3 / 2, &buffer)));
            BYTE* pixels{};
            REQUIRE(SUCCEEDED(buffer->Lock(&pixels, nullptr, nullptr)));
            std::memset(pixels, 32 + frame * 20, cWidth * cHeight);
            std::memset(pixels + cWidth * cHeight, 128, cWidth * cHeight / 2);
            REQUIRE(SUCCEEDED(buffer->Unlock()));
            REQUIRE(SUCCEEDED(buffer->SetCurrentLength(cWidth * cHeight * 3 / 2)));
            ComPtr<IMFSample> sample;
            REQUIRE(SUCCEEDED(MFCreateSample(&sample)));
            REQUIRE(SUCCEEDED(sample->AddBuffer(buffer.Get())));
            REQUIRE(SUCCEEDED(sample->SetSampleTime(frame * 1000000LL)));
            REQUIRE(SUCCEEDED(sample->SetSampleDuration(1000000)));
            REQUIRE(SUCCEEDED(writer->WriteSample(stream, sample.Get())));
        }
        REQUIRE(SUCCEEDED(writer->Finalize()));
        return file;
    }

    std::filesystem::path Directory;
    ComPtr<ID3D11Device> Device;
    ComPtr<ID3D11DeviceContext> Context;
};
} // namespace

TEST_CASE("Windows Main Menu media transfers frames and reaches EOS or loops", "[.][main-menu-media]")
{
    MediaFixture fixture;
    const auto file = fixture.MakeVideo();
    const bool loop = GENERATE(false, true);
    VideoPlayer player;
    REQUIRE(player.Open(fixture.Device.Get(), file, loop, false));
    const auto start = std::chrono::steady_clock::now();
    unsigned frames{};
    Playback result = Playback::Pending;
    while (std::chrono::steady_clock::now() - start < std::chrono::seconds(loop ? 3 : 10))
    {
        result = player.Update();
        INFO("HRESULT: " << std::hex << player.Error());
        REQUIRE(result != Playback::Failed);
        if (result == Playback::Frame)
        {
            ++frames;
            REQUIRE(player.Texture() != nullptr);
            REQUIRE(player.Width() == cWidth);
            REQUIRE(player.Height() == cHeight);
        }
        if (result == Playback::Ended)
            break;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    REQUIRE(frames > 1);
    REQUIRE((result == Playback::Ended) == !loop);
    player.Stop();
    REQUIRE(player.Texture() == nullptr);
}

TEST_CASE("Windows Main Menu missing and corrupt media fail and pending open cancels safely", "[.][main-menu-media]")
{
    MediaFixture fixture;
    VideoPlayer player;
    REQUIRE_FALSE(player.Open(fixture.Device.Get(), fixture.Directory / "absent.mp4", false, true));
    REQUIRE(player.Texture() == nullptr);
    const auto corrupt = fixture.Directory / "corrupt.mp4";
    std::ofstream(corrupt, std::ios::binary) << "not a video";
    (void)player.Open(fixture.Device.Get(), corrupt, false, true);
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(8);
    Playback result = Playback::Pending;
    while (std::chrono::steady_clock::now() < deadline && result != Playback::Failed)
    {
        result = player.Update();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    REQUIRE(result == Playback::Failed);
    const auto good = fixture.MakeVideo();
    for (int i = 0; i < 5; ++i)
    {
        REQUIRE(player.Open(fixture.Device.Get(), good, false, false));
        player.Stop();
        REQUIRE(player.Texture() == nullptr);
    }
}
#endif
