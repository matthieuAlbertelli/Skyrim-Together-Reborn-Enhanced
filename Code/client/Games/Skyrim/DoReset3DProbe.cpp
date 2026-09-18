#include <TiltedOnlinePCH.h>
#include <SexChangeProbe.h>
#include <SexRebuildDiffProbe.h>
#include <CharacterCreation/SexProbeWindow.h>
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
TP_THIS_FUNCTION(TDoReset3D, void, Actor, bool);
TDoReset3D* RealDoReset3D{};
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
        "[STRE][SexChangeProbe] phase={} callSeq={} thread={} actor={:X} actorPtrToken={:X} isPlayerRef={} actorRuntimeRace={:X} baseNpc={:X} baseRace={:X} overlayRace={:X} "
        "sex={} weight={} thirdPerson3D={:X} faceNode={:X} head={:X} actorStateFlags1={:08X} actorStateFlags2={:08X}",
        phase, sequence, GetCurrentThreadId(), value.ActorId, reinterpret_cast<uintptr_t>(actor), actor && actor == PlayerCharacter::Get(), value.RuntimeRace, value.Base,
        value.BaseRace, value.Overlay, value.Sex, value.Weight, value.Third, value.Face, value.Head, value.Flags1, value.Flags2);
}

void TP_MAKE_THISCALL(HookDoReset3D, Actor, bool updateWeight)
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
        SignalSexChangeProbe(apThis, "do-reset3d-window");
        captured = true;
        MEMORY_BASIC_INFORMATION memory{};
        const bool known = VirtualQuery(reinterpret_cast<void*>(caller), &memory, sizeof(memory)) != 0 && memory.Type == MEM_IMAGE;
        char module[MAX_PATH]{};
        if (known)
            GetModuleFileNameA(static_cast<HMODULE>(memory.AllocationBase), module, MAX_PATH);
        const auto moduleBase = known ? reinterpret_cast<uintptr_t>(memory.AllocationBase) : 0;
        spdlog::info(
            "[STRE][SexChangeProbe] phase=do-reset3d-arguments callSeq={} argUpdateWeight={} callerToken={:X} callerModule={} callerRva={:X} callerKnown={}", sequence,
            updateWeight, caller, module, moduleBase ? caller - moduleBase : 0, known);
        Log("do-reset3d-enter", sequence, apThis, before);
    }
    catch (...)
    {
    } // Diagnostic allocation/logging failure must not suppress the original.

    LogSexRebuildDiff("DoReset3D-enter", apThis, sequence, caller, updateWeight, "unresolved");
    STRE::CharacterCreation::SexRebuildExecution execution;
    SetLastError(incomingError);
    // Exactly one forwarding call, outside diagnostic exception handling.
    {
        SexRebuildDiffScope scope(apThis, sequence);
        STRE::CharacterCreation::PassiveSexProbeCall([] {}, [&] { TiltedPhoques::ThisCall(RealDoReset3D, apThis, updateWeight); }, [] {});
        execution = scope.Frame;
    }
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
            LogSexRebuildDiff("DoReset3D-return", apThis, sequence, caller, updateWeight, execution.Result());
            const auto after = Read(apThis);
            Log("do-reset3d-return", sequence, apThis, after);
            SignalSexChangeProbe(apThis, "do-reset3d-return-observation");
            spdlog::info(
                "[STRE][SexChangeProbe] phase=do-reset3d-changes callSeq={} runtimeRaceChanged={} baseRaceChanged={} overlayRaceChanged={} thirdPersonChanged={} "
                "faceNodeChanged={} "
                "headChanged={}",
                sequence, before.RuntimeRace != after.RuntimeRace, before.BaseRace != after.BaseRace, before.Overlay != after.Overlay, before.Third != after.Third,
                before.Face != after.Face, before.Head != after.Head);
        }
        else
            spdlog::warn("[STRE][SexChangeProbe] phase=do-reset3d-return callSeq={} stateUnavailable=true reason=missing-prestate-or-actor-identity-changed", sequence);
    }
    catch (...)
    {
    }
    SetLastError(outgoingError);
}
static TiltedPhoques::Initializer s_reset3DProbe(
    []
    {
        const auto& version = VersionDb::Get();
        if (!STRE::CharacterCreation::SexProbeRuntime(version.GetLoadedVersionString()))
            return;
        auto* address = version.FindAddressById(40255);
        MEMORY_BASIC_INFORMATION memory{};
        if (!address || !VirtualQuery(address, &memory, sizeof(memory)) || memory.State != MEM_COMMIT || (memory.Protect & PAGE_GUARD) ||
            !(memory.Protect & (PAGE_EXECUTE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY)))
        {
            spdlog::warn("[STRE][SexChangeProbe] skipped reason=unresolved-or-nonexecutable-relocation");
            return;
        }
        RealDoReset3D = reinterpret_cast<TDoReset3D*>(address);
        // Existing immediate path checks MH_CreateHook/MH_EnableHook results. Unlike
        // delayed installation it does not retain a failed hook. No custom trampoline.
        TP_HOOK_IMMEDIATE(&RealDoReset3D, HookDoReset3D);
        // The manager has no status-return API: first enter is activation evidence.
        spdlog::info("[STRE][SexChangeProbe] install-attempt runtime=1.6.1170.0 relocation=40255 activationRequiresEnterTrace=true");
    });
} // namespace
