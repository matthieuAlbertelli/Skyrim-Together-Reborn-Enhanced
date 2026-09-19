#include <TiltedOnlinePCH.h>

#include <Systems/AnimationSystem.h>
#include <Services/RemoteSeatingProbe.h>
#include <Services/CampaignRuntimeGateService.h>
#include <Services/TransportService.h>

#include <Games/Animation/TESActionData.h>
#include <Games/Animation/ActorMediator.h>

#include <Games/References.h>

#include <Forms/BGSAction.h>
#include <AI/AIProcess.h>
#include <Misc/MiddleProcess.h>

#include <Messages/ClientReferencesMoveRequest.h>

#include <Components.h>
#include <World.h>

#include <Forms/TESObjectCELL.h>
#include <Forms/TESWorldSpace.h>

extern thread_local const char* g_animErrorCode;

void AnimationSystem::OnBindingChanged(World& aWorld, entt::entity aEntity, const AnimationBindingChange& aChange) noexcept
{
    auto* animation = aWorld.try_get<RemoteAnimationComponent>(aEntity);
    const auto* remote = aWorld.try_get<RemoteComponent>(aEntity);
    const auto* form = aWorld.try_get<FormIdComponent>(aEntity);
    const auto* gate = CampaignRuntimeGateService::TryGet();
    if (!animation || !remote || !form || static_cast<uint32_t>(aEntity) != aChange.EntityVersioned ||
        remote->Id != aChange.ServerId || remote->CachedRefId != aChange.New.FormId || form->Id != aChange.New.FormId ||
        !aWorld.GetTransport().IsConnected() || (gate && gate->IsLocked()))
    {
        spdlog::warn("[STRE][AnimationReplay] phase=binding-rejected serverId={} entityVersioned={} session={} generation={} reason=current-binding-or-context-invalid",
                     aChange.ServerId, aChange.EntityVersioned, aChange.Session, aChange.Generation);
        return;
    }
    const auto source = animation->BindingReplay.Cache().GetActions(); // <=32 value-owned actions.
    const auto pending = animation->TimePoints.size();
    const auto rebuilt = animation->BindingReplay.Rebind(aChange, animation->TimePoints, animation->ReplayCount, animation->ResetAnimationGraphForReplay);
    if (!rebuilt.Accepted && std::string_view(rebuilt.Reason) == "duplicate-or-stale-generation")
        return;
    spdlog::info("[STRE][AnimationReplay] phase={} serverId={} entityVersioned={} session={} generation={} oldActor={:X} oldToken={:X} "
                 "newActor={:X} newToken={:X} source=consumed-cache sourceCount={} pendingBefore={} injected={} pendingAfter={} resetGraph={} reason={}",
                 rebuilt.Accepted ? "binding-reconstructed" : "binding-rejected", aChange.ServerId, aChange.EntityVersioned,
                 aChange.Session, aChange.Generation, aChange.Old.FormId, aChange.Old.Token, aChange.New.FormId, aChange.New.Token,
                 rebuilt.SourceCount, pending, rebuilt.Chain.Actions.size(), animation->TimePoints.size(), rebuilt.Chain.ResetAnimationGraph, rebuilt.Reason);
    // Dedicated bounded evidence, independent of native-action spam in RemoteProbe.
    for (size_t i = 0; rebuilt.Accepted && i < source.size(); ++i)
        spdlog::info("[STRE][AnimationReplay] phase=cache-source serverId={} generation={} index={} actionTick={} event={} idle={:X}",
                     aChange.ServerId, aChange.Generation, i, source[i].Tick, source[i].EventName.c_str(), source[i].IdleId);
    for (size_t i = 0; i < rebuilt.Chain.Actions.size(); ++i)
    {
        const auto& action = rebuilt.Chain.Actions[i];
        spdlog::info("[STRE][AnimationReplay] phase=replay-injected serverId={} generation={} index={} actionTick={} event={} targetEvent={} action={:X} idle={:X} "
                     "newActor={:X} newToken={:X}", aChange.ServerId, aChange.Generation, i, action.Tick, action.EventName.c_str(),
                     action.TargetEventName.c_str(), action.ActionId, action.IdleId, aChange.New.FormId, aChange.New.Token);
    }
}

void AnimationSystem::InvalidateReplayContexts(World& aWorld, const char* aReason) noexcept
{
    for (auto entity : aWorld.view<RemoteAnimationComponent>())
    {
        auto& animation = aWorld.get<RemoteAnimationComponent>(entity);
        if (animation.BindingReplay.Remaining())
            spdlog::info("[STRE][AnimationReplay] phase=replay-invalidated entityVersioned={} remaining={} reason={}",
                         static_cast<uint32_t>(entity), animation.BindingReplay.Remaining(), aReason);
        animation.BindingReplay.Invalidate(animation.TimePoints, animation.ReplayCount, animation.ResetAnimationGraphForReplay);
    }
}

