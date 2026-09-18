#include <TiltedOnlinePCH.h>
#include <NativeLifetimeProbe.h>
#include <CharacterCreation/NativeLifetimeWindow.h>
#include <CharacterCreation/NativeLifetimeBinding.h>
#include <Actor.h>
#include <Games/ActorExtension.h>
#include <Forms/TESNPC.h>
#include <Games/TES.h>
#include <Components.h>
#include <World.h>
#include <VersionDb.h>
#include <array>
#include <atomic>
#include <chrono>
#include <mutex>

namespace
{
using Clock = std::chrono::steady_clock;
using STRE::CharacterCreation::NativeLifetimeWindow;
std::atomic_bool s_enabled{false};
#if (!IS_MASTER)
std::atomic_bool s_observeCurrentRequested{false};
#endif
std::mutex s_mutex;
thread_local uint64_t s_creationSession{};
uint64_t s_nextSession{}, s_lastServiceTick{};
struct Record
{
    uint64_t Session{}, Tick{};
    uint32_t Entity{}, Server{}, ActorForm{}, BaseForm{}, Handle{};
    uintptr_t ActorToken{}, BaseToken{}; // Comparison/logging only, never dereferenced.
    Clock::time_point Start{};
    NativeLifetimeWindow Window;
    bool Active{}, DiscoveryKnown{}, Discovered{}, ActorDtor{}, BaseDtor{};
    bool ActorSeen{}, BaseSeen{}, HandleSeen{}, HighSeen{}, RootSeen{};
    bool Sampled{};
    int ActorLookup{-1}, BaseLookup{-1}, HandleLookup{-1}, HighProcess{-1}, Root{-1}, Bound{-1};
};
Record s_current;

uint64_t Elapsed(const Record& aRecord)
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now() - aRecord.Start).count();
}
bool Tracking()
{
    return s_current.Active && !s_current.Window.Expired(Elapsed(s_current));
}

// Every diagnostic entry restores Win32 last-error and swallows only diagnostic
// failures. Native forwarding calls remain outside these guards.
template <class T> void Observe(T&& aBody) noexcept
{
    const auto error = GetLastError();
    try
    {
        aBody();
    }
    catch (...)
    {
    }
    SetLastError(error);
}

void Log(const char* apPhase, const Record& aRecord, uint32_t aFlags = 0)
{
    spdlog::info(
        "[STRE][NativeLifetime] phase={} probeSession={} lastServiceTick={} thread={} elapsedMs={} serverId={} entityVersioned={} "
        "actorForm={:X} baseForm={:X} actorToken={:X} baseToken={:X} handle={:X} flags={} "
        "discoveryKnown={} discovered={} actorLookup={} baseLookup={} handleLookup={} highProcess={} lastDiscoveryRoot={} ecsBound={}",
        apPhase, aRecord.Session, aRecord.Tick, GetCurrentThreadId(), Elapsed(aRecord), aRecord.Server, aRecord.Entity, aRecord.ActorForm, aRecord.BaseForm, aRecord.ActorToken,
        aRecord.BaseToken, aRecord.Handle, aFlags, aRecord.DiscoveryKnown, aRecord.Discovered, aRecord.ActorLookup, aRecord.BaseLookup, aRecord.HandleLookup, aRecord.HighProcess,
        aRecord.Root, aRecord.Bound);
}

using DeletingDestructor = void*(void*, uint32_t);
DeletingDestructor* s_realCharacterDeletingDestructor{};
DeletingDestructor* s_realNpcDeletingDestructor{};

