#include <TiltedOnlinePCH.h>

#include "DialogueProgressRuntime.h"
#include <Dialogue/ProgressGate.h>
#include <Forms/TESForm.h>
#include <Games/TES.h>
#include <Interface/IMenu.h>
#include <Interface/UI.h>
#include <Games/Skyrim/MainMenuRuntime.h>

#include <cstring>
#include <mutex>

namespace DialogueProgressRuntime
{
namespace
{
using MainMenuRuntime::Detail::Readable;
using namespace STRE::Dialogue;

// AE-only ABI views. Sources and relocation IDs are recorded in the feature
// contract. Never retain or dereference these borrowed pointers across frames.
struct TopicManagerView
{
    std::byte Prefix[0x38];
    void* LastSelectedDialogue;
    std::byte CriticalSection[0x28];
    std::uint32_t Speaker, LastSpeaker;
    TESForm* CurrentTopicInfo;
};
static_assert(offsetof(TopicManagerView, CurrentTopicInfo) == 0x70);
struct SourceFiles
{
    Mod** Data;
    std::uint32_t Size;
    std::uint32_t Pad;
};
static_assert(sizeof(SourceFiles) == 0x10);
static_assert(offsetof(TESForm, unk4) == 0x08);
struct Value
{
    void* Interface{};
    std::uint32_t Type{}, Pad{};
    union
    {
        void* Object{};
        bool Boolean;
    };
};
static_assert(sizeof(Value) == 0x18);
using ReleaseValue = void (*)(void*, Value*, void*);
using AdvanceMovie = void (*)(IMenu*, float, std::uint32_t);
using ProcessMessage = UI_MESSAGE_RESULTS (*)(IMenu*, UIMessage&);
ReleaseValue s_releaseValue{};
AdvanceMovie s_advance{};
ProcessMessage s_message{};
TopicManagerView** s_topicManager{};
std::recursive_mutex s_mutex;
ProgressGate s_gate;
std::uint64_t s_session{};
std::uint32_t s_logCount{};
constexpr std::uint32_t cLogLimit = 128; // temporary diagnostic budget per process
constexpr char cPath[] = "DialogueMenu_mc.bAllowProgress";

template <class T> T Method(void* apObject, std::size_t aSlot)
{
    if (!Readable(apObject, sizeof(void*)))
        return nullptr;
    auto** table = *static_cast<void***>(apObject);
    if (!Readable(table, (aSlot + 1) * sizeof(void*)) || !Readable(table[aSlot], 1, true))
        return nullptr;
    return reinterpret_cast<T>(table[aSlot]);
}

std::optional<bool> ReadGate(void* apMovie)
{
    const auto get = Method<bool (*)(void*, Value*, const char*)>(apMovie, 0x11);
    if (!get)
        return std::nullopt;
    Value value;
    const bool found = get(apMovie, &value, cPath);
    const auto result = found && value.Type == 2 ? std::optional<bool>(value.Boolean) : std::nullopt;
    // Reject non-Boolean values, but release a managed result if an unexpected
    // SWF returned one. No leaked GFx reference and no coercion/variable creation.
    if (value.Type & 0x40)
        s_releaseValue(value.Interface, &value, value.Object);
    return result;
}

Observation Observe(IMenu* apMenu)
{
    Observation result;
    auto* ui = UI::Get();
    static BSFixedString s_name{"Dialogue Menu"};
    if (!ui || !Readable(apMenu, sizeof(IMenu)) || !ui->GetMenuOpen(s_name) || ui->FindMenuByName(s_name) != apMenu || !apMenu->uiMovie)
        return result;
    if (!Readable(s_topicManager, sizeof(void*)) || !Readable(*s_topicManager, sizeof(TopicManagerView)))
        return result;
    const auto* manager = *s_topicManager;
    const auto* info = manager->CurrentTopicInfo;
    // INFO is form type 0x4C. Temporary and vanilla-origin records are excluded.
    if (!Readable(info, sizeof(TESForm)) || static_cast<std::uint8_t>(info->formType) != 0x4C || info->IsTemporary())
        return result;
    const auto* files = reinterpret_cast<const SourceFiles*>(info->unk4);
    if (!Readable(files, sizeof(SourceFiles)) || !files->Size || files->Size > 4096 || !Readable(files->Data, files->Size * sizeof(Mod*)))
        return result;
    const auto* origin = files->Data[0];
    const auto* winner = files->Data[files->Size - 1];
    if (!Readable(origin, sizeof(Mod)) || !Readable(winner, sizeof(Mod)) || strnlen_s(origin->filename, sizeof(origin->filename)) == sizeof(origin->filename) ||
        strnlen_s(winner->filename, sizeof(winner->filename)) == sizeof(winner->filename) || _stricmp(origin->filename, cPlugin.data()) != 0 ||
        _stricmp(winner->filename, cPlugin.data()) != 0 || !origin->IsLoaded() || origin->IsLite() || (info->formID >> 24) != origin->standardId)
        return result;
    std::uintptr_t response{};
    // An additional segment token resets the deadline during automatic response
    // changes. It is optional for greetings without lastSelectedDialogue.
    if (Readable(manager->LastSelectedDialogue, 0x48))
    {
        const auto* bytes = static_cast<const std::byte*>(manager->LastSelectedDialogue);
        TESForm* parent{};
        std::memcpy(&parent, bytes + 0x30, sizeof(parent));
        if (parent == info)
            std::memcpy(&response, bytes + 0x40, sizeof(response));
    }
    result.Key = {
        reinterpret_cast<std::uintptr_t>(apMenu),
        reinterpret_cast<std::uintptr_t>(apMenu->uiMovie),
        reinterpret_cast<std::uintptr_t>(info),
        response,
        info->formID,
        manager->Speaker,
        s_session};
    result.Plugin = cPlugin;
    result.MenuOpen = true;
    result.AllowProgress = ReadGate(apMenu->uiMovie);
    return result;
}

void AdvanceHook(IMenu* apMenu, float aInterval, std::uint32_t aTime)
{
    s_advance(apMenu, aInterval, aTime);
    // Run on the native menu/movie update path. Session invalidation and writes
    // are serialized; no delayed task captures a menu, INFO or movie pointer.
    const std::lock_guard lock(s_mutex);
    const auto sample = Observe(apMenu);
    const auto action = s_gate.Observe(sample, ProgressGate::Clock::now());
    if (action == GateAction::Arm && s_logCount < cLogLimit)
    {
        ++s_logCount;
        spdlog::info("[STRE][DialogueProgress] INFO={:08X} plugin={} allowProgress=false arm delayMs=750 session={}", sample.Key.FormId, cPlugin, s_session);
    }
    if (action != GateAction::Enable)
        return;
    const auto current = Observe(apMenu);
    if (current.Key != sample.Key || current.Plugin != cPlugin || current.AllowProgress != std::optional<bool>(false))
    {
        s_gate.Reset();
        return;
    }
    const auto set = Method<bool (*)(void*, const char*, const Value&, std::uint32_t)>(apMenu->uiMovie, 0x10);
    Value enabled;
    enabled.Type = 2;
    enabled.Boolean = true;
    // kNormal (0): no queued sticky write that could affect a later menu.
    const bool success = set && set(apMenu->uiMovie, cPath, enabled, 0) && ReadGate(apMenu->uiMovie) == std::optional<bool>(true);
    if (success)
        s_gate.ConfirmEnabled();
    if (s_logCount < cLogLimit)
    {
        ++s_logCount;
        spdlog::info("[STRE][DialogueProgress] INFO={:08X} plugin={} allowProgress={} write={}", sample.Key.FormId, cPlugin, success ? "true" : "unconfirmed", success);
    }
}

UI_MESSAGE_RESULTS MessageHook(IMenu* apMenu, UIMessage& aMessage)
{
    if (aMessage.eType == UIMessage::kHide || aMessage.eType == UIMessage::kForceHide || aMessage.eType == UIMessage::kShow)
    {
        const std::lock_guard lock(s_mutex);
        s_gate.Reset();
    }
    return s_message(apMenu, aMessage); // every input/message follows vanilla
}

bool Install()
{
    auto& db = VersionDb::Get();
    if (db.GetLoadedVersionString() != "1.6.1170.0")
        return false;
    auto** table = static_cast<void**>(db.FindAddressById(215255));
    s_topicManager = static_cast<TopicManagerView**>(db.FindAddressById(401099));
    s_releaseValue = reinterpret_cast<ReleaseValue>(db.FindAddressById(82270));
    if (!Readable(table, 6 * sizeof(void*)) || !Readable(table[4], 1, true) || !Readable(table[5], 1, true) || !Readable(s_topicManager, sizeof(void*)) ||
        !Readable(reinterpret_cast<void*>(s_releaseValue), 1, true))
        return false;
    DWORD protection{};
    if (!VirtualProtect(table + 4, 2 * sizeof(void*), PAGE_EXECUTE_READWRITE, &protection))
        return false;
    s_message = reinterpret_cast<ProcessMessage>(table[4]);
    s_advance = reinterpret_cast<AdvanceMovie>(table[5]);
    table[4] = reinterpret_cast<void*>(&MessageHook);
    table[5] = reinterpret_cast<void*>(&AdvanceHook);
    DWORD ignored{};
    VirtualProtect(table + 4, 2 * sizeof(void*), protection, &ignored);
    return true;
}

TiltedPhoques::Initializer s_install(
    []() { spdlog::info("[STRE][DialogueProgress] installed={} runtime={} delayMs=750 diagnosticLimit={}", Install(), VersionDb::Get().GetLoadedVersionString(), cLogLimit); });
} // namespace

void ResetSession() noexcept
{
    const std::lock_guard lock(s_mutex);
    ++s_session;
    s_gate.Reset();
}
} // namespace DialogueProgressRuntime