void AnimationSystem::Update(World& aWorld, Actor* apActor, RemoteAnimationComponent& aAnimationComponent, const uint64_t aTick) noexcept
{
    auto& actions = aAnimationComponent.TimePoints;
    auto& bindingReplay = aAnimationComponent.BindingReplay;
    const auto* gate = CampaignRuntimeGateService::TryGet();
    if (!aWorld.GetTransport().IsConnected() || (gate && gate->IsLocked()))
        bindingReplay.Invalidate(actions, aAnimationComponent.ReplayCount, aAnimationComponent.ResetAnimationGraphForReplay);
    const AnimationBinding actualBinding{apActor->formID, reinterpret_cast<uintptr_t>(apActor)};
    bindingReplay.ObserveBinding(actualBinding, actions, aAnimationComponent.ReplayCount, aAnimationComponent.ResetAnimationGraphForReplay);
    if (bindingReplay.CancelForPendingExit(actions, aAnimationComponent.ReplayCount, aAnimationComponent.ResetAnimationGraphForReplay))
        spdlog::info("[STRE][AnimationReplay] phase=replay-cancelled serverId={} generation={} reason=newer-pending-exit",
                     bindingReplay.Change().ServerId, bindingReplay.Change().Generation);

    const auto it = std::begin(actions);
    if (it != std::end(actions) && it->Tick > aTick)
        STRE::RemoteSeatingProbe::Replay(aWorld, apActor, *it, "action-not-due", -1, actions.size(), aTick);
    if (it != std::end(actions) && it->Tick <= aTick)
    {
        // Check if animation graph is ready before attempting to play animations
        if (!apActor->animationGraphHolder.IsReady())
        {
            if (bindingReplay.Remaining() && bindingReplay.LogWaitOnce())
                spdlog::info("[STRE][AnimationReplay] phase=replay-graph-pending serverId={} generation={} actor={:X} token={:X} remaining={}",
                             bindingReplay.Change().ServerId, bindingReplay.Change().Generation, actualBinding.FormId, actualBinding.Token, bindingReplay.Remaining());
            STRE::RemoteSeatingProbe::Replay(aWorld, apActor, *it, "graph-not-ready", -1, actions.size(), aTick);
            // Animation graph not ready, keep the action in queue and try again later
            return;
        }
        if (aAnimationComponent.ReplayCount > 0 && aAnimationComponent.ResetAnimationGraphForReplay)
        {
            apActor->animationGraphHolder.RevertAnimationGraphManager();
            aAnimationComponent.ResetAnimationGraphForReplay = false;
        }

        const auto& first = *it;

        const auto actionId = first.ActionId;
        const auto targetId = first.TargetId;

        const auto pAction = Cast<BGSAction>(TESForm::GetById(actionId));
        const auto pTarget = Cast<TESObjectREFR>(TESForm::GetById(targetId));

        STRE::RemoteSeatingProbe::Replay(aWorld, apActor, first, "action-dequeued", -1, actions.size(), aTick);

        apActor->actorState.flags1 = first.State1;
        apActor->actorState.flags2 = first.State2;

        apActor->LoadAnimationVariables(first.Variables);

        aAnimationComponent.LastRanAction = first;

        // Play the animation
        TESActionData actionData(first.Type & 0x3, apActor, pAction, pTarget);
        actionData.eventName = BSFixedString(first.EventName.c_str());
        actionData.idleForm = Cast<TESIdleForm>(TESForm::GetById(first.IdleId));
        actionData.someFlag = ((first.Type & 0x4) != 0) ? 1 : 0;

        const auto result = ActorMediator::Get()->ForceAction(&actionData);
        STRE::RemoteSeatingProbe::Replay(aWorld, apActor, first, "force-action-return", result, actions.size(), aTick);
        if (bindingReplay.Remaining())
            spdlog::info("[STRE][AnimationReplay] phase=replay-force-action-return serverId={} entityVersioned={} session={} generation={} "
                         "actor={:X} token={:X} matchesNewBinding={} actionTick={} event={} idle={:X} result={} remaining={}",
                         bindingReplay.Change().ServerId, bindingReplay.Change().EntityVersioned, bindingReplay.Change().Session,
                         bindingReplay.Change().Generation, actualBinding.FormId, actualBinding.Token, actualBinding == bindingReplay.Change().New,
                         first.Tick, first.EventName.c_str(), first.IdleId, result, bindingReplay.Remaining() - 1);
        bindingReplay.Consumed(first);

        if (aAnimationComponent.ReplayCount > 0)
            aAnimationComponent.ReplayCount--;

        actions.pop_front();
    }
}