uint64_t DestructorEnter(void* apObject, bool aBase, uint32_t aFlags) noexcept
{
    uint64_t session = 0;
    if (!s_enabled.load())
        return 0;
    Observe(
        [&]
        {
            std::lock_guard lock(s_mutex);
            auto& seen = aBase ? s_current.BaseDtor : s_current.ActorDtor;
            if (!Tracking() || seen || reinterpret_cast<uintptr_t>(apObject) != (aBase ? s_current.BaseToken : s_current.ActorToken))
                return;
            // This is the live complete-object argument at the proven destructor entry,
            // before forwarding. Never reconstruct a pointer from a stored token.
            if (static_cast<const TESForm*>(apObject)->formID != (aBase ? s_current.BaseForm : s_current.ActorForm))
                return;
            seen = true;
            if (!aBase)
                s_current.Root = -1;
            session = s_current.Session;
            s_current.Window.ObserveRemoval(Elapsed(s_current));
            Log(aBase ? "tesnpc-deleting-destructor-enter" : "actor-deleting-destructor-enter", s_current, aFlags);
        });
    return session;
}
void DestructorReturn(uint64_t aSession, bool aBase, uint32_t aFlags) noexcept
{
    if (!aSession)
        return;
    Observe(
        [&]
        {
            std::lock_guard lock(s_mutex);
            if (Tracking() && s_current.Session == aSession)
                Log(aBase ? "tesnpc-deleting-destructor-return" : "actor-deleting-destructor-return", s_current, aFlags);
        });
}
void* HookCharacterDeletingDestructor(void* apObject, uint32_t aFlags)
{
    const auto session = DestructorEnter(apObject, false, aFlags);
    void* result = s_realCharacterDeletingDestructor(apObject, aFlags);
    DestructorReturn(session, false, aFlags);
    return result;
}
void* HookNpcDeletingDestructor(void* apObject, uint32_t aFlags)
{
    const auto session = DestructorEnter(apObject, true, aFlags);
    void* result = s_realNpcDeletingDestructor(apObject, aFlags);
    DestructorReturn(session, true, aFlags);
    return result;
}

bool Verified(uint64_t aId, uintptr_t aRva, const std::array<uint8_t, 16>& aBytes)
{
    const auto address = VersionDb::Get().FindAddressById(aId);
    std::array<uint8_t, 16> actual{};
    SIZE_T read{};
    MEMORY_BASIC_INFORMATION region{};
    return reinterpret_cast<uintptr_t>(address) == reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr)) + aRva &&
           VirtualQuery(address, &region, sizeof(region)) == sizeof(region) && region.State == MEM_COMMIT && !(region.Protect & (PAGE_GUARD | PAGE_NOACCESS)) &&
           (region.Protect & (PAGE_EXECUTE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY)) &&
           ReadProcessMemory(GetCurrentProcess(), address, actual.data(), actual.size(), &read) && read == actual.size() && actual == aBytes;
}
void InstallVerifiedHooks()
{
    // Installed only on explicit diagnostic enable. No old destructor stub reuse.
    static bool attempted = false;
    if (attempted)
        return;
    attempted = true;
    if (Verified(40288, 0x728090, {0x48, 0x89, 0x5c, 0x24, 0x08, 0x57, 0x48, 0x83, 0xec, 0x20, 0x8b, 0xda, 0x48, 0x8b, 0xf9, 0xe8}))
    {
        s_realCharacterDeletingDestructor = reinterpret_cast<DeletingDestructor*>(VersionDb::Get().FindAddressById(40288));
        TP_HOOK_IMMEDIATE(&s_realCharacterDeletingDestructor, HookCharacterDeletingDestructor);
        spdlog::info("[STRE][NativeLifetime] phase=hook-install-attempt id=40288 activationRequiresEnter=true");
    }
    else
        spdlog::warn("[STRE][NativeLifetime] phase=hook-skipped id=40288 reason=identity-or-prologue");
    if (Verified(24888, 0x3c4be0, {0x40, 0x57, 0x41, 0x56, 0x41, 0x57, 0x48, 0x83, 0xec, 0x30, 0x48, 0xc7, 0x44, 0x24, 0x20, 0xfe}))
    {
        s_realNpcDeletingDestructor = reinterpret_cast<DeletingDestructor*>(VersionDb::Get().FindAddressById(24888));
        TP_HOOK_IMMEDIATE(&s_realNpcDeletingDestructor, HookNpcDeletingDestructor);
        spdlog::info("[STRE][NativeLifetime] phase=hook-install-attempt id=24888 activationRequiresEnter=true");
    }
    else
        spdlog::warn("[STRE][NativeLifetime] phase=hook-skipped id=24888 reason=identity-or-prologue");
}

