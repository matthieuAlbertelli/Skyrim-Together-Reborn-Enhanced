#include "VideoPlayer.h"

#include <Windows.h>
#include <d3d11.h>
#include <d3d10.h>
#include <mfapi.h>
#include <mferror.h>
#include <mfmediaengine.h>
#include <wrl/client.h>

#include <atomic>
#include <new>

namespace STRE::MainMenu
{
using Microsoft::WRL::ComPtr;

namespace
{
// Independently ref-counted callback state: shutdown/late events cannot reach
// a destroyed VideoPlayer, Skyrim pointer, or an immediate D3D context.
class Notifications final : public IMFMediaEngineNotify
{
public:
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID aId, void** appResult) override
    {
        if (!appResult)
            return E_POINTER;
        *appResult = nullptr;
        if (aId != __uuidof(IUnknown) && aId != __uuidof(IMFMediaEngineNotify))
            return E_NOINTERFACE;
        *appResult = static_cast<IMFMediaEngineNotify*>(this);
        AddRef();
        return S_OK;
    }
    ULONG STDMETHODCALLTYPE AddRef() override { return ++m_refs; }
    ULONG STDMETHODCALLTYPE Release() override
    {
        const ULONG count = --m_refs;
        if (!count)
            delete this;
        return count;
    }
    HRESULT STDMETHODCALLTYPE EventNotify(DWORD aEvent, DWORD_PTR aParam1, DWORD aParam2) override
    {
        switch (aEvent)
        {
        case MF_MEDIA_ENGINE_EVENT_CANPLAY: Ready = true; break;
        case MF_MEDIA_ENGINE_EVENT_ENDED: Ended = true; break;
        case MF_MEDIA_ENGINE_EVENT_ERROR: Error = aParam2 ? static_cast<HRESULT>(aParam2) : E_FAIL; break;
        case MF_MEDIA_ENGINE_EVENT_STREAMRENDERINGERROR:
            // Microsoft does not specify parameters for this event. Do not
            // guess the failed stream. Mute audio and let the frame watchdog
            // determine whether the surviving stream is usable video.
            AudioError = true;
            break;
        case MF_MEDIA_ENGINE_EVENT_NOTIFYSTABLESTATE: SetEvent(reinterpret_cast<HANDLE>(aParam1)); break;
        default: break;
        }
        return S_OK;
    }

    std::atomic<bool> Ready{};
    std::atomic<bool> Ended{};
    std::atomic<bool> AudioError{};
    std::atomic<HRESULT> Error{S_OK};

private:
    std::atomic<ULONG> m_refs{1};
};
} // namespace

struct VideoPlayer::Detail
{
    ~Detail()
    {
        Close();
        DeviceManager.Reset();
        Device.Reset();
        if (Started && Shutdown)
            Shutdown();
        if (Platform)
            FreeLibrary(Platform);
        if (OwnApartment && GetCurrentThreadId() == ApartmentThread)
            CoUninitialize();
    }

