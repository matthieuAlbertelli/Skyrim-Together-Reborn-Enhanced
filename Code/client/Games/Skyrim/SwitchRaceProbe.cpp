#include <TiltedOnlinePCH.h>
#include <SwitchRaceProbe.h>
#include <SexChangeProbe.h>
#include <Actor.h>
#include <Games/ActorExtension.h>
#include <PlayerCharacter.h>
#include <Forms/TESNPC.h>
#include <Forms/TESRace.h>
#include <Systems/FaceGenSystem.h>
#include <CharacterCreation/SwitchRaceCorrelation.h>
#include <VersionDb.h>
#include <intrin.h>
#include <mutex>

namespace
{
TP_THIS_FUNCTION(TSwitchRace, void, Actor, TESRace*, bool);
TSwitchRace* RealSwitchRace{};
std::mutex CorrelationMutex;
STRE::CharacterCreation::SwitchRaceCorrelation Correlation;

struct Observation
{
    uint32_t ActorId{}, RuntimeRace{}, Base{}, BaseRace{}, Overlay{}, Sex{}, Flags1{}, Flags2{};
    float Weight{};
    uintptr_t Third{}, Face{}, Head{};
};

Observation Read(Actor* actor)
{
    Observation value;
    if (!actor)
        return value;
    value.ActorId = actor->formID;
    value.RuntimeRace = actor->race ? actor->race->formID : 0;
    if (const auto* base = Cast<TESNPC>(actor->baseForm))
    {
        value.Base = base->formID;
        value.BaseRace = base->raceForm.race ? base->raceForm.race->formID : 0;
        value.Overlay = base->overlayRace ? base->overlayRace->formID : 0;
        value.Sex = base->actorData.IsFemale();
        value.Weight = base->weight;
    }
    value.Third = reinterpret_cast<uintptr_t>(actor->GetNiNode());
    value.Face = reinterpret_cast<uintptr_t>(actor->GetFaceGenNiNode());
    value.Head = reinterpret_cast<uintptr_t>(FaceGenSystem::GetHeadGeometry(actor));
    value.Flags1 = actor->actorState.flags1;
    value.Flags2 = actor->actorState.flags2;
    return value;
}

void Log(const char* phase, uint64_t sequence, Actor* actor, const Observation& value)
{
    spdlog::info("[STRE][AppearanceTrace][Native] phase={} callSeq={} actorForm={:X} baseForm={:X} basePtr={:X} category={} serverId=join-by-actor-form",
                 phase, sequence, value.ActorId, value.Base, reinterpret_cast<uintptr_t>(actor ? actor->baseForm : nullptr),
                 actor && actor == PlayerCharacter::Get() ? "local-player" : actor && actor->GetExtension()->IsRemotePlayer() ? "remote-player-STRE" : "ordinary-NPC");

    spdlog::info(
        "[STRE][SwitchRaceProbe] phase={} callSeq={} thread={} actor={:X} actorPtrToken={:X} isPlayerRef={} actorRuntimeRace={:X} baseNpc={:X} baseRace={:X} overlayRace={:X} "
        "sex={} weight={} thirdPerson3D={:X} faceNode={:X} head={:X} actorStateFlags1={:08X} actorStateFlags2={:08X}",
        phase, sequence, GetCurrentThreadId(), value.ActorId, reinterpret_cast<uintptr_t>(actor), actor && actor == PlayerCharacter::Get(), value.RuntimeRace, value.Base,
        value.BaseRace, value.Overlay, value.Sex, value.Weight, value.Third, value.Face, value.Head, value.Flags1, value.Flags2);
}

void TP_MAKE_THISCALL(HookSwitchRace, Actor, TESRace* race, bool player)
{
    const auto caller = reinterpret_cast<uintptr_t>(_ReturnAddress());
    const auto incomingError = GetLastError();
    uint64_t sequence{};
    Observation before;
    bool captured{};
    try
    {
        {
            std::lock_guard lock(CorrelationMutex);
            sequence = Correlation.Enter(reinterpret_cast<uintptr_t>(apThis));
        }
        before = Read(apThis);
        SignalSexChangeProbe(apThis, "switch-race-window" );
        captured = true;
        MEMORY_BASIC_INFORMATION memory{};
        const bool known = VirtualQuery(reinterpret_cast<void*>(caller), &memory, sizeof(memory)) != 0 && memory.Type == MEM_IMAGE;
        char module[MAX_PATH]{};
        if (known)
            GetModuleFileNameA(static_cast<HMODULE>(memory.AllocationBase), module, MAX_PATH);
        const auto moduleBase = known ? reinterpret_cast<uintptr_t>(memory.AllocationBase) : 0;
        spdlog::info(
            "[STRE][SwitchRaceProbe] phase=arguments callSeq={} targetRace={:X} argPlayer={} callerToken={:X} callerModule={} callerRva={:X} callerKnown={}", sequence,
            race ? race->formID : 0, player, caller, module, moduleBase ? caller - moduleBase : 0, known);
        Log("enter", sequence, apThis, before);
    }
    catch (...)
    {
    } // Diagnostic allocation/logging failure must not suppress the original.

    SetLastError(incomingError);
    // Exactly one forwarding call, outside diagnostic exception handling.
    TiltedPhoques::ThisCall(RealSwitchRace, apThis, race, player);
    const auto outgoingError = GetLastError();
    try
    {
        {
            std::lock_guard lock(CorrelationMutex);
            Correlation.Return(sequence);
        }
        // Do not dereference an old actor if the engine unexpectedly replaced it.
        const bool sameActor = captured && apThis && Cast<Actor>(TESForm::GetById(before.ActorId)) == apThis;
        if (sameActor)
        {
            const auto after = Read(apThis);
            Log("return", sequence, apThis, after);
            SignalSexChangeProbe(apThis, "switch-race-return-observation");
            spdlog::info(
                "[STRE][SwitchRaceProbe] phase=changes callSeq={} runtimeRaceChanged={} baseRaceChanged={} overlayRaceChanged={} thirdPersonChanged={} faceNodeChanged={} "
                "headChanged={}",
                sequence, before.RuntimeRace != after.RuntimeRace, before.BaseRace != after.BaseRace, before.Overlay != after.Overlay, before.Third != after.Third,
                before.Face != after.Face, before.Head != after.Head);
        }
        else
            spdlog::warn("[STRE][SwitchRaceProbe] phase=return callSeq={} stateUnavailable=true reason=missing-prestate-or-actor-identity-changed", sequence);
    }
    catch (...)
    {
    }
    SetLastError(outgoingError);
}
static TiltedPhoques::Initializer s_switchRaceProbe(
    []
    {
        const auto& version = VersionDb::Get();
        if (version.GetLoadedVersionString() != "1.6.1170.0")
            return;
        auto* address = version.FindAddressById(37925);
        MEMORY_BASIC_INFORMATION memory{};
        if (!address || !VirtualQuery(address, &memory, sizeof(memory)) || memory.State != MEM_COMMIT || (memory.Protect & PAGE_GUARD) ||
            !(memory.Protect & (PAGE_EXECUTE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY)))
        {
            spdlog::warn("[STRE][SwitchRaceProbe] skipped reason=unresolved-or-nonexecutable-relocation");
            return;
        }
        RealSwitchRace = reinterpret_cast<TSwitchRace*>(address);
        // Existing immediate path checks MH_CreateHook/MH_EnableHook results. Unlike
        // delayed installation it does not retain a failed hook. No custom trampoline.
        TP_HOOK_IMMEDIATE(&RealSwitchRace, HookSwitchRace);
        // The manager has no status-return API: first enter is activation evidence.
        spdlog::info("[STRE][SwitchRaceProbe] install-attempt runtime=1.6.1170.0 relocation=37925 activationRequiresEnterTrace=true");
    });
} // namespace

void ObserveSwitchRaceProbe(Actor* actor, const char* phase) noexcept
{
    if (!actor || VersionDb::Get().GetLoadedVersionString() != "1.6.1170.0")
        return;
    const auto previousError = GetLastError();
    try
    {
        STRE::CharacterCreation::SwitchRaceCorrelation::Record record;
        {
            std::lock_guard lock(CorrelationMutex);
            record = Correlation.Latest(reinterpret_cast<uintptr_t>(actor));
        }
        Log(phase, record.Sequence, actor, Read(actor));
        spdlog::info("[STRE][SwitchRaceProbe] phase=correlation callSeq={} inFlight={} association=latest-observed-not-causal", record.Sequence, record.InFlight);
    }
    catch (...)
    {
    }
    SetLastError(previousError);
}