#if (!IS_MASTER)
void ObserveCurrentPrivateRemote(World& aWorld, uint64_t aServiceTick)
{
    const auto reject = [](const char* reason)
    {
        spdlog::info("[STRE][NativeLifetime] phase=current-private-remote-rejected reason={}", reason);
    };
    if (s_enabled.load())
        return reject("disable-existing-probe-first");
    if (VersionDb::Get().GetLoadedVersionString() != "1.6.1170.0")
        return reject("unsupported-runtime");

    // Count remote players before filtering provenance. Ambiguous or partly
    // materialized multiplayer state must never select an arbitrary actor.
    entt::entity selected = entt::null;
    for (const auto entity : aWorld.view<RemoteComponent, PlayerComponent>())
    {
        if (selected != entt::null)
            return reject("multiple-remote-players");
        selected = entity;
    }
    if (selected == entt::null)
        return reject("no-current-remote-player");
    const auto* form = aWorld.try_get<FormIdComponent>(selected);
    const auto* identity = aWorld.try_get<RemotePlayerAppearanceBaseComponent>(selected);
    const auto& remote = aWorld.get<RemoteComponent>(selected);
    if (!form || aWorld.any_of<LocalComponent, WaitingForAssignmentComponent, WaitingFor3D>(selected) || remote.CachedRefId != form->Id || form->Id < 0xFF000000)
        return reject("private-binding-not-ready");

    // Fresh lookups on the game thread, never a stored token cast back to a pointer.
    auto* actor = Cast<Actor>(TESForm::GetById(form->Id));
    auto* extension = actor ? actor->GetExtension() : nullptr;
    auto* nativeBase = actor ? Cast<TESNPC>(actor->baseForm) : nullptr;
    const uint32_t baseId = nativeBase ? nativeBase->formID : 0;
    auto* base = baseId >= 0xFF000000 ? Cast<TESNPC>(TESForm::GetById(baseId)) : nullptr;
    if (!actor || !extension || !extension->IsRemotePlayer() || !base || actor->formID != form->Id || base->formID != baseId || actor->baseForm != base)
        return reject("native-binding-not-ready");

    using namespace STRE::CharacterCreation;
    const auto evidence =
        ClassifyNativeLifetimeBinding(form->Id, baseId, remote.CachedRefId, identity != nullptr, identity ? identity->ActorFormId : 0, identity ? identity->BaseFormId : 0);
    if (evidence == NativeLifetimeBindingEvidence::Rejected)
        return reject("private-provenance-mismatch"); // Never bypass contradictory provenance.

    // Exclude aliases in known ECS/native bindings. This is not a census of
    // every engine reference, nor proof of exclusive private-base ownership.
    for (const auto other : aWorld.view<FormIdComponent>())
    {
        if (other == selected)
            continue;
        const uint32_t otherId = aWorld.get<FormIdComponent>(other).Id;
        if (otherId == form->Id)
            return reject("ambiguous-current-binding");
        auto* otherActor = Cast<Actor>(TESForm::GetById(otherId));
        if (otherActor && otherActor->baseForm == base)
            return reject("shared-current-base");
    }
    for (const auto other : aWorld.view<RemoteComponent>())
        if (other != selected)
        {
            const auto& otherRemote = aWorld.get<RemoteComponent>(other);
            if (otherRemote.Id == remote.Id || otherRemote.CachedRefId == form->Id)
                return reject("ambiguous-current-binding");
        }

    Record observed;
    observed.Tick = aServiceTick;
    observed.Entity = static_cast<uint32_t>(selected);
    observed.Server = remote.Id;
    observed.ActorForm = form->Id;
    observed.BaseForm = baseId;
    observed.ActorToken = reinterpret_cast<uintptr_t>(actor);
    observed.BaseToken = reinterpret_cast<uintptr_t>(base);
    observed.ActorLookup = observed.BaseLookup = observed.Bound = 1;
    observed.ActorSeen = observed.BaseSeen = true;

    // Reuse an already published handle if present. Never call GetHandle (which
    // can create one), and never dereference a returned handle-lookup pointer.
    if (auto* lists = ProcessLists::Get())
        for (uint32_t i = 0; i < lists->highActorHandleArray.length; ++i)
        {
            const uint32_t handle = lists->highActorHandleArray[i];
            if (handle && handle != UINT32_MAX && reinterpret_cast<uintptr_t>(TESObjectREFR::GetByHandle(handle)) == observed.ActorToken)
            {
                observed.Handle = handle;
                observed.HandleLookup = observed.HighProcess = 1;
                observed.HandleSeen = observed.HighSeen = true;
                break;
            }
        }
    // A temporary handle release may have destroyed the object. Revalidate only
    // registry tokens, with no subsequent native-object dereference.
    if (reinterpret_cast<uintptr_t>(TESForm::GetById(observed.ActorForm)) != observed.ActorToken ||
        reinterpret_cast<uintptr_t>(TESForm::GetById(observed.BaseForm)) != observed.BaseToken)
        return reject("native-binding-changed-during-selection");

    InstallVerifiedHooks();
    std::lock_guard lock(s_mutex);
    if (s_enabled.load())
        return reject("disable-existing-probe-first");
    observed.Session = ++s_nextSession;
    observed.Start = Clock::now(); // Current observation starts now, not at creation.
    observed.Active = true;
    s_current = observed;
    s_enabled.store(true);
    spdlog::info("[STRE][NativeLifetime] phase=armed-current-private-remote pollMs=100 maxMs=180000 postRemovalMs=30000");
    spdlog::info(
        "[STRE][NativeLifetime] phase=current-private-remote-evidence probeSession={} selectionEvidence={} provenancePresent={} serverId={} entityVersioned={} actorForm={:X} "
        "baseForm={:X}",
        s_current.Session, evidence == NativeLifetimeBindingEvidence::DurableProvenance ? "durable-provenance" : "validated-current-binding-fallback", identity != nullptr,
        observed.Server, observed.Entity, observed.ActorForm, observed.BaseForm);
    Log("current-private-remote-selected", s_current);
    // No fabricated creation/discovery history. Unknown root/discovery/handle
    // fields stay unknown until the existing passive observers see them.
}
#endif
} // namespace

