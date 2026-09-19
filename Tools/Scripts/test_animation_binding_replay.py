"""Binding replay integration boundaries; native pose still needs human validation."""
import unittest
from test_remote_respawn_lab import ROOT, read, body

ANIMATION = read('Code/client/Systems/AnimationSystem.cpp')
LAB = read('Code/client/Services/Generic/RemoteRespawnLab.cpp')
SERVICE = read('Code/client/Services/Generic/CharacterService.cpp')
HISTORY = read('Code/encoding/Animation/BindingActionReplay.cpp')


class AnimationBindingReplay(unittest.TestCase):
    def test_commit_only_signals_values_after_publication(self):
        signal = LAB.index('AnimationSystem::OnBindingChanged(')
        self.assertLess(LAB.index('Log(aJob, "candidate-commit")'), signal)
        self.assertLess(LAB.index('CachedRefId = aJob.Candidate.Forms.Actor'), signal)
        self.assertLess(signal, LAB.index('DispatchRetirement(aJob);', signal))
        self.assertEqual(LAB.count('AnimationSystem::'), 1)
        for field in ('Key().Session', 'Key().Generation', 'aJob.Old.ActorToken', 'aJob.Candidate.ActorToken'):
            self.assertIn(field, LAB[signal:signal + 400])
        for forbidden in ('FormRefinedReplayChain', 'AddActionsForReplay', 'ForceAction(', 'get<RemoteAnimationComponent>'):
            self.assertNotIn(forbidden, LAB)

    def test_cache_rules_have_one_shared_implementation(self):
        for name in ('ActionReplayCache', 'AnimationEventLists'):
            self.assertTrue((ROOT / f'Code/encoding/Animation/{name}.cpp').exists())
            self.assertFalse((ROOT / f'Code/server/Game/Animation/{name}.cpp').exists())
        self.assertIn('<Animation/ActionReplayCache.h>', read('Code/server/Components/AnimationComponent.h'))
        for text in (ANIMATION, HISTORY):
            for special in ('IdleChairRightEnter', 'IdleChairEnterInstant', '3B070', 'Seat02', 'Seat01'):
                self.assertNotIn(special, text)

    def test_no_new_native_primitive_or_actorstate_write(self):
        self.assertEqual(ANIMATION.count('ActorMediator::Get()->ForceAction('), 1)
        self.assertEqual(ANIMATION.count('apActor->actorState.flags1 ='), 1)
        self.assertEqual(ANIMATION.count('apActor->actorState.flags2 ='), 1)
        for text in (ANIMATION, HISTORY):
            for forbidden in ('->Activate(', '->MoveTo(', 'SwitchRace(', 'Reset3D(', 'SendAnimationEvent(', '.Send('):
                self.assertNotIn(forbidden, text)
        signal = body(ANIMATION, 'void AnimationSystem::OnBindingChanged(')
        for forbidden in ('TESForm::GetById', 'ForceAction(', 'actorState', 'LoadAnimationVariables'):
            self.assertNotIn(forbidden, signal)

    def test_functional_replay_keeps_all_build_scope_and_normal_graph_guard(self):
        self.assertNotIn('IS_MASTER', HISTORY)
        self.assertNotIn('IS_MASTER', ANIMATION)
        update = body(ANIMATION, 'void AnimationSystem::Update(')
        self.assertLess(update.index('ObserveBinding('), update.index('const auto it ='))
        self.assertLess(update.index('CancelForPendingExit('), update.index('const auto it ='))
        self.assertLess(update.index('animationGraphHolder.IsReady()'), update.index('ForceAction('))
        self.assertLess(update.index('ForceAction('), update.index('bindingReplay.Consumed(first)'))
        self.assertLess(update.index('bindingReplay.Consumed(first)'), update.index('actions.pop_front()'))

    def test_disconnect_recovery_and_new_session_invalidate_history(self):
        for method in ('OnConnected', 'OnDisconnected', 'OnUpdate'):
            self.assertIn('InvalidateReplayContexts', body(SERVICE, f'void CharacterService::{method}('))
        update = body(ANIMATION, 'void AnimationSystem::Update(')
        self.assertLess(update.index('gate->IsLocked()'), update.index('ForceAction('))
        self.assertIn('bindingReplay.Invalidate', update)

    def test_bounded_evidence_is_independent_of_native_probe_budget(self):
        for phase in ('cache-source', 'replay-injected', 'replay-force-action-return', 'replay-graph-pending'):
            self.assertIn(f'phase={phase}', ANIMATION)
        self.assertIn('bindingReplay.Remaining()', ANIMATION)
        self.assertIn('bindingReplay.LogWaitOnce()', ANIMATION)
        self.assertIn('matchesNewBinding={}', ANIMATION)
        self.assertNotIn('RemoteSeatingProbe', HISTORY)


if __name__ == '__main__':
    unittest.main()
