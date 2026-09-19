"""Individual Applied seating boundaries; source checks, not native execution."""
import json
import re
import unittest
from test_remote_respawn_lab import read, body, ROOT, CREATION, LAB
SEATING = read("Code/client/Services/Generic/CreationSeating.cpp")

class CreationSeating(unittest.TestCase):
    def test_local_completion_requires_animation_observation_not_sit_state(self):
        self.assertIn('observeLocal ? sample.Evidence.AnimationObserved() : seated', SEATING)
        self.assertIn('bool observeLocal = intent.Solo;', SEATING)
        self.assertIn('#ifndef MASTER\n        observeLocal = intent.Local;\n#endif', SEATING)
        self.assertIn('correct, entryConfirmed, occupied && !correct, used', SEATING)
        self.assertIn('furniture-logical-only', SEATING)
        self.assertIn('entry-animation-observed', SEATING)
        self.assertNotIn('? "seated"', SEATING)
        self.assertIn('visualConfirmation=human-required', SEATING)
        for name in ('isInFurniture', 'isIdleSitting', 'bAnimationDriven', 'bMotionDriven', 'bIsSynced'):
            self.assertIn(f'GraphBool(apActor, "{name}")', SEATING)
        self.assertIn('SeatingObservationWindow::SampleMs', SEATING)

    def test_native_events_cover_post_creation_and_never_mutate_the_engine(self):
        event = body(CREATION, 'BSTEventResult CharacterCreationService::OnEvent(const TESFurnitureEvent*')
        self.assertLess(event.index('CreationSeating::ObserveFurnitureEvent'), event.index('IsRaceMenuDiagnosticActive'))
        observe = body(SEATING, 'void ObserveFurnitureEvent(')
        self.assertNotIn('s_intents', observe)
        self.assertIn('s_solo.SeatToken !=', observe)
        self.assertIn('s_solo.ActorToken !=', observe)
        action = body(SEATING, 'void ObserveAnimationAction(')
        self.assertNotIn('s_intents', action)
        self.assertIn('s_solo.ActionBudget.Admit(now)', action)
        self.assertIn('++s_solo.ActionsDropped', action)
        hook = body(read('Code/client/Games/Animation.cpp'), 'uint8_t TP_MAKE_THISCALL(HookPerformAction,')
        self.assertLess(hook.index('ThisCall(RealPerformAction'), hook.index('ObserveAnimationAction(apAction, res, false)'))
        self.assertIn('ObserveAnimationAction(apAction, 0, true)', hook)
        for forbidden in ('SendAnimationEvent(', 'SetVariableBool(', 'SetVariableInt(', 'ForceAction(', 'SetDontMove', 'PlayIdle('):
            self.assertNotIn(forbidden, SEATING)

    def test_long_capture_survives_entry_completion_and_native_pending_guards(self):
        tick = body(SEATING, 'void Tick(')
        for exit_guard in ('pending-actor', 'pending-unsafe-actor-state', 'SeatAction::Complete', 'SeatAction::AwaitEntry'):
            self.assertLess(tick.index('PollSoloObservation(intent)'), tick.index(exit_guard))
        action = body(SEATING, 'void ObserveAnimationAction(')
        self.assertIn('SeatingObservationWindow::Active(now, started, entered)', action)
        event = body(SEATING, 'void ObserveFurnitureEvent(')
        self.assertIn('FirstEnterAt.compare_exchange_strong', event)
        self.assertIn('CaptureSoloSample(', event)
        self.assertIn('furniture-enter-immediate', event)
        self.assertNotIn('GetRunner', event)  # snapshot is synchronous, not next tick
        self.assertNotIn('SeatingObservationWindow::Active', event)  # first late Enter can reopen

    def test_process_trace_is_read_only_and_does_not_claim_path_solver_or_human_input_proof(self):
        for field in ('runOncePackage', 'packageProcedureIndex', 'runOnceTargetHandle', 'furniturePathPoint', 'furnitureIdle', 'MoveInputVec', 'LookInputVec'):
            self.assertIn(field, SEATING)
            self.assertNotRegex(SEATING, rf'->(?:\w+\.)?{field}\s*=(?!=)')
        self.assertIn('pathSolverStatus=unavailable', SEATING)
        self.assertIn('inputProvenance=not-proven', SEATING)
        self.assertIn('distance={} yawDelta={}', SEATING)
        capture = body(SEATING, 'void CaptureSoloSample(')
        self.assertIn('occupiedFurniture.handle.iBits', capture)
        self.assertIn('PapyrusFunction<int32_t, Actor>', capture)
        self.assertIn('ReadSoloSample(', capture)

    def test_applied_is_individual_and_server_hash_guards_remain(self):
        receive = body(SEATING, "void Receive(")
        self.assertIn("CharacterBuildNetworkState::Applied", receive)
        self.assertNotIn("Present", SEATING)
        self.assertNotIn("AllApplied", SEATING)
        server = read("Code/server/Services/CharacterBuildService.cpp")
        applied = body(server, "void CharacterBuildService::OnCharacterBuildAppliedRequest(")
        for guard in ("pBuild->Revision != acPacket.Packet.Revision", "acPacket.Packet.InventoryHash !=", "acPacket.Packet.SpellHash !="):
            self.assertLess(applied.index(guard), applied.index("pBuild->Applied = true"))
        self.assertIn("GetAdmission(aPlayer)", server)
        self.assertIn("AdmittedIdentity->Player.Value", server)

    def test_mapping_matches_actual_manifest(self):
        table = body(read("Code/common/CharacterCreation/CreationSeating.h"), "inline std::optional<uint32_t> CreationSeatLocalFormId(")
        ids = [int(x, 16) for x in re.findall(r"0x[0-9A-F]+", table)]
        records = json.loads((ROOT / "docs/features/alternate-start/CK_RECORDS_M7_IMPLEMENTED.json").read_text(encoding="utf-8"))["records"]
        lookup = {r["editorId"]: r for r in records}
        self.assertEqual(len(ids), 10)
        for i, form in enumerate(ids):
            self.assertEqual(form, int(lookup[f"STRE_FURN_PlayerSeat{i+1:02d}"]["expectedLocalFormId"], 16))
        self.assertIn("StandingCreationPositionIndex(players, intent.DurablePlayer)", SEATING)
        self.assertNotIn(".SlotId", SEATING)

    def test_remote_waits_for_committed_current_binding(self):
        self.assertIn("RemoteRespawnLab::CommittedActor", SEATING)
        query = body(LAB, "Actor* CommittedActor(")
        for guard in ("FinalBuildRevision != aRevision", "MaterializationState::Committed", "Bound(aWorld, job, job.Candidate)"):
            self.assertIn(guard, query)
        self.assertNotIn("Activate", query)
        self.assertIn('pending-actor', SEATING)

    def test_occupancy_checked_before_single_activation_and_only_scoped_local_approach_can_move(self):
        self.assertEqual(SEATING.count("->Activate("), 1)
        self.assertLess(SEATING.index('papyrus.Get("ObjectReference", "IsFurnitureInUse")'), SEATING.index("->Activate("))
        self.assertLess(SEATING.index("intent.Projection.Issued = true"), SEATING.index("->Activate("))
        self.assertIn("(seat, false)", SEATING)  # include reservations
        self.assertIn("occupiedFurniture.handle.iBits", SEATING)
        for forbidden in ("SetPosition(", "SetStage(", "SetActorState", ".Send(", "SwitchRace", "Reset3D"):
            self.assertNotIn(forbidden, SEATING)
        tick = body(SEATING, "void Tick(")
        self.assertIn('#ifndef MASTER\n        if (intent.Local && !PrepareLocalApproach(intent, actor, seat))', tick)
        self.assertLess(tick.index('PrepareLocalApproach(intent'), tick.index('->Activate('))
        self.assertNotIn('MoveTo(', tick)
        approach = body(SEATING, 'bool PrepareLocalApproach(')
        self.assertEqual(SEATING.count('->MoveTo('), 1)
        self.assertIn('LocalSeatApproachRejection(aIntent.Local,', approach)
        for native_guard in ('apActor == PlayerCharacter::Get()', 'TESForm::GetById(0x14) == apActor', 'apActor->GetExtension()->IsRemote(), aIntent.Index'):
            self.assertIn(native_guard, approach)
        self.assertNotIn('aIntent.Solo', approach)
        self.assertNotIn('aIntent.Server != 0', approach)
        self.assertLess(approach.index('LocalSeatApproachRejection('), approach.index('->MoveTo('))
        self.assertLess(approach.index('approach.Start('), approach.index('->MoveTo('))
        self.assertIn('NiPoint3{target->Position}', approach)
        self.assertNotIn('MoveTo(apSeat->parentCell, apSeat->position)', approach)
        for phase in ('before-approach-move', 'after-approach-move-request', 'approach-arrived-before-facing', 'approach-facing-applied'):
            self.assertIn(phase, approach)

    def test_approach_barrier_uses_update_events_not_repeated_tick_calls_or_sleep(self):
        update = body(CREATION, 'void CharacterCreationService::OnUpdate(')
        self.assertEqual(update.count('CreationSeating::OnUpdate(m_world)'), 1)
        finalize = body(CREATION, 'void CharacterCreationService::FinalizeCompletedBuild(')
        self.assertNotIn('CreationSeating::OnUpdate', finalize)
        self.assertIn('++s_engineUpdate', body(SEATING, 'void OnUpdate('))
        self.assertNotIn('++s_engineUpdate', body(SEATING, 'void Tick('))
        self.assertNotIn('sleep', body(SEATING, 'bool PrepareLocalApproach(').lower())

    def test_existing_markers_supply_exact_targets_without_furniture_geometry(self):
        approach = body(SEATING, 'bool PrepareLocalApproach(')
        common = read('Code/common/CharacterCreation/SeatApproach.h')
        scope = body(common, 'inline const char* LocalSeatApproachRejection(')
        self.assertIn('aRank >= 10', scope)
        mapping = body(common, 'inline std::optional<SeatApproachBinding> ResolveSeatApproachBinding(')
        for source in ('CreationSeatLocalFormId(aRank)', 'CreationMarkerLocalFormId(aRank)'):
            self.assertIn(source, mapping)
        self.assertIn('Seat(binding->Marker)', approach)
        self.assertIn('Seat(binding->Seat) != apSeat', approach)
        self.assertIn('SeatApproachMarkerTarget(marker->position, marker->rotation)', approach)
        self.assertIn('MoveTo(marker->parentCell, NiPoint3{target->Position})', approach)
        self.assertIn('SetRotation(approach.Target.Rotation.x, approach.Target.Rotation.y, approach.Target.Rotation.z)', approach)
        self.assertIn('approach.Target.Position == target->Position && approach.Target.Rotation == target->Rotation', approach)
        for guard in ('approach-marker-missing', 'approach-marker-not-xmarkerheading', 'approach-marker-disabled',
                      'approach-marker-cell-mismatch', 'approach-marker-transform-invalid'):
            self.assertIn(guard, approach)
        for forbidden in ('atan2', 'std::sin', 'std::cos', 'ApproachTable', 'ApproachNeighbor', 'Seat01ApproachTarget', 'Seat02ApproachTarget'):
            self.assertNotIn(forbidden, approach + common)
        for stage in ('approach.Start(', '->MoveTo(', 'approach.Observe(', '->SetRotation('):
            self.assertEqual(approach.count(stage), 1)
        self.assertIn('assignedApproachName=Marker{:02}', SEATING)

    def test_existing_marker_seat_pairs_match_the_actual_plugin(self):
        import audit_seat_approach_markers as audit
        rows, errors = audit.audit((ROOT / 'GameFiles/Skyrim/STRE_AlternateStart.esp').read_bytes())
        self.assertEqual(errors, [])
        self.assertEqual(len(rows), 10)
        for index, row in enumerate(rows):
            self.assertEqual(row['rank'], index)
            self.assertEqual(row['editorId'], f'STRE_REFR_PlayerCreationMarker{index+1:02d}')
        self.assertEqual(audit.check_bindings(rows), [])

    def test_connected_local_fences_precede_observation_and_approach(self):
        tick = body(SEATING, 'void Tick(')
        for guard in ('pending-recovery-lock', 'pending-session-changed', 'pending-disconnected',
                      'pending-campaign', 'rejected-player-id', 'pending-local-finalization',
                      'rejected-local-transport-identity', 'rejected-local-identity'):
            self.assertLess(tick.index(guard), tick.index('PollSoloObservation(intent)'))
            self.assertLess(tick.index(guard), tick.index('PrepareLocalApproach(intent'))
        self.assertIn('intent.Player != aWorld.GetTransport().GetLocalPlayerId()', tick)
        self.assertIn('*durable != intent.DurablePlayer.c_str()', tick)
        finalize = body(SEATING, 'void FinalizeLocal(')
        self.assertIn('found->second.Revision == aRevision', finalize)
        local_applied = body(CREATION, 'void CharacterCreationService::OnNotifyCharacterBuildState(')
        for guard in ('acMessage.PlayerId ==', 'acMessage.State != CharacterBuildNetworkState::Applied',
                      'acMessage.Revision != m_serverBuildRevision', 'acMessage.ServerId != m_serverCharacterId'):
            self.assertIn(guard, local_applied)

    def test_local_probe_context_is_immutable_and_callbacks_remain_read_only(self):
        context = body(SEATING, 'std::string IntentContext(')
        for field in ('sessionConnected={}', 'localPlayer={}', 'durablePlayerId={}', 'creationPositionIndex={}', 'assignedSeat={:X}'):
            self.assertIn(field, context)
        begin = body(SEATING, 'void BeginSoloObservation(')
        self.assertIn('s_observationContext.store(', begin)
        self.assertIn('std::make_shared<const std::string>(IntentContext(aIntent))', begin)
        self.assertIn('std::atomic<std::shared_ptr<const std::string>>', SEATING)
        self.assertIn('s_observationContext.load()', body(SEATING, 'std::string ObservationContext('))
        for callback in ('void ObserveFurnitureEvent(', 'void ObserveAnimationAction('):
            source = body(SEATING, callback)
            self.assertIn('ObservationContext()', source)
            self.assertNotIn('IntentContext(', source)
            self.assertNotIn('s_intents', source)
        for phase in ('prototype-triggered', 'prototype-rejected', 'approach-target', 'furniture-enter-immediate'):
            self.assertIn(phase, SEATING)

    def test_local_finalize_unlocks_before_seating_and_solo_has_no_network_dependency(self):
        finalize = body(CREATION, "void CharacterCreationService::FinalizeCompletedBuild(")
        self.assertLess(finalize.index("UnlockCharacterCreationControls()"), finalize.index("CreationSeating::FinalizeLocal"))
        self.assertLess(finalize.index("SetStopped()"), finalize.index("CreationSeating::FinalizeLocal"))
        self.assertLess(finalize.index("CreationSeating::FinalizeLocal"), finalize.index("ResetNetworkBuildState()"))
        self.assertIn("CreationSeating::Tick(m_world)", finalize)
        self.assertIn("solo.Local = solo.Solo = true", SEATING)
        self.assertIn("size_t index{}", SEATING)
        self.assertNotIn("kBuildSealSeconds", CREATION)

    def test_recovery_disconnect_and_duplicate_are_fenced(self):
        self.assertIn("pending-recovery-lock", SEATING)
        self.assertIn("rejected-identity-or-revision", SEATING)
        self.assertIn("rejected-duplicate-durable-binding", SEATING)
        self.assertIn("pending-entry-timeout-no-reactivation", SEATING)
        self.assertIn("CreationSeating::Clear()", body(CREATION, "void CharacterCreationService::OnDisconnected("))
        self.assertIn("CreationSeating::Clear()", body(CREATION, "bool CharacterCreationService::ResetForFreshCharacterCreation("))

    def test_wire_extension_is_bounded_and_transactional(self):
        source = read("Code/encoding/Messages/NotifyCharacterBuildState.cpp")
        self.assertIn("size > MaxSeatingIdentityBytes", source)
        self.assertIn("version != 1", source)
        self.assertLess(source.index("!aReader.ReadBytes"), source.index("SeatingCampaignId = std::move"))