#if (!IS_MASTER)
void RequestObserveCurrentPrivateRemote() noexcept
{
    s_observeCurrentRequested.store(true);
}
#endif

bool IsNativeLifetimeProbeEnabled() noexcept
{
    return s_enabled.load();
}
void SetNativeLifetimeProbeEnabled(bool aEnabled) noexcept
{
#if (!IS_MASTER)
    s_observeCurrentRequested.store(false);
#endif
    Observe(
        [&]
        {
            if (aEnabled && VersionDb::Get().GetLoadedVersionString() != "1.6.1170.0")
                return;
            if (aEnabled == s_enabled.load())
                return;
            if (aEnabled)
                InstallVerifiedHooks();
            std::lock_guard lock(s_mutex);
            if (s_current.Active)
                Log("observation-cancelled-by-user", s_current);
            s_current = {};
            s_enabled.store(aEnabled);
            spdlog::info("[STRE][NativeLifetime] phase={} pollMs=100 maxMs=180000 postRemovalMs=30000", aEnabled ? "armed-next-private-player" : "disabled");
        });
}

NativeLifetimeCreationScope::NativeLifetimeCreationScope(uint32_t aEntity, uint32_t aServerId) noexcept
    : m_previous(s_creationSession)
{
    s_creationSession = 0;
    if (!s_enabled.load())
        return;
    Observe(
        [&]
        {
            std::lock_guard lock(s_mutex);
            if (s_current.Session)
                return; // One natural pair per arm, no silent eviction/recreation.
            s_current.Session = ++s_nextSession;
            s_current.Tick = s_lastServiceTick;
            s_current.Entity = aEntity;
            s_current.Server = aServerId;
            s_current.Start = Clock::now();
            s_current.Active = true;
            s_creationSession = s_current.Session;
            Log("natural-creation-selected", s_current);
        });
}
NativeLifetimeCreationScope::~NativeLifetimeCreationScope() noexcept
{
    s_creationSession = m_previous;
}