void AnimationSystem::Setup(World& aWorld, const entt::entity aEntity) noexcept
{
    STRE::RemoteSeatingProbe::QueueReset(aWorld, static_cast<uint32_t>(aEntity), "setup");
    aWorld.emplace_or_replace<RemoteAnimationComponent>(aEntity);
}

void AnimationSystem::Clean(World& aWorld, const entt::entity aEntity) noexcept
{
    STRE::RemoteSeatingProbe::QueueReset(aWorld, static_cast<uint32_t>(aEntity), "clean");
    if (aWorld.all_of<RemoteAnimationComponent>(aEntity))
        aWorld.remove<RemoteAnimationComponent>(aEntity);
}

void AnimationSystem::AddActionsForReplay(RemoteAnimationComponent& aAnimationComponent,
                                          const ActionReplayChain& acReplay) noexcept
{
    aAnimationComponent.TimePoints.insert(aAnimationComponent.TimePoints.end(), acReplay.Actions.begin(),
                                          acReplay.Actions.end());
    aAnimationComponent.ReplayCount = acReplay.Actions.size();
    aAnimationComponent.ResetAnimationGraphForReplay = acReplay.ResetAnimationGraph;
}

void AnimationSystem::AddAction(RemoteAnimationComponent& aAnimationComponent, const std::string& acActionDiff) noexcept
{
    auto itor = std::begin(aAnimationComponent.TimePoints);
    const auto end = std::cend(aAnimationComponent.TimePoints);

    auto& lastProcessedAction = aAnimationComponent.LastProcessedAction;

    TiltedPhoques::ViewBuffer buffer((uint8_t*)acActionDiff.data(), acActionDiff.size());
    Buffer::Reader reader(&buffer);

    lastProcessedAction.ApplyDifferential(reader);

    aAnimationComponent.TimePoints.push_back(lastProcessedAction);
}

void AnimationSystem::Serialize(World& aWorld, ClientReferencesMoveRequest& aMovementSnapshot, LocalComponent& localComponent, LocalAnimationComponent& animationComponent, FormIdComponent& formIdComponent)
{
    const auto pForm = TESForm::GetById(formIdComponent.Id);
    const auto pActor = Cast<Actor>(pForm);
    if (!pActor)
        return;

    auto& update = aMovementSnapshot.Updates[localComponent.Id];
    auto& movement = update.UpdatedMovement;

    if (const auto pCell = pActor->parentCell)
        World::Get().GetModSystem().GetServerModId(pCell->formID, movement.CellId.ModId, movement.CellId.BaseId);

    if (const auto pWorldSpace = pActor->GetWorldSpace())
        World::Get().GetModSystem().GetServerModId(pWorldSpace->formID, movement.WorldSpaceId.ModId, movement.WorldSpaceId.BaseId);

    movement.Position = pActor->position;

    movement.Rotation.x = pActor->rotation.x;
    movement.Rotation.y = pActor->rotation.z;

    pActor->SaveAnimationVariables(movement.Variables);

    if (pActor->currentProcess && pActor->currentProcess->middleProcess)
    {
        movement.Direction = pActor->currentProcess->middleProcess->direction;
    }

    for (auto& entry : animationComponent.Actions)
    {
        update.ActionEvents.push_back(entry);
    }

    auto latestAction = animationComponent.GetLatestAction();

    if (latestAction)
        localComponent.CurrentAction = latestAction.MoveResult();

    animationComponent.Actions.clear();
}

bool AnimationSystem::Serialize(World& aWorld, const ActionEvent& aActionEvent, const ActionEvent& aLastProcessedAction, std::string* apData)
{
    uint32_t actionBaseId = 0;
    uint32_t actionModId = 0;
    if (!aWorld.GetModSystem().GetServerModId(aActionEvent.ActionId, actionModId, actionBaseId))
        return false;

    uint32_t targetBaseId = 0;
    uint32_t targetModId = 0;
    if (!aWorld.GetModSystem().GetServerModId(aActionEvent.TargetId, targetModId, targetBaseId))
        return false;

    uint8_t scratch[1 << 14];
    TiltedPhoques::ViewBuffer buffer(scratch, std::size(scratch));
    Buffer::Writer writer(&buffer);
    aActionEvent.GenerateDifferential(aLastProcessedAction, writer);

    apData->assign(buffer.GetData(), buffer.GetData() + writer.Size());

    return true;
}
