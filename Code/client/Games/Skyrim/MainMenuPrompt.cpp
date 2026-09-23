#include <TiltedOnlinePCH.h>

#include "MainMenuPrompt.h"
#include "MainMenuRuntime.h"
#include <MainMenu/Branding.h>

#include <algorithm>
#include <array>
#include <cmath>

namespace MainMenuRuntime
{
namespace
{
// CommonLibSSE-NG public GFxMovie/GFxMovieView/GFxValue and input ABIs.
// These calls are optional, fenced to 1.6.1170 and never patch another hook.
using Detail::Readable;

template <class T> T Method(void* apObject, std::size_t aSlot)
{
    return reinterpret_cast<T>((*static_cast<void***>(apObject))[aSlot]);
}

bool HasMethods(void* apObject, std::initializer_list<std::size_t> aSlots)
{
    if (!Readable(apObject, sizeof(void*)))
        return false;
    auto** table = *static_cast<void***>(apObject);
    for (auto slot : aSlots)
        if (!Readable(table, (slot + 1) * sizeof(void*)) || !Readable(table[slot], 1, true))
            return false;
    return true;
}

struct Value
{
    void* Interface{};
    std::uint32_t Type{};
    std::uint32_t Pad{};
    union
    {
        double Number{};
        const char* String;
        void* Object;
        bool Boolean;
    };