    bool Initialize(ID3D11Device* apDevice)
    {
        if (Started)
            return true;
        const HRESULT apartment = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
        OwnApartment = SUCCEEDED(apartment);
        ApartmentThread = GetCurrentThreadId();
        if (FAILED(apartment) && apartment != RPC_E_CHANGED_MODE)
            return Fail(apartment);

        // Optional OS component: never make missing Media Foundation a loader
        // failure that prevents the Skyrim process from starting.
        Platform = LoadLibraryExW(L"mfplat.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
        if (!Platform)
            return Fail(HRESULT_FROM_WIN32(GetLastError()));
        const auto startup = reinterpret_cast<decltype(&MFStartup)>(GetProcAddress(Platform, "MFStartup"));
        Shutdown = reinterpret_cast<decltype(&MFShutdown)>(GetProcAddress(Platform, "MFShutdown"));
        CreateAttributes = reinterpret_cast<decltype(&MFCreateAttributes)>(GetProcAddress(Platform, "MFCreateAttributes"));
        const auto createManager = reinterpret_cast<decltype(&MFCreateDXGIDeviceManager)>(GetProcAddress(Platform, "MFCreateDXGIDeviceManager"));
        if (!startup || !Shutdown || !CreateAttributes || !createManager)
            return Fail(E_NOINTERFACE);
        HRESULT hr = startup(MF_VERSION, MFSTARTUP_FULL);
        if (FAILED(hr))
            return Fail(hr);
        Started = true;
        UINT token{};
        hr = createManager(&token, &DeviceManager);
        if (FAILED(hr))
            return Fail(hr);
        hr = DeviceManager->ResetDevice(apDevice, token);
        if (FAILED(hr))
            return Fail(hr);
        ComPtr<ID3D10Multithread> multithread;
        hr = apDevice->QueryInterface(IID_PPV_ARGS(&multithread));
        if (FAILED(hr))
            return Fail(hr);
        // MF can use the shared device from its internal decode workers.
        multithread->SetMultithreadProtected(TRUE);
        Device = apDevice;
        return true;
    }

    bool Fail(HRESULT aError)
    {
        LastError = aError;
        return false;
    }

    void Close()
    {
        if (Engine)
        {
            Engine->SetMuted(TRUE);
            Engine->Pause();
            Engine->Shutdown();
        }
        Engine.Reset();
        Events.Reset();
        View.Reset();
        Texture.Reset();
        Width = Height = 0;
        Playing = HasFrame = AudioError = false;
    }

    HMODULE Platform{};
    decltype(&MFShutdown) Shutdown{};
    decltype(&MFCreateAttributes) CreateAttributes{};
    bool Started{};
    bool OwnApartment{};
    DWORD ApartmentThread{};
    ComPtr<ID3D11Device> Device;
    ComPtr<IMFDXGIDeviceManager> DeviceManager;
    ComPtr<IMFMediaEngine> Engine;
    ComPtr<Notifications> Events;
    ComPtr<ID3D11Texture2D> Texture;
    ComPtr<ID3D11ShaderResourceView> View;
    DWORD Width{};
    DWORD Height{};
    HRESULT LastError{S_OK};
    bool Playing{};
    bool HasFrame{};
    bool AudioError{};
};

VideoPlayer::VideoPlayer() = default;
VideoPlayer::~VideoPlayer() = default;

bool VideoPlayer::Open(ID3D11Device* apDevice, const std::filesystem::path& aFile, bool aLoop, bool aAudio)
{
    Stop();
    // Fresh initialization also makes failure terminal for this attempt, with
    // no half-initialized MF platform reused on the next clip.
    m_detail = std::make_unique<Detail>();
    auto& d = *m_detail;
    std::error_code ec;
    if (!apDevice || !std::filesystem::is_regular_file(aFile, ec) || ec)
        return d.Fail(HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND));
    if (!d.Initialize(apDevice))
        return false;
    ComPtr<IMFMediaEngineClassFactory> factory;
    HRESULT hr = CoCreateInstance(CLSID_MFMediaEngineClassFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&factory));
    if (FAILED(hr))
        return d.Fail(hr);
    ComPtr<IMFAttributes> attributes;
    hr = d.CreateAttributes(&attributes, 3);
    if (FAILED(hr))
        return d.Fail(hr);
    d.Events.Attach(new (std::nothrow) Notifications);
    if (!d.Events)
        return d.Fail(E_OUTOFMEMORY);
    if (FAILED(hr = attributes->SetUnknown(MF_MEDIA_ENGINE_CALLBACK, d.Events.Get())) || FAILED(hr = attributes->SetUnknown(MF_MEDIA_ENGINE_DXGI_MANAGER, d.DeviceManager.Get())) ||
        FAILED(hr = attributes->SetUINT32(MF_MEDIA_ENGINE_VIDEO_OUTPUT_FORMAT, DXGI_FORMAT_B8G8R8A8_UNORM)))
        return d.Fail(hr);
    const DWORD flags = MF_MEDIA_ENGINE_DISABLE_LOCAL_PLUGINS | (aAudio ? 0 : MF_MEDIA_ENGINE_FORCEMUTE);
    hr = factory->CreateInstance(flags, attributes.Get(), &d.Engine);
    if (FAILED(hr))
        return d.Fail(hr);
    if (FAILED(hr = d.Engine->SetLoop(aLoop)) || FAILED(hr = d.Engine->SetAutoPlay(FALSE)))
        return d.Fail(hr);
    if (FAILED(d.Engine->SetMuted(!aAudio)))
        d.AudioError = true;
    const auto absolute = std::filesystem::absolute(aFile, ec);
    if (ec)
        return d.Fail(E_INVALIDARG);
    BSTR source = SysAllocString(absolute.c_str());
    if (!source)
        return d.Fail(E_OUTOFMEMORY);
    hr = d.Engine->SetSource(source);
    SysFreeString(source);
    if (FAILED(hr))
        return d.Fail(hr);
    return true;
}

