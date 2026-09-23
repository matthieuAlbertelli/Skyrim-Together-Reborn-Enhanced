#pragma once

#include "PresentationPolicy.h"

#include <filesystem>
#include <memory>

struct ID3D11Device;
struct ID3D11ShaderResourceView;

namespace STRE::MainMenu
{
// Windows Media Foundation frame-server adapter. No Skyrim/ImGui/network types.
// All methods run on the rendering thread; MF callbacks publish atomics only.
class VideoPlayer
{
public:
    VideoPlayer();
    ~VideoPlayer();
    VideoPlayer(const VideoPlayer&) = delete;
    VideoPlayer& operator=(const VideoPlayer&) = delete;

    [[nodiscard]] bool Open(ID3D11Device* apDevice, const std::filesystem::path& aFile, bool aLoop, bool aAudio);
    [[nodiscard]] Playback Update();
    void Stop();
    [[nodiscard]] ID3D11ShaderResourceView* Texture() const noexcept;
    [[nodiscard]] unsigned Width() const noexcept;
    [[nodiscard]] unsigned Height() const noexcept;
    [[nodiscard]] long Error() const noexcept;
    [[nodiscard]] bool AudioFailed() const noexcept;

private:
    struct Detail;
    std::unique_ptr<Detail> m_detail;
};
} // namespace STRE::MainMenu