    Value() = default;
    explicit Value(double aNumber)
        : Type(3)
        , Number(aNumber)
    {
    }
    explicit Value(const char* apString)
        : Type(4)
        , String(apString)
    {
    }
    explicit Value(bool aBoolean)
        : Type(2)
        , Boolean(aBoolean)
    {
    }
};
static_assert(sizeof(Value) == 0x18);

using ReleaseValue = void (*)(void*, Value*, void*);
using VisitMembers = void (*)(void*, void*, void*, bool);
ReleaseValue s_releaseValue{};
VisitMembers s_visitMembers{};

void Release(Value& aValue)
{
    if (aValue.Type & 0x40)
        s_releaseValue(aValue.Interface, &aValue, aValue.Object);
    aValue = Value{};
}

bool Set(void* apMovie, const char* apPath, const Value& aValue)
{
    return Method<bool (*)(void*, const char*, const Value&, std::uint32_t)>(apMovie, 0x10)(apMovie, apPath, aValue, 0);
}

bool Get(void* apMovie, const char* apPath, Value& aValue)
{
    return Method<bool (*)(void*, Value*, const char*)>(apMovie, 0x11)(apMovie, &aValue, apPath);
}

bool Invoke(void* apMovie, const char* apMethod, std::initializer_list<Value> aArguments)
{
    // Ignore return values so attachMovie/createTextField cannot leak managed
    // references. The required resulting paths are checked separately.
    return Method<bool (*)(void*, const char*, Value*, const Value*, std::uint32_t)>(apMovie, 0x17)(
        apMovie, apMethod, nullptr, aArguments.begin(), static_cast<std::uint32_t>(aArguments.size()));
}

double Number(void* apMovie, const char* apPath)
{
    Value value;
    const bool found = Get(apMovie, apPath, value);
    const double result = found && value.Type == 3 && std::isfinite(value.Number) ? value.Number : 0.0;
    Release(value);
    return result;
}

bool FormatText(void* apMovie, const char* apPath, const char* apText, double aSize, double aColor)
{
    const std::array<Value, 3> formatArgs{Value("$EverywhereMediumFont"), Value(aSize), Value(aColor)};
    Value format;
    Method<void (*)(void*, Value*, const char*, const Value*, std::uint32_t)>(apMovie, 0x0d)(
        apMovie, &format, "TextFormat", formatArgs.data(), static_cast<std::uint32_t>(formatArgs.size()));
    const bool formatted = (format.Type & 0x0f) == 6 && Invoke(apMovie, (std::string(apPath) + ".setNewTextFormat").c_str(), {format}) &&
                           Set(apMovie, (std::string(apPath) + ".text").c_str(), Value(apText)) && Invoke(apMovie, (std::string(apPath) + ".setTextFormat").c_str(), {format});
    Release(format);
    return formatted;
}

struct Viewport
{
    std::int32_t BufferWidth, BufferHeight, Left, Top, Width, Height;
    std::int32_t ScissorLeft, ScissorTop, ScissorWidth, ScissorHeight;
    float Scale, AspectRatio;
    std::uint32_t Flags, Pad;
};
static_assert(sizeof(Viewport) == 0x38);

// sharedcomponents contains quantity-menu sample clips as well as exported
// artwork. Hide existing root children before adding our own inert prompt.
// Collect first: do not mutate the member table while Scaleform enumerates it.
struct RootChildren
{
    virtual ~RootChildren() = default;
    virtual void Visit(const char* apName, const Value& aValue)
    {
        if ((aValue.Type & 0x0f) != 8 || !apName)
            return;
        const auto length = strnlen_s(apName, 128);
        const std::string_view name(apName, length);
        if (name == "_root" || name == "_parent" || name.starts_with("_level"))
            return;
        const bool identifier = std::all_of(
            name.begin(), name.end(), [](unsigned char c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_' || c == '$'; });
        if (name.empty() || length == 128 || Names.size() == 128 || !identifier)
        {
            Valid = false;
            return;
        }
        Names.emplace_back(apName, length);
    }
    std::vector<std::string> Names;
    bool Valid{true};
};
} // namespace

PresentationMovie::PresentationMovie()
{
    uiMenuFlags = kNone;
    eInputContext = 0;
}

PresentationMovie::~PresentationMovie()
{
    ReleaseMovie();
}

void PresentationMovie::ReleaseMovie()
{
    if (uiMovie)
    {
        if (HasMethods(uiMovie, {0x45}))
            Method<void (*)(void*)>(uiMovie, 0x45)(uiMovie);
        uiMovie = nullptr;
    }
    // Same delegate ownership as the existing TradePreviewHostMenu.
    if (auto* delegate = static_cast<GRefCountImpl*>(fxDelegate))
    {
        fxDelegate = nullptr;
        auto* count = reinterpret_cast<std::uint32_t*>(reinterpret_cast<std::byte*>(delegate) + 0x8);
        if (std::atomic_ref<std::uint32_t>(*count).fetch_sub(1, std::memory_order_acq_rel) == 1)
            delete delegate;
    }
}

void IntroPrompt::Fail()
{
    spdlog::warn("[STRE][MainMenu] hint unavailable; playback and skip remain active");
    ReleaseMovie();
}

bool PresentationMovie::LoadLibrary()
{
    auto& db = VersionDb::Get();
    if (db.GetLoadedVersionString() != "1.6.1170.0")
        return false;
    auto** manager = static_cast<void**>(db.FindAddressById(402775));
    using LoadMovie = bool (*)(void*, IMenu*, void**, const char*, std::uint32_t, float);
    auto load = reinterpret_cast<LoadMovie>(db.FindAddressById(82325));
    s_releaseValue = reinterpret_cast<ReleaseValue>(db.FindAddressById(82270));
    s_visitMembers = reinterpret_cast<VisitMembers>(db.FindAddressById(82302));
    if (!Readable(manager, sizeof(void*)) || !Readable(*manager, 0x40) || !Readable(reinterpret_cast<const void*>(load), 1, true) ||
        !Readable(reinterpret_cast<const void*>(s_releaseValue), 1, true) || !Readable(reinterpret_cast<const void*>(s_visitMembers), 1, true))
        return false;
    // Load the player's installed library, not a copied Bethesda asset. No
    // StartMenu instance, menu registration, control context or callback action.
    if (!load(*manager, this, &uiMovie, "sharedcomponents", 0, 0.0f) || !HasMethods(uiMovie, {0x0d, 0x10, 0x11, 0x17, 0x19, 0x1d, 0x25, 0x26, 0x45}))
        return false;
    Value root;
    if (!Get(uiMovie, "_root", root) || (root.Type & 0x0f) != 8 || !root.Interface)
    {
        Release(root);
        return false;
    }
    RootChildren children;
    s_visitMembers(root.Interface, root.Object, &children, true);
    Release(root);
    if (!children.Valid || children.Names.empty())
        return false;
    for (const auto& child : children.Names)
        if (!Set(uiMovie, ("_root." + child + "._visible").c_str(), Value(false)))
            return false;

    Method<void (*)(void*, std::uint32_t)>(uiMovie, 0x1d)(uiMovie, 5); // top-left, no scale
    return true;
}

IntroPrompt::IntroPrompt(std::uint32_t aScanCode, std::string aAction)
    : m_scanCode(aScanCode)
    , m_action(std::move(aAction))
{
}

bool IntroPrompt::Initialize()
{
    if (m_action.empty() || VersionDb::Get().GetLoadedVersionString() != "1.6.1170.0")
        return false;
    auto** input = static_cast<void**>(VersionDb::Get().FindAddressById(402776));
    if (!Readable(input, sizeof(void*)) || !Readable(*input, 0x68))
        return false;
    void* keyboard = *reinterpret_cast<void**>(static_cast<std::byte*>(*input) + 0x60);
    if (!HasMethods(keyboard, {4}))
        return false;
    BSFixedString art;
    if (!Method<bool (*)(void*, std::uint32_t, BSFixedString&)>(keyboard, 4)(keyboard, m_scanCode, art) || !art.data || !Readable(art.data, 128) || strnlen_s(art.data, 128) == 128)
        return false;
    if (!LoadLibrary())
        return false;
    if (!Invoke(uiMovie, "_root.createEmptyMovieClip", {Value("STREIntroPrompt"), Value(16384.0)}) ||
        !Invoke(uiMovie, "_root.STREIntroPrompt.attachMovie", {Value(art.data), Value("Key"), Value(1.0)}) ||
        !Invoke(uiMovie, "_root.STREIntroPrompt.createTextField", {Value("Action"), Value(2.0), Value(0.0), Value(0.0), Value(1.0), Value(1.0)}))
        return false;
    Method<float (*)(void*, float, std::uint32_t)>(uiMovie, 0x25)(uiMovie, 0.0f, 0);
    m_keyHeight = Number(uiMovie, "_root.STREIntroPrompt.Key._height");
    if (Number(uiMovie, "_root.STREIntroPrompt.Key._width") <= 0 || m_keyHeight <= 0)
        return false; // absent art, including unsupported custom bindings
    m_started = GetTickCount64();
    const bool ready = Set(uiMovie, "_root.STREIntroPrompt.Action.selectable", Value(false)) && Set(uiMovie, "_root.STREIntroPrompt.Action.embedFonts", Value(true)) &&
                       Set(uiMovie, "_root.STREIntroPrompt.Action.autoSize", Value("left")) && Set(uiMovie, "_root.STREIntroPrompt.Action.noTranslate", Value(true));
    if (ready)
        spdlog::info("[STRE][MainMenu] hint=native-scaleform key-art={} source=sharedcomponents", art.data);
    return ready;
}

bool IntroPrompt::Layout(int aWidth, int aHeight)
{
    // TextFormat uses Skyrim's font mapping; localized content is plain text,
    // never HTML/ActionScript. A native key sprite supplies its own cartouche.
    const double scale = std::clamp(aHeight / 1080.0, 0.5, 3.0);
    if (!FormatText(uiMovie, "_root.STREIntroPrompt.Action", m_action.c_str(), 24.0 * scale, 0xffffff))
        return false;
    if (!Set(uiMovie, "_root.STREIntroPrompt.Key._xscale", Value(2400.0 * scale / m_keyHeight)) ||
        !Set(uiMovie, "_root.STREIntroPrompt.Key._yscale", Value(2400.0 * scale / m_keyHeight)))
        return false;
    const double keyWidth = Number(uiMovie, "_root.STREIntroPrompt.Key._width");
    const double textWidth = Number(uiMovie, "_root.STREIntroPrompt.Action._width");
    const double textHeight = Number(uiMovie, "_root.STREIntroPrompt.Action._height");
    const double width = keyWidth + 8.0 * scale + textWidth;
    if (keyWidth <= 0 || textWidth <= 0 || textHeight <= 0 || width > aWidth * 0.85)
        return false;
    return Set(uiMovie, "_root.STREIntroPrompt.Action._x", Value(keyWidth + 8.0 * scale)) &&
           Set(uiMovie, "_root.STREIntroPrompt.Key._y", Value((textHeight - 24.0 * scale) * 0.5)) &&
           Set(uiMovie, "_root.STREIntroPrompt._x", Value(aWidth - 40.0 * scale - width)) && Set(uiMovie, "_root.STREIntroPrompt._y", Value(aHeight - 36.0 * scale - textHeight));
}

void IntroPrompt::Render(void* apMainMenuMovie)
{
    if (!m_attempted)
    {
        m_attempted = true;
        if (!Initialize())
        {
            Fail();
            return;
        }
    }
    if (!uiMovie)
        return;
    int width{}, height{};
    if (!CopyViewport(apMainMenuMovie, width, height))
        return;
    if ((m_width != width || m_height != height) && !Layout(width, height))
    {
        Fail();
        return;
    }
    m_width = width;
    m_height = height;
    if (!Set(uiMovie, "_root.STREIntroPrompt._alpha", Value(std::min((GetTickCount64() - m_started) / 350.0, 1.0) * 85.0)))
    {
        Fail();
        return;
    }
    Method<void (*)(void*)>(uiMovie, 0x26)(uiMovie);
}
bool PresentationMovie::CopyViewport(void* apMainMenuMovie, int& aWidth, int& aHeight)
{
    if (!HasMethods(apMainMenuMovie, {0x1a}))
        return false;
    Viewport viewport{};
    Method<void (*)(void*, Viewport*)>(apMainMenuMovie, 0x1a)(apMainMenuMovie, &viewport);
    if (viewport.Width <= 0 || viewport.Height <= 0 || viewport.Width > 16384 || viewport.Height > 16384)
        return false;
    Method<void (*)(void*, const Viewport&)>(uiMovie, 0x19)(uiMovie, viewport);
    aWidth = viewport.Width;
    aHeight = viewport.Height;
    return true;
}

MenuSubtitle::MenuSubtitle(std::string aText)
    : m_text(std::move(aText))
{
}

void MenuSubtitle::Fail()
{
    spdlog::warn("[STRE][MainMenu] subtitle unavailable; background and vanilla remain active");
    ReleaseMovie();
}

bool MenuSubtitle::Layout(int aWidth, int aHeight)
{
    const auto layout = STRE::MainMenu::LayoutBranding(static_cast<float>(aWidth), static_cast<float>(aHeight), 0, 0);
    constexpr auto path = "_root.STRESubtitle";
    if (!FormatText(uiMovie, path, m_text.c_str(), layout.FontSize, 0xdfce9b))
        return false;
    double width = Number(uiMovie, "_root.STRESubtitle._width");
    if (width > layout.SubtitleMaxWidth)
    {
        // Fit longer translations without stretching the font or wrapping.
        if (!FormatText(uiMovie, path, m_text.c_str(), layout.FontSize * layout.SubtitleMaxWidth / width * 0.95, 0xdfce9b))
            return false;
        width = Number(uiMovie, "_root.STRESubtitle._width");
    }
    const double height = Number(uiMovie, "_root.STRESubtitle._height");
    if (width <= 0 || width > layout.SubtitleMaxWidth || height <= 0 || height > aHeight * 0.10)
        return false;
    return Set(uiMovie, "_root.STRESubtitle._x", Value(layout.CenterX - width * 0.5)) && Set(uiMovie, "_root.STRESubtitle._y", Value(double(layout.SubtitleY)));
}

void MenuSubtitle::Render(void* apMainMenuMovie, float aOpacity)
{
    if (m_text.empty() || !std::isfinite(aOpacity) || aOpacity <= 0)
        return;
    if (!m_attempted)
    {
        m_attempted = true;
        if (!LoadLibrary() || !Invoke(uiMovie, "_root.createTextField", {Value("STRESubtitle"), Value(16384.0), Value(0.0), Value(0.0), Value(1.0), Value(1.0)}) ||
            !Set(uiMovie, "_root.STRESubtitle.selectable", Value(false)) || !Set(uiMovie, "_root.STRESubtitle.embedFonts", Value(true)) ||
            !Set(uiMovie, "_root.STRESubtitle.autoSize", Value("left")) || !Set(uiMovie, "_root.STRESubtitle.noTranslate", Value(true)))
        {
            Fail();
            return;
        }
        Method<float (*)(void*, float, std::uint32_t)>(uiMovie, 0x25)(uiMovie, 0.0f, 0);
    }
    if (!uiMovie)
        return;
    int width{}, height{};
    if (!CopyViewport(apMainMenuMovie, width, height))
        return;
    if ((m_width != width || m_height != height) && !Layout(width, height))
    {
        Fail();
        return;
    }
    m_width = width;
    m_height = height;
    if (!Set(uiMovie, "_root.STRESubtitle._alpha", Value(std::min(aOpacity, 1.0f) * 100.0)))
    {
        Fail();
        return;
    }
    Method<void (*)(void*)>(uiMovie, 0x26)(uiMovie);
}
} // namespace MainMenuRuntime
