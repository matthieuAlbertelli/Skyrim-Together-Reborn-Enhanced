"""Remote seating observation boundaries; no claim of native visual success."""
import unittest
from test_remote_respawn_lab import read, body

SEATING = read('Code/client/Services/Generic/CreationSeating.cpp')
PROBE = read('Code/client/Services/Generic/RemoteSeatingProbe.cpp')
CHARACTER = read('Code/client/Services/Generic/CharacterService.cpp')
ANIMATION = read('Code/client/Systems/AnimationSystem.cpp')


class RemoteSeatingProjection(unittest.TestCase):
    def test_remote_branch_cannot_reach_activation_or_local_approach(self):
        tick = body(SEATING, 'void Tick(')
        remote = body(tick, 'if (!intent.Local)')
        self.assertIn('RemoteSeatingProbe::Binding(', remote)
        self.assertIn('continue;', remote)
        self.assertNotIn('->Activate(', remote)
        self.assertNotIn('PrepareLocalApproach', remote)
        self.assertLess(tick.index('if (!intent.Local)'), tick.index('PrepareLocalApproach'))
        self.assertLess(tick.index('actor != PlayerCharacter::Get()'), tick.index('->Activate('))
        self.assertLess(tick.index('actor->GetExtension()->IsRemote()'), tick.index('->Activate('))
        self.assertEqual(SEATING.count('->Activate('), 1)

    def test_receipt_is_traced_on_both_filter_and_enqueue_paths(self):
        receive = body(CHARACTER, 'void CharacterService::OnReferencesMoveRequest(')
        filtered = body(receive, 'if (itor == std::end(view))')
        self.assertIn('received-filtered-missing-remote-animation-view', filtered)
        self.assertLess(filtered.index('RemoteSeatingProbe::Received'), filtered.index('continue;'))
        self.assertLess(receive.index('"received-enqueue"'), receive.index('TimePoints.push_back(action)'))
        self.assertIn('static_cast<uint32_t>(*itor)', receive)

    def test_existing_force_action_is_bracketed_without_another_replay(self):
        update = body(ANIMATION, 'void AnimationSystem::Update(')
        self.assertEqual(update.count('ActorMediator::Get()->ForceAction('), 1)
        self.assertLess(update.index('"action-dequeued"'), update.index('apActor->actorState.flags1 ='))
        self.assertLess(update.index('"action-dequeued"'), update.index('ForceAction('))
        self.assertLess(update.index('ForceAction('), update.index('"force-action-return"'))
        self.assertLess(update.index('"force-action-return"'), update.index('actions.pop_front()'))
        for phase in ('"graph-not-ready"', '"action-not-due"'):
            self.assertIn(phase, update)
        self.assertIn('MissingUpdateActor', body(CHARACTER, 'void CharacterService::RunRemoteUpdates('))

    def test_queue_destruction_is_observed_before_existing_mutation(self):
        setup = body(ANIMATION, 'void AnimationSystem::Setup(')
        clean = body(ANIMATION, 'void AnimationSystem::Clean(')
        self.assertLess(setup.index('RemoteSeatingProbe::QueueReset'), setup.index('emplace_or_replace'))
        self.assertLess(clean.index('RemoteSeatingProbe::QueueReset'), clean.index('remove<'))
        self.assertIn('last-force-action-at-binding', PROBE)
        self.assertIn('queue-at-binding', PROBE)

    def test_probe_cannot_mutate_native_or_network_or_animation_queue(self):
        for forbidden in ('->Activate(', '->MoveTo(', 'ForceAction(', 'SetVariable', 'SetRotation(',
                          'SendAnimationEvent(', '.Send(', 'TimePoints.push', 'pop_front(',
                          'TimePoints.clear', 'emplace_or_replace', 'g_forceAnimation ='):
            self.assertNotIn(forbidden, PROBE)
        self.assertNotRegex(PROBE, r'actorState\.flags[12]\s*=(?!=)')
        self.assertNotIn('Services/RemoteRespawnLab.h', PROBE)
        self.assertNotIn('const_cast', PROBE)

    def test_observation_has_fixed_bounds_and_session_reset(self):
        self.assertIn('#if (!IS_MASTER)', PROBE)
        self.assertIn('std::array<std::atomic<std::shared_ptr<Trace>>, 10>', PROBE)
        self.assertIn('SeatingObservationWindow::Active', PROBE)
        self.assertIn('>= 512', PROBE)
        self.assertIn('t.Budget.Admit(now)', PROBE)
        self.assertIn('droppedDiagnosticLines={}', PROBE)
        applied = body(PROBE, 'void Applied(')
        self.assertLess(applied.index('t->Identity.load()'), applied.index('t->Started ='))
        self.assertIn('!t->SawCommit.exchange(true)', body(PROBE, 'void Binding('))
        self.assertIn('slot.exchange(nullptr)', body(PROBE, 'void Clear('))
        self.assertIn('RemoteSeatingProbe::Clear()', body(SEATING, 'void Clear('))

    def test_actual_action_target_is_fresh_and_distinct_from_observed_binding(self):
        replay = body(PROBE, 'void Replay(')
        self.assertIn('Find(world, actor)', replay)
        for field in ('actualActor={:X}', 'actualActorToken={:X}', 'actionTick={}',
                      'committedObserved={}', 'matchesObservedCommitted={}'):
            self.assertIn(field, PROBE)
        self.assertIn('RemoteComponent, PlayerComponent', body(PROBE, 'void Received('))
        self.assertIn('Track(server)', body(PROBE, 'void Received('))  # Before Applied too.
        self.assertIn('receivedBeforeApplied={}', PROBE)

    def test_native_callbacks_use_only_published_value_context(self):
        for signature in ('void NativeAction(', 'void Furniture('):
            callback = body(PROBE, signature)
            self.assertIn('Find(', callback)
            self.assertNotIn('world.', callback)
            self.assertNotIn('s_intents', callback)
        self.assertIn('RemoteSeatingProbe::Furniture(apEvent)', body(SEATING, 'void ObserveFurnitureEvent('))
        self.assertIn('RemoteSeatingProbe::NativeAction(', body(SEATING, 'void ObserveAnimationAction('))


if __name__ == '__main__':
    unittest.main()
