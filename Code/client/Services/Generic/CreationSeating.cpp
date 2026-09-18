#include <TiltedOnlinePCH.h>
#include <Services/CreationSeating.h>
#include <CharacterCreation/CreationSeating.h>
#include <CharacterCreation/FinalRespawn.h>
#include <CharacterCreation/StandingCreation.h>
#include <Services/RemoteRespawnLab.h>
#include <Services/CampaignService.h>
#include <Services/CampaignRuntimeGateService.h>
#include <Services/TransportService.h>
#include <Services/PapyrusService.h>
#include <Messages/NotifyCharacterBuildState.h>
#include <Components.h>
#include <World.h>
#include <Actor.h>
#include <PlayerCharacter.h>
#include <AI/AIProcess.h>
#include <Misc/MiddleProcess.h>
#include <Forms/TESObjectCELL.h>
#include <Games/ActorExtension.h>
#include <Games/TES.h>
#include <VersionDb.h>
#include <map>
#include <string>
#include <chrono>

namespace STRE::CreationSeating
{
namespace
{
using namespace CharacterCreation;
struct Intent
{
    uint32_t Server{}, Player{};
    uint64_t Revision{};
    size_t Index{};
    uint32_t SeatForm{};
    std::string Campaign, DurablePlayer;
    bool Local{}, Solo{};
    SeatProjection Projection;
    std::string LastReason;
    std::chrono::steady_clock::time_point IssuedAt{};
};
std::map<uint32_t, Intent> s_intents;

void Log(Intent& aIntent, const char* aReason)
{
    if (aIntent.LastReason == aReason)
        return;
    aIntent.LastReason = aReason;
    spdlog::info("[STRE][CreationSeating] phase={} serverId={} playerId={} revision={} creationPositionIndex={} seat={:X} actor={:X}",
                 aReason, aIntent.Server, aIntent.DurablePlayer, aIntent.Revision, aIntent.Index, aIntent.SeatForm, aIntent.Projection.Actor);
}
TESObjectREFR* Seat(uint32_t aLocalId)
{
    auto* manager = ModManager::Get();
    auto* mod = manager ? manager->GetByName("STRE_AlternateStart.esp") : nullptr;
    return mod ? Cast<TESObjectREFR>(TESForm::GetById(mod->GetFormId(aLocalId))) : nullptr;
}
}

void Receive(World& aWorld, const NotifyCharacterBuildState& aState) noexcept
{
    if (aState.State != CharacterBuildNetworkState::Applied || !aState.Revision ||
        aState.SeatingCampaignId.empty() || aState.SeatingPlayerId.empty())
        return;
    const auto& snapshot = aWorld.GetCampaignService().GetLatestSnapshot();
    if (snapshot && snapshot->CampaignId != aState.SeatingCampaignId)
        return;
    for (auto& [server, intent] : s_intents)
        if (server != aState.ServerId && intent.Campaign == aState.SeatingCampaignId.c_str() &&
            intent.DurablePlayer == aState.SeatingPlayerId.c_str())
        {
            Log(intent, "rejected-duplicate-durable-binding");
            return;
        }
    const auto existing = s_intents.find(aState.ServerId);
    if (existing != s_intents.end())
    {
        // Never let a duplicate or contradictory identity rearm native activation.
        auto& intent = existing->second;
        if (intent.Revision != aState.Revision || intent.Player != aState.PlayerId ||
            intent.Campaign != aState.SeatingCampaignId.c_str() || intent.DurablePlayer != aState.SeatingPlayerId.c_str())
            Log(intent, "rejected-identity-or-revision");
        return;
    }
    if (s_intents.size() >= 10)
        return;
    Intent intent;
    intent.Server = aState.ServerId;
    intent.Player = aState.PlayerId;
    intent.Revision = aState.Revision;
    intent.Campaign = aState.SeatingCampaignId.c_str();
    intent.DurablePlayer = aState.SeatingPlayerId.c_str();
    s_intents.emplace(intent.Server, std::move(intent));
}

void FinalizeLocal(World& aWorld, uint32_t aServerId, uint64_t aRevision) noexcept
{
    if (!aWorld.GetTransport().IsConnected())
    {
        if (aRevision)
            return; // A disconnected multiplayer final is never an offline Solo final.
        Intent solo;
        solo.Local = solo.Solo = true;
        s_intents.emplace(0, std::move(solo));
        return;
    }
    const auto found = s_intents.find(aServerId);
    if (found != s_intents.end() && found->second.Revision == aRevision)
        found->second.Local = true;
}

void Tick(World& aWorld) noexcept
{
    const auto* gate = CampaignRuntimeGateService::TryGet();
    for (auto& [id, intent] : s_intents)
    {
        if (gate && gate->IsLocked())
        {
            Log(intent, "pending-recovery-lock");
            continue;
        }
        if (VersionDb::Get().GetLoadedVersionString() != "1.6.1170.0")
        {
            Log(intent, "pending-runtime-unsupported");
            continue;
        }
        if (intent.Solo && aWorld.GetTransport().IsConnected())
        {
            Log(intent, "pending-session-changed");
            continue;
        }
        size_t index{};
        if (!intent.Solo)
        {
            if (!aWorld.GetTransport().IsConnected())
            {
                Log(intent, "pending-disconnected");
                continue;
            }
            const auto& snapshot = aWorld.GetCampaignService().GetLatestSnapshot();
            if (!snapshot || !snapshot->RosterSealed || snapshot->CampaignId != intent.Campaign.c_str() ||
                snapshot->Phase != kCampaignWirePhaseCharacterCreation || snapshot->RuntimeState != kCampaignWireRuntimeActive)
            {
                Log(intent, "pending-campaign");
                continue;
            }
            std::vector<std::string_view> players;
            for (const auto& member : snapshot->Roster)
                players.emplace_back(member.PlayerId.data(), member.PlayerId.size());
            const auto selected = StandingCreationPositionIndex(players, intent.DurablePlayer);
            if (!selected)
            {
                Log(intent, "rejected-player-id");
                continue;
            }
            index = *selected;
            if (intent.Player == aWorld.GetTransport().GetLocalPlayerId() && !intent.Local)
            {
                Log(intent, "pending-local-finalization");
                continue;
            }
        }
        auto* actor = intent.Local ? PlayerCharacter::Get() : RemoteRespawnLab::CommittedActor(aWorld, intent.Server, intent.Revision);
        if (!actor || TESForm::GetById(actor->formID) != actor || !actor->GetNiNode() ||
            actor->IsDead() || actor->IsDisabled() || actor->IsInCombat() || actor->IsMount())
        {
            Log(intent, "pending-actor");
            continue;
        }
        if (intent.Local && !intent.Solo)
        {
            const auto durable = aWorld.GetCampaignService().GetDurablePlayerIdForAuthentication();
            if (!durable || *durable != intent.DurablePlayer.c_str())
            {
                Log(intent, "rejected-local-identity");
                continue;
            }
        }
        const auto nativeState = DecodeFinalRespawnActorState(actor->actorState.flags1, actor->actorState.flags2);
        if (!nativeState.LifeAllowsReplacement() || !nativeState.KnockIdle() || !nativeState.AttackIdle())
        {
            Log(intent, "pending-unsafe-actor-state");
            continue;
        }
        const auto localId = CreationSeatLocalFormId(index);
        auto* seat = localId ? Seat(*localId) : nullptr;
        if (!seat || !seat->baseForm || seat->baseForm->formType != FormType::Furniture || !seat->parentCell ||
            seat->IsDisabled() || seat->parentCell != actor->parentCell)
        {
            Log(intent, "pending-seat-or-cell");
            continue;
        }
        auto* manager = ModManager::Get();
        auto* mod = manager ? manager->GetByName("STRE_AlternateStart.esp") : nullptr;
        if (!mod || seat->parentCell->formID != mod->GetFormId(0x000012D1))
        {
            Log(intent, "rejected-seat-cell");
            continue;
        }
        intent.Index = index;
        intent.SeatForm = seat->formID;
        auto* process = actor->currentProcess;
        if (!process || !process->middleProcess)
        {
            Log(intent, "pending-process");
            continue;
        }
        const auto& papyrus = aWorld.ctx().at<PapyrusService>();
        const auto* inUse = papyrus.Get("ObjectReference", "IsFurnitureInUse");
        const auto* sitState = papyrus.Get("Actor", "GetSitState");
        if (!inUse || !sitState || !GameVM::Get() || !GameVM::Get()->virtualMachine)
        {
            Log(intent, "pending-native-functions");
            continue;
        }
        auto* occupied = TESObjectREFR::GetByHandle(process->middleProcess->occupiedFurniture.handle.iBits);
        const bool correct = occupied == seat;
        const bool seated = PapyrusFunction<int32_t, Actor>(sitState)(actor) == 3;
        const bool used = PapyrusFunction<bool, TESObjectREFR, bool>(inUse)(seat, false);
        const auto action = intent.Projection.Observe(actor->formID, reinterpret_cast<uintptr_t>(actor), true,
                                                      correct, seated, occupied && !correct, used);
        if (action == SeatAction::Complete)
        {
            Log(intent, correct && seated ? "seated" : "projection-already-completed");
            continue;
        }
        if (action == SeatAction::Conflict)
        {
            Log(intent, "pending-occupation-conflict");
            continue;
        }
        if (action == SeatAction::AwaitEntry)
        {
            Log(intent, std::chrono::steady_clock::now() - intent.IssuedAt > std::chrono::seconds(10) ?
                           "pending-entry-timeout-no-reactivation" : "pending-entry");
            continue;
        }
        if (action != SeatAction::Activate)
            continue;
        // One native request per current Actor token. The existing wrapper calls
        // RealActivate under ScopedActivateOverride; no ActivateRequest is sent.
        intent.Projection.Issued = true;
        intent.IssuedAt = std::chrono::steady_clock::now();
        const bool accepted = seat->Activate(actor, 0, nullptr, 1, 0);
        Log(intent, accepted ? "activation-issued" : "activation-rejected-no-reactivation");
    }
}
void Clear() noexcept { s_intents.clear(); }
}
