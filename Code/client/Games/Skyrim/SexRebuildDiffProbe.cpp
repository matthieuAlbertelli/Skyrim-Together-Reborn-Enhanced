#include <TiltedOnlinePCH.h>
#include <SexRebuildDiffProbe.h>
#include <SexChangeProbe.h>
#include <CharacterCreation/SexProbeWindow.h>
#include <Actor.h>
#include <Games/ActorExtension.h>
#include <Games/TES.h>
#include <Forms/TESNPC.h>
#include <Forms/TESRace.h>
#include <Forms/BGSHeadPart.h>
#include <Systems/FaceGenSystem.h>
#include <VersionDb.h>
#include <intrin.h>
#include <array>
#include <mutex>

namespace
{
thread_local SexRebuildDiffScope* Active{};
thread_local Actor* MenuActor{};
struct Identity
{
    uintptr_t Actor{};
    uint32_t Form{}, Server{};
};
std::array<Identity, 256> Identities{};
std::mutex IdentityMutex;
size_t NextIdentity{};
// Exact-size passive reads: unavailable is distinct from a readable null/zero.
template <class T> bool ReadAt(uintptr_t address, T& value) noexcept
{
    T copy{};
    SIZE_T size{};
    if (!address || !ReadProcessMemory(GetCurrentProcess(), reinterpret_cast<void*>(address), &copy, sizeof(copy), &size) || size != sizeof(copy))
        return false;
    value = copy;
    return true;
}
uintptr_t Image() noexcept
{
    return reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr));
}
uintptr_t Global(uint64_t id, bool& known)
{
    uintptr_t value{};
    known = ReadAt(reinterpret_cast<uintptr_t>(VersionDb::Get().FindAddressById(id)), value);
    return value;
}
using NativeActorCall = void(void*, Actor*);
NativeActorCall* RealEnqueue{};
NativeActorCall* RealInline{};
using NativeMenuCall = void(void*);
NativeMenuCall* RealMenu{};
void ObserveBranch(Actor* actor, bool queued, uintptr_t caller) noexcept
{
    try
    {
        if (!Active || Active->Frame.Actor != reinterpret_cast<uintptr_t>(actor) || (!queued && caller != Image() + 0x7270AC))
            return;
        Active->Frame.Observe(reinterpret_cast<uintptr_t>(actor), queued);
        spdlog::info(
            "[STRE][SexRebuildDiff] phase={} actorForm={:X} actorPtr={:X} thread={} sourceCallSeq={} opcode={}", queued ? "task-enqueue" : "inline-AIProcess-enter", actor->formID,
            reinterpret_cast<uintptr_t>(actor), GetCurrentThreadId(), Active->Frame.Sequence, queued ? "0x1C" : "not-applicable");
    }
    catch (...)
    {
    }
}
void HookEnqueue(void* queue, Actor* actor)
{
    const auto error = GetLastError();
    ObserveBranch(actor, true, reinterpret_cast<uintptr_t>(_ReturnAddress()));
    SetLastError(error);
    RealEnqueue(queue, actor);
}
void HookInline(void* process, Actor* actor)
{
    const auto error = GetLastError();
    ObserveBranch(actor, false, reinterpret_cast<uintptr_t>(_ReturnAddress()));
    SetLastError(error);
    RealInline(process, actor);
}
void HookMenu(void* menu)
{
    const auto error = GetLastError();
    struct RestoreMenuContext
    {
        Actor* Previous{MenuActor};
        ~RestoreMenuContext() { MenuActor = Previous; }
    } restore;
    bool known{};
    auto* player = reinterpret_cast<Actor*>(Global(403521, known));
    MenuActor = known ? player : nullptr;
    LogSexRebuildDiff("RaceMenu52391-enter", MenuActor, 0, reinterpret_cast<uintptr_t>(_ReturnAddress()), true, "unresolved");
    SetLastError(error);
    RealMenu(menu);
    const auto outgoing = GetLastError();
    // Re-read the player global: do not dereference a possibly replaced old player.
    bool afterKnown{};
    auto* after = reinterpret_cast<Actor*>(Global(403521, afterKnown));
    if (afterKnown && after == player)
        LogSexRebuildDiff("RaceMenu52391-return", after, 0, 0, true, "unresolved");
    SetLastError(outgoing);
}
template <size_t N> bool Verified(uint64_t id, uintptr_t rva, const std::array<unsigned char, N>& bytes)
{
    auto* address = VersionDb::Get().FindAddressById(id);
    std::array<unsigned char, N> actual{};
    return reinterpret_cast<uintptr_t>(address) == Image() + rva && ReadAt(reinterpret_cast<uintptr_t>(address), actual) && actual == bytes;
}
static TiltedPhoques::Initializer Install(
    []
    {
        if (!STRE::CharacterCreation::SexProbeRuntime(VersionDb::Get().GetLoadedVersionString()))
            return;
        if (Verified(36947, 0x656160, std::array<unsigned char, 7>{0x48, 0x81, 0xEC, 0xA8, 0, 0, 0}))
        {
            RealEnqueue = reinterpret_cast<NativeActorCall*>(VersionDb::Get().FindAddressById(36947));
            TP_HOOK_IMMEDIATE(&RealEnqueue, HookEnqueue);
            spdlog::info("[STRE][SexRebuildDiff] install-attempt relocation=36947 activationRequiresEnterTrace=true");
        }
        else
            spdlog::warn("[STRE][SexRebuildDiff] skipped relocation=36947 reason=runtime-address-or-prologue");
        if (Verified(39395, 0x6E3B70, std::array<unsigned char, 6>{0x40, 0x55, 0x56, 0x57, 0x41, 0x54}))
        {
            RealInline = reinterpret_cast<NativeActorCall*>(VersionDb::Get().FindAddressById(39395));
            TP_HOOK_IMMEDIATE(&RealInline, HookInline);
            spdlog::info("[STRE][SexRebuildDiff] install-attempt relocation=39395 activationRequiresEnterTrace=true");
        }
        else
            spdlog::warn("[STRE][SexRebuildDiff] skipped relocation=39395 reason=runtime-address-or-prologue");
        if (Verified(52391, 0x954960, std::array<unsigned char, 6>{0x40, 0x53, 0x48, 0x83, 0xEC, 0x30}))
        {
            RealMenu = reinterpret_cast<NativeMenuCall*>(VersionDb::Get().FindAddressById(52391));
            TP_HOOK_IMMEDIATE(&RealMenu, HookMenu);
            spdlog::info("[STRE][SexRebuildDiff] install-attempt relocation=52391 activationRequiresEnterTrace=true");
        }
        else
            spdlog::warn("[STRE][SexRebuildDiff] skipped relocation=52391 reason=runtime-address-or-prologue");
    });
} // namespace
SexRebuildDiffScope::SexRebuildDiffScope(Actor* actor, uint64_t sequence) noexcept
    : Frame{reinterpret_cast<uintptr_t>(actor), sequence}
    , Previous(Active)
{
    Active = this;
}
SexRebuildDiffScope::~SexRebuildDiffScope()
{
    Active = Previous;
}
void BindSexRebuildDiffActor(Actor* actor, uint32_t serverId) noexcept
{
    try
    {
        std::lock_guard lock(IdentityMutex);
        const auto token = reinterpret_cast<uintptr_t>(actor);
        Identity* slot{};
        for (auto& entry : Identities)
            if (entry.Actor == token || entry.Server == serverId)
            {
                entry = {};
                slot = &entry;
            }
        if (!slot)
            for (auto& entry : Identities)
                if (!entry.Actor)
                {
                    slot = &entry;
                    break;
                }
        if (!slot)
            slot = &Identities[NextIdentity++ % Identities.size()];
        *slot = {token, actor->formID, serverId};
    }
    catch (...)
    {
    }
}
void ResetSexRebuildDiffActors() noexcept
{
    try
    {
        std::lock_guard lock(IdentityMutex);
        Identities = {};
        NextIdentity = 0;
    }
    catch (...)
    {
    }
}
void LogSexRebuildDiff(const char* phase, Actor* actor, uint64_t sequence, uintptr_t caller, bool updateWeight, const char* execution) noexcept
{
    const auto error = GetLastError();
    try
    {
        if (!actor)
        {
            SetLastError(error);
            return;
        }
        auto* base = Cast<TESNPC>(actor->baseForm);
        bool g1Known{}, g2Known{};
        const auto g1 = Global(401069, g1Known), g2 = Global(403521, g2Known);
        const auto token = reinterpret_cast<uintptr_t>(actor);
        uint32_t server{};
        bool serverKnown{};
        {
            std::lock_guard lock(IdentityMutex);
            for (const auto& entry : Identities)
                if (entry.Actor == token && entry.Form == actor->formID)
                {
                    server = entry.Server;
                    serverKnown = true;
                    break;
                }
        }
        const char* category = (g1Known && token == g1) || (g2Known && token == g2) ? "local-player" : actor->GetExtension()->IsRemotePlayer() ? "remote-player-STRE" : "ordinary";
        uint64_t parts = 14695981039346656037ull;
        bool partsKnown = base && (!base->headpartsCount || base->headparts);
        if (partsKnown)
            for (uint32_t i = 0; i < base->headpartsCount; ++i)
            {
                uintptr_t part{};
                uint32_t form{};
                if (!ReadAt(reinterpret_cast<uintptr_t>(base->headparts) + i * sizeof(void*), part) || (part && !ReadAt(part + offsetof(TESForm, formID), form)))
                {
                    partsKnown = false;
                    break;
                }
                parts = (parts ^ form) * 1099511628211ull;
            }
        uintptr_t changeManager{}, changeStore{};
        const bool flagsKnown = base && ReadAt(reinterpret_cast<uintptr_t>(VersionDb::Get().FindAddressById(403330)), changeManager) && changeManager &&
                                ReadAt(changeManager + 0x330, changeStore) && changeStore;
        const auto flags = flagsKnown ? base->GetChangeFlags() : 0;
        auto* settings = INISettingCollection::Get();
        auto* ini = settings ? settings->GetSetting("bUseFaceGenPreprocessedHeads:General") : nullptr;
        const auto iniValue = ini ? ini->data : 0;
        uintptr_t process{}, sub8{}, sub10{};
        uint8_t level{}, flag{};
        const bool processKnown = ReadAt(token + 0xF8, process);
        const bool sub8Known = processKnown && process && ReadAt(process + 8, sub8);
        const bool sub10Known = processKnown && process && ReadAt(process + 0x10, sub10);
        const bool levelKnown = processKnown && process && ReadAt(process + 0x137, level);
        const bool flagKnown = sub8Known && sub8 && ReadAt(sub8 + 0x311, flag);
        uintptr_t biped1{}, biped2{}, vtable{}, getter{};
        bool b1Known{}, b2Known{};
        const bool globalBranch = g1Known && token == g1;
        // These are the operands used by DoReset3D, not arbitrary biped contents.
        if (globalBranch && g2Known && g2 && ReadAt(g2, vtable) && ReadAt(vtable + 0x3F0, getter) && getter == Image() + 0x736110)
        {
            b1Known = ReadAt(g2 + 0x268, biped1);
            b2Known = ReadAt(g2 + 0x8F0, biped2);
        }
        else if (g1Known && !globalBranch && ReadAt(token, vtable) && ReadAt(vtable + 0x3F8, getter) && getter == Image() + 0x727C30)
            b1Known = ReadAt(token + 0x268, biped1);
        MEMORY_BASIC_INFORMATION callerMemory{};
        const bool callerKnown =
            caller && VirtualQuery(reinterpret_cast<void*>(caller), &callerMemory, sizeof(callerMemory)) && reinterpret_cast<uintptr_t>(callerMemory.AllocationBase) == Image();
        const auto timing = ReadSexChangeProbeTiming(actor);
        const auto root = reinterpret_cast<uintptr_t>(actor->GetNiNode());
        const auto face = reinterpret_cast<uintptr_t>(actor->GetFaceGenNiNode());
        const auto head = reinterpret_cast<uintptr_t>(FaceGenSystem::GetHeadGeometry(actor));
        spdlog::info(
            "[STRE][SexRebuildDiff] phase={} callSeq={} thread={} callerRva={:X} callerKnown={} category={} serverId={} serverIdKnown={} updateWeight={} execution={} "
            "actorForm={:X} actorPtr={:X} baseForm={:X} basePtr={:X} runtimeRace={:X} baseRace={:X} overlayRace={:X} sex={} weight={} headpartsCount={} headpartsHash={:X} "
            "headpartsKnown={} root={:X} face={:X} head={:X}",
            phase, sequence, GetCurrentThreadId(), callerKnown ? caller - Image() : 0, callerKnown, category, server, serverKnown, updateWeight, execution, actor->formID, token,
            base ? base->formID : 0, reinterpret_cast<uintptr_t>(base), actor->race ? actor->race->formID : 0, base && base->raceForm.race ? base->raceForm.race->formID : 0,
            base && base->overlayRace ? base->overlayRace->formID : 0, base ? base->actorData.IsFemale() : false, base ? base->weight : 0.f, base ? base->headpartsCount : 0, parts,
            partsKnown, root, face, head);
        spdlog::info(
            "[STRE][SexRebuildDiff] phase={} callSeq={} thread={} actorPtr={:X} global401069Ptr={:X} global401069Known={} global403521Ptr={:X} global403521Known={} "
            "actorEquals401069={} actorEquals403521={} globalsAlias={} globalsAliasKnown={} baseChangeFlags={:X} baseChangeFlagsKnown={} has0x800={} FaceGenINI={} "
            "FaceGenINIKnown={} aiProcessPtr={:X} aiProcessKnown={} processLevel={} processLevelKnown={} processSubPtr8={:X} processSubPtr8Known={} processSubPtr10={:X} "
            "processSubPtr10Known={} processFlagByte311={:X} processFlagByte311Known={}",
            phase, sequence, GetCurrentThreadId(), token, g1, g1Known, g2, g2Known, g1Known && token == g1, g2Known && token == g2, g1Known && g2Known && g1 == g2,
            g1Known && g2Known, flags, flagsKnown, (flags & 0x800) != 0, iniValue, ini != nullptr, process, processKnown, level, levelKnown, sub8, sub8Known, sub10,
            sub10Known, flag, flagKnown);
        spdlog::info(
            "[STRE][SexRebuildDiff] phase={} callSeq={} thread={} actorPtr={:X} biped1Ptr={:X} biped1Known={} biped2Ptr={:X} biped2Known={} bipedBranch={} "
            "bipedBranchApplicable={} sexChangeTick={} DoResetTick={} delta={} sexChangeTickKnown={} tickDomain=CharacterCreationService",
            phase, sequence, GetCurrentThreadId(), token, biped1, b1Known, biped2, b2Known,
            !g1Known       ? "unresolved"
            : globalBranch ? "global-player"
                           : "non-global-actor",
            updateWeight, timing.SexTick, timing.Tick, timing.Known ? timing.Tick - timing.SexTick : 0, timing.Known);
        if (MenuActor == actor && std::string_view(phase) == "DoReset3D-return")
            spdlog::info(
                "[STRE][SexRebuildDiff] phase=RaceMenu52391-after-reset callSeq={} thread={} actorPtr={:X} root={:X} face={:X} head={:X}", sequence, GetCurrentThreadId(), token,
                root, face, head);
    }
    catch (...)
    {
    }
    SetLastError(error);
}