void NativeLifetimeStage(const char* apPhase, const TESForm* apLiveForm, bool aBase, uint32_t aSpawnHandle) noexcept
{
    if (!s_enabled.load() || !s_creationSession)
        return;
    Observe(
        [&]
        {
            // Only source-owned live arguments, never reconstructed address tokens.
            // Keep engine registry locking outside the recorder mutex.
            const uint32_t formId = apLiveForm ? apLiveForm->formID : 0;
            const auto* found = formId ? TESForm::GetById(formId) : nullptr;
            const int lookup = !formId ? -1 : !found ? 0 : found == apLiveForm ? 1 : 2;
            std::lock_guard lock(s_mutex);
            if (!Tracking() || s_current.Session != s_creationSession)
                return;
            if (apLiveForm)
            {
                if (aBase)
                {
                    s_current.BaseForm = formId;
                    s_current.BaseToken = reinterpret_cast<uintptr_t>(apLiveForm);
                }
                else
                {
                    s_current.ActorForm = formId;
                    s_current.ActorToken = reinterpret_cast<uintptr_t>(apLiveForm);
                }
                if (aBase)
                    s_current.BaseLookup = lookup;
                else
                    s_current.ActorLookup = lookup;
                auto& seen = aBase ? s_current.BaseSeen : s_current.ActorSeen;
                if (lookup == 1 && !seen)
                {
                    seen = true;
                    Log(aBase ? "base-getbyid-first-seen" : "form-getbyid-first-seen", s_current);
                }
            }
            if (aSpawnHandle && aSpawnHandle != UINT32_MAX)
                s_current.Handle = aSpawnHandle;
            Log(apPhase, s_current);
        });
}

uint64_t NativeLifetimeDeleteEnter(const TESObjectREFR* apLiveReference) noexcept
{
    uint64_t session = 0;
    if (!s_enabled.load())
        return 0;
    Observe(
        [&]
        {
            std::lock_guard lock(s_mutex);
            if (!Tracking() || reinterpret_cast<uintptr_t>(apLiveReference) != s_current.ActorToken || s_current.ActorDtor)
                return;
            session = s_current.Session;
            s_current.Window.ObserveRemoval(Elapsed(s_current));
            Log("delete-request-enter", s_current);
            Log("delete-call-enter", s_current);
        });
    return session;
}
void NativeLifetimeDeleteReturn(uint64_t aSession) noexcept
{
    if (!aSession)
        return;
    Observe(
        [&]
        {
            std::lock_guard lock(s_mutex);
            if (Tracking() && s_current.Session == aSession)
                Log("delete-call-return", s_current);
        });
}

void NativeLifetimeDiscovery(uint32_t aFormId, bool aAdded) noexcept
{
    if (!s_enabled.load())
        return;
    Observe(
        [&]
        {
            std::lock_guard lock(s_mutex);
            if (!Tracking() || !s_current.ActorForm || aFormId != s_current.ActorForm)
                return;
            s_current.DiscoveryKnown = true;
            s_current.Discovered = aAdded;
            // Discovery's existing visit predicate already established a nonnull root.
            // Removal may mean high-process loss, so it does not prove root == null.
            s_current.Root = aAdded ? 1 : -1;
            if (aAdded && !s_current.RootSeen)
            {
                s_current.RootSeen = true;
                Log("root-first-nonnull-observed-by-discovery", s_current);
            }
            if (!aAdded)
                s_current.Window.ObserveRemoval(Elapsed(s_current));
            Log(aAdded ? "discovery-add" : "discovery-remove", s_current);
        });
}
void NativeLifetimeServerRemoval(uint32_t aServerId) noexcept
{
    if (!s_enabled.load())
        return;
    Observe(
        [&]
        {
            std::lock_guard lock(s_mutex);
            if (!Tracking() || aServerId != s_current.Server)
                return;
            s_current.Window.ObserveRemoval(Elapsed(s_current));
            Log("server-removal", s_current);
        });
}
void NativeLifetimeDisconnect() noexcept
{
    if (!s_enabled.load())
        return;
    Observe(
        [&]
        {
            std::lock_guard lock(s_mutex);
            if (!Tracking())
                return;
            s_current.Window.ObserveRemoval(Elapsed(s_current));
            Log("observer-disconnect", s_current); // Continue bounded observation; do not clear identity.
        });
}