Playback VideoPlayer::Update()
{
    if (!m_detail || !m_detail->Engine || FAILED(m_detail->LastError))
        return Playback::Failed;
    auto& d = *m_detail;
    const HRESULT error = d.Events->Error.load();
    if (FAILED(error))
    {
        d.Fail(error);
        return Playback::Failed;
    }
    if (d.Events->Ended)
        return Playback::Ended;
    if (d.Events->AudioError.exchange(false))
    {
        d.AudioError = true;
        d.Engine->SetMuted(TRUE);
    }
    if (!d.Events->Ready)
        return Playback::Pending;
    HRESULT hr = S_OK;
    if (!d.Playing)
    {
        if (!d.Engine->HasVideo() || FAILED(hr = d.Engine->GetNativeVideoSize(&d.Width, &d.Height)) || !d.Width || !d.Height || d.Width > 4096 || d.Height > 4096 ||
            static_cast<std::uint64_t>(d.Width) * d.Height > 8847360)
        {
            d.Fail(FAILED(hr) ? hr : MF_E_INVALIDMEDIATYPE);
            return Playback::Failed;
        }
        D3D11_TEXTURE2D_DESC desc{};
        desc.Width = d.Width;
        desc.Height = d.Height;
        desc.MipLevels = desc.ArraySize = 1;
        desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
        desc.SampleDesc.Count = 1;
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
        if (FAILED(hr = d.Device->CreateTexture2D(&desc, nullptr, &d.Texture)) || FAILED(hr = d.Device->CreateShaderResourceView(d.Texture.Get(), nullptr, &d.View)) ||
            FAILED(hr = d.Engine->Play()))
        {
            d.Fail(hr);
            return Playback::Failed;
        }
        d.Playing = true;
    }
    LONGLONG timestamp{};
    hr = d.Engine->OnVideoStreamTick(&timestamp);
    if (hr == S_FALSE)
        return Playback::Pending;
    if (SUCCEEDED(hr))
    {
        const RECT destination{0, 0, static_cast<LONG>(d.Width), static_cast<LONG>(d.Height)};
        const MFARGB black{0, 0, 0, 255};
        hr = d.Engine->TransferVideoFrame(d.Texture.Get(), nullptr, &destination, &black);
    }
    if (FAILED(hr))
    {
        d.Fail(hr);
        return Playback::Failed;
    }
    d.HasFrame = true;
    return Playback::Frame;
}

void VideoPlayer::Stop()
{
    m_detail.reset();
}
ID3D11ShaderResourceView* VideoPlayer::Texture() const noexcept
{
    return m_detail && m_detail->HasFrame ? m_detail->View.Get() : nullptr;
}
unsigned VideoPlayer::Width() const noexcept
{
    return m_detail ? m_detail->Width : 0;
}
unsigned VideoPlayer::Height() const noexcept
{
    return m_detail ? m_detail->Height : 0;
}
long VideoPlayer::Error() const noexcept
{
    return m_detail ? m_detail->LastError : S_OK;
}
bool VideoPlayer::AudioFailed() const noexcept
{
    return m_detail && m_detail->AudioError;
}
} // namespace STRE::MainMenu