void TickNativeLifetimeProbe(World& aWorld, uint64_t aServiceTick) noexcept
{
#if (!IS_MASTER)
    if (s_observeCurrentRequested.exchange(false))
        Observe([&] { ObserveCurrentPrivateRemote(aWorld, aServiceTick); });
#endif
    if (!s_enabled.load())
        return;
    Observe(
        [&]
        {
            Record sample;
            {
                std::lock_guard lock(s_mutex);
                s_lastServiceTick = aServiceTick;
                if (!s_current.Active)
                    return;
                s_current.Tick = aServiceTick;
                const auto elapsed = Elapsed(s_current);
                if (s_current.Window.Expired(elapsed))
                {
                    Log("window-ended-NOT-OBSERVED-WITHIN-WINDOW-is-not-leak-proof", s_current);
                    s_current.Active = false;
                    return;
                }
                if (!s_current.Window.PollDue(elapsed))
                    return;
                sample = s_current;
            }
            // No recorder lock held over an engine lookup: releasing the temporary
            // GetByHandle reference may synchronously enter a destructor observer.
            TESForm* actor = sample.ActorForm ? TESForm::GetById(sample.ActorForm) : nullptr;
            TESForm* base = sample.BaseForm ? TESForm::GetById(sample.BaseForm) : nullptr;
            const auto match = [](const void* p, uintptr_t token)
            {
                return !p ? 0 : reinterpret_cast<uintptr_t>(p) == token ? 1 : 2;
            };
            sample.ActorLookup = sample.ActorForm ? match(actor, sample.ActorToken) : -1;
            sample.BaseLookup = sample.BaseForm ? match(base, sample.BaseToken) : -1;
            if (sample.Handle)
                sample.HandleLookup = match(TESObjectREFR::GetByHandle(sample.Handle), sample.ActorToken);
            if (auto* lists = ProcessLists::Get(); lists && sample.Handle)
            {
                sample.HighProcess = 0;
                for (uint32_t i = 0; i < lists->highActorHandleArray.length; ++i)
                    if (lists->highActorHandleArray[i] == sample.Handle)
                    {
                        sample.HighProcess = 1;
                        break;
                    }
            }
            const auto entity = static_cast<entt::entity>(sample.Entity);
            const auto* form = aWorld.valid(entity) ? aWorld.try_get<FormIdComponent>(entity) : nullptr;
            const auto* remote = aWorld.valid(entity) ? aWorld.try_get<RemoteComponent>(entity) : nullptr;
            sample.Bound = form && remote && form->Id == sample.ActorForm && remote->Id == sample.Server && remote->CachedRefId == sample.ActorForm;
            // Lookup pointers are compared only. In particular, GetByHandle's temporary
            // release may destroy the object; never dereference its returned pointer.
            std::lock_guard lock(s_mutex);
            if (!s_current.Active || s_current.Session != sample.Session)
                return;
            sample.Root = s_current.Root;
            sample.Discovered = s_current.Discovered;
            sample.DiscoveryKnown = s_current.DiscoveryKnown;
            if (sample.ActorLookup == 1 && !s_current.ActorSeen)
            {
                s_current.ActorSeen = true;
                Log("form-getbyid-first-seen", sample);
            }
            if (sample.BaseLookup == 1 && !s_current.BaseSeen)
            {
                s_current.BaseSeen = true;
                Log("base-getbyid-first-seen", sample);
            }
            if (sample.HandleLookup == 1 && !s_current.HandleSeen)
            {
                s_current.HandleSeen = true;
                Log("handle-first-valid", sample);
            }
            if (sample.HighProcess == 1 && !s_current.HighSeen)
            {
                s_current.HighSeen = true;
                Log("high-process-first-seen", sample);
            }
            if (sample.ActorLookup == 0 && s_current.ActorSeen && s_current.ActorLookup != 0)
                Log("form-getbyid-absent", sample);
            if (sample.BaseLookup == 0 && s_current.BaseSeen && s_current.BaseLookup != 0)
                Log("base-getbyid-absent", sample);
            if (sample.HandleLookup == 0 && s_current.HandleSeen && s_current.HandleLookup != 0)
                Log("handle-no-longer-resolves", sample);
            if (!s_current.Sampled || sample.ActorLookup != s_current.ActorLookup || sample.BaseLookup != s_current.BaseLookup || sample.HandleLookup != s_current.HandleLookup ||
                sample.HighProcess != s_current.HighProcess || sample.Root != s_current.Root || sample.Bound != s_current.Bound)
                Log("fresh-lookup-sample-values-minus1-unknown-0-absent-1-same-2-reused", sample);
            s_current.Sampled = true;
            s_current.ActorLookup = sample.ActorLookup;
            s_current.BaseLookup = sample.BaseLookup;
            s_current.HandleLookup = sample.HandleLookup;
            s_current.HighProcess = sample.HighProcess;
            s_current.Bound = sample.Bound;
            if (sample.ActorLookup == 2 || sample.BaseLookup == 2 || sample.HandleLookup == 2)
            {
                Log("identity-reused-stop-correlation", sample);
                s_current.Active = false;
            }
        });
}
