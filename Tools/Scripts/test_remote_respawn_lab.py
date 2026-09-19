"""Final creation rematerialization contracts. No Skyrim/native validation."""
from pathlib import Path
import re
import unittest

ROOT = Path(__file__).resolve().parents[2]


def read(path):
    return (ROOT / path).read_text(encoding="utf-8")


def body(source, marker):
    start = source.index("{", source.index(marker))
    depth = 1
    end = start + 1
    while depth:
        depth += (source[end] == "{") - (source[end] == "}")
        end += 1
    return source[start:end]


LAB = read("Code/client/Services/Generic/RemoteRespawnLab.cpp")
SERVICE = read("Code/client/Services/Generic/CharacterService.cpp")
CREATION = read("Code/client/Services/Generic/CharacterCreationService.cpp")
PROBE = read("Code/client/Services/Generic/CharacterAppearanceProbe.cpp")
DISCOVERY = read("Code/client/Services/Generic/DiscoveryService.cpp")
ACTOR = read("Code/client/Games/Skyrim/Actor.cpp")


class FinalRespawnContract(unittest.TestCase):
    def test_pending_admission_is_read_only_and_runs_all_binding_guards_first(self):
        capture = body(LAB, 'FinalRespawnAdmissionDecision Capture(')
        decision = capture.index('aJob.Admission.Observe(')
        for guard in ('provenance-mismatch', 'entity-alias-collision', 'base-alias-collision',
                      'remote-animation-missing', 'root-not-ready', 'waiting-for-3d',
                      'assignment-pending', 'cell-lookup-mismatch', 'worldspace-lookup-mismatch',
                      'unresolvable-final-race', 'invalid-canonical-snapshot', 'invalid-canonical-applied-build',
                      'pre-reservation-session-changed'):
            self.assertLess(capture.index('"' + guard + '"'), decision)
        pending = body(capture, 'if (admission.Decision == FinalRespawnAdmissionDecision::Pending)')
        self.assertIn('return admission.Decision', pending)
        for mutation in ('aJob.Old =', 'aJob.Entity =', 'aJob.Values =', 'aJob.Position =', 'aJob.Rotation ='):
            self.assertGreater(capture.index(mutation), capture.index('"admission-ready"'))
        for forbidden in ('Materialize(', '.Reserve(', 'MoveTo(', 'Activate(', 'Retire(', 'DispatchRetirement(', 'SetWeaponDrawn'):
            self.assertNotIn(forbidden, capture)
        advance = body(LAB, 'void Advance(')
        check = body(advance, 'if (admission != FinalRespawnAdmissionDecision::Ready)')
        self.assertIn('aJob.Done = admission == FinalRespawnAdmissionDecision::Rejected', check)
        self.assertIn('return;', check)
        self.assertLess(advance.index('Capture(aWorld, aJob)'), advance.index('.Reserve('))

    def test_snapshot_and_deadline_are_frozen_and_cancellation_has_no_native_side_effects(self):
        receive = body(LAB, 'void ReceiveFinal(')
        first = body(receive, 'if (!job.Final)')
        self.assertEqual(receive.count('job.Final = aFinal'), 1)
        self.assertIn('job.Final = aFinal', first)
        self.assertIn('job.Admission.Begin(Clock::now())', first)
        self.assertEqual(LAB.count('Admission.Begin('), 1)
        self.assertIn('CharacterAppearanceUpdate::operator==(aFinal)', receive)
        self.assertIn('"canonical-final-changed"', receive)
        self.assertIn('"canonical-applied-build-changed"', body(LAB, 'void ReceiveBuild('))
        cancel = body(LAB, 'void CancelAdmission(')
        for forbidden in ('Lifecycle.', 'Retire(', 'DispatchRetirement(', 'Delete(', 'Materialize('):
            self.assertNotIn(forbidden, cancel)
        tick = body(LAB, 'void Tick(')
        self.assertIn('job.Admission.Expired(Clock::now())', tick)
        self.assertIn('continue; // Pending/rejected admission owns no native retirement intents.', tick)
        self.assertIn('CancelAdmission(aWorld, *ptr, "transport-disconnected")', body(LAB, 'void Disconnect('))
        job = body(LAB, 'struct Job')
        self.assertNotIn('BindingObservation', job)
        self.assertNotRegex(job, r'(?:Actor|TESNPC)\s*\*')

    def test_admission_logging_is_bounded_and_master_keeps_the_wait(self):
        capture = body(LAB, 'FinalRespawnAdmissionDecision Capture(')
        self.assertIn('aJob.AdmissionTraceCount++ < 32', capture)
        for phase in ('admission-wait-begin', 'admission-weapon-changed', 'admission-ready', 'admission-expired', 'admission-rejected'):
            self.assertIn('"' + phase + '"', capture)
        diagnostic = body(LAB, 'void LogAdmission(')
        for field in ('elapsedMs=', 'budgetMs=10000', 'bindingGuards=', 'finalRevision=', 'suppressedTransitions=', 'upstreamWeaponCause=unresolved'):
            self.assertIn(field, diagnostic)
        master = re.sub(r"#if \(!IS_MASTER\).*?#endif", "", LAB, flags=re.S)
        self.assertIn('aJob.Admission.Observe(', body(master, 'FinalRespawnAdmissionDecision Capture('))

    def test_native_state_diagnostics_share_gate_inputs_at_every_lab_boundary(self):
        observation = body(LAB, 'BindingObservation ObserveBinding(')
        self.assertIn('FinalRespawnActorStateSafe(o.Flags1, o.Flags2)', observation)
        rejection = body(LAB, 'void LogBindingRejection(')
        self.assertIn('ActorStateDiagnostic(o.Flags1, o.Flags2)', rejection)
        self.assertIn('stateDecode=unavailable', rejection)
        diagnostic = body(LAB, 'std::string ActorStateDiagnostic(')
        self.assertIn('DecodeFinalRespawnActorState(aFlags1, aFlags2)', diagnostic)
        for field in ('stateVeto=', 'lifeState=', 'lifeName=', 'knockState=', 'attackState=', 'flyState=',
                      'weaponState=', 'recoilState=', 'staggered=', 'sprinting=', 'swimming=',
                      'lifeAllowed=', 'knockIdle=', 'attackIdle=', 'flyIdle=', 'weaponSheathed=',
                      'recoilIdle=', 'notStaggered=', 'notSprinting=', 'notSwimming='):
            self.assertIn(field, diagnostic)
        safe = body(LAB, 'bool SafeActor(')
        self.assertIn('FinalRespawnActorStateSafe(flags1, flags2)', safe)
        self.assertIn('ActorStateDiagnostic(flags1, flags2)', safe)
        self.assertIn('detail=unsafe-actor-state', safe)
        advance = body(LAB, 'void Advance(')
        for boundary in ('old-binding', 'precommit', 'post-placement'):
            self.assertIn('aJob, "' + boundary + '"', advance)
        capture = body(LAB, 'FinalRespawnAdmissionDecision Capture(')
        self.assertIn('phase=actor-state-accepted', capture)
        self.assertIn('ActorStateDiagnostic(observed.Flags1, observed.Flags2)', capture)
        self.assertLess(advance.index('"gate-accepted"'), advance.index('"transaction-reserved"'))
        self.assertLess(advance.index('"transaction-reserved"'), advance.index('"candidate-create-enter"'))

    def test_current_binding_rejections_name_each_existing_guard(self):
        capture = body(LAB, 'FinalRespawnAdmissionDecision Capture(')
        for reason in ('unsupported-runtime', 'remote-serverid-collision', 'remote-component-missing', 'player-component-missing',
                       'formid-missing', 'player-id-mismatch', 'actor-lookup-failed', 'actor-not-remote-player', 'actor-dead',
                       'actor-disabled', 'combat', 'mounted', 'unsafe-actor-state', 'base-not-tesnpc', 'actor-not-temporary',
                       'base-not-temporary', 'actor-base-formid-collision', 'base-lookup-mismatch', 'entity-invalid',
                       'local-component-present', 'assignment-pending', 'waiting-for-3d', 'remote-serverid-mismatch',
                       'cachedref-mismatch', 'formid-actor-mismatch', 'native-binding-token-mismatch', 'provenance-mismatch',
                       'entity-alias-collision', 'base-alias-collision', 'cell-missing', 'root-not-ready',
                       'interpolation-missing', 'remote-animation-missing'):
            self.assertIn('"' + reason + '"', capture)
        self.assertNotIn('unsafe-or-inconsistent-current-binding', LAB)
        # Preserve the exact selected set; the Remote-only lookup is diagnostic, not selection.
        self.assertIn('aWorld.view<RemoteComponent, PlayerComponent>()', capture)
        self.assertIn('if (found == entt::null)', capture)
        self.assertNotIn('Materialize(', capture)
        self.assertIn('observed.Provenance && !observed.ProvenanceMatches', capture)
        self.assertIn('if (!Resolve(current))', capture)

    def test_rejected_binding_log_uses_the_observation_that_was_tested(self):
        capture = body(LAB, 'FinalRespawnAdmissionDecision Capture(')
        self.assertIn('auto observed = ObserveBinding(aWorld, found);', capture)
        self.assertIn('RejectAdmission(aJob, observed, reason)', capture)
        observation = body(LAB, 'BindingObservation ObserveBinding(')
        for source in ('remote->CachedRefId', 'form->Id', 'actor->actorState.flags1', 'actor->actorState.flags2',
                       'FinalRespawnActorStateSafe(o.Flags1, o.Flags2)', 'actor->GetParentCell()', 'actor->parentCell',
                       'RemoteActorProjection::ReadyFor3D(actor)', 'snapshot->RuntimeState'):
            self.assertIn(source, observation)
        log = body(LAB, 'void LogBindingRejection(')
        for field in ('serverId=', 'entityVersioned=', 'entityVersion=', 'FormId=', 'CachedRefId=', 'Actor=', 'Base=',
                      'ActorToken=', 'BaseToken=', 'baseLookupToken=', 'WaitingFor3D=', 'Local=', 'Assignment=',
                      'recoveryLocked=', 'recoveryState=', 'safeState=', 'actorStateFlags1=', 'actorStateFlags2='):
            self.assertIn(field, log)
        tick = body(LAB, 'void Tick(')
        self.assertIn('"transport-disconnected" : "recovery-locked"', tick)

    def test_t1_hot_paths_are_unreachable(self):
        receive = body(PROBE, "void CharacterService::OnAppearanceProbe(")
        self.assertIn("RemoteRespawnLab::ReceiveFinal", receive)
        self.assertNotIn("ApplyAppearanceSnapshots();", SERVICE)
        self.assertNotIn("Receive(", receive)
        for forbidden in ("SwitchRace(", "DoReset3D(", "Deserialize(", "SetSex(",
                          "SetRace(", "ApplyAppearanceSnapshots("):
            self.assertNotIn(forbidden, LAB)

    def test_t2_uniform_shared_materializer(self):
        advance = body(LAB, "void Advance(")
        self.assertEqual(advance.count("Materialize("), 1)
        for forbidden in ("raceChanged", "SexChange", "SameRace", "Classify", "TESNPC::Create(", "Actor::Create("):
            self.assertNotIn(forbidden, LAB)
        adapter = body(SERVICE, "Actor* STRE::RemoteRespawnLab::Materialize(")
        self.assertIn("MaterializePrivateRemoteActor(", adapter)
        self.assertIn("aFinal.AppearanceBuffer, aFinal.ChangeFlags, aFinal.FaceTints", adapter)
        self.assertNotIn("Descriptor", adapter)

    def test_t7_t8_t16_only_applied_official_build_publishes(self):
        notify = body(CREATION, "void CharacterCreationService::OnNotifyCharacterBuildState(")
        call = "trigger(RequestLocalAppearanceUpdateEvent{acMessage.Revision})"
        self.assertEqual(CREATION.count(call), 1)
        for guard in ("CharacterBuildNetworkState::Applied", "!m_waitingForServerFinalization",
                      "acMessage.Revision != m_serverBuildRevision", "acMessage.ServerId != m_serverCharacterId"):
            self.assertLess(notify.index(guard), notify.index(call))
        self.assertLess(notify.index("m_waitingForServerFinalization = false"), notify.index(call))
        update = body(CREATION, "void CharacterCreationService::OnUpdate(")
        self.assertNotIn("RequestLocalAppearanceUpdateEvent", update)
        receive = body(LAB, "void ReceiveFinal(")
        self.assertLess(receive.index("job.Done || job.Lifecycle.State() != MaterializationState::Idle"), receive.index("job.Final = aFinal"))
        self.assertIn("if (!job.Final)", receive)  # retries cannot extend timeout
        self.assertNotIn("Materialize(", receive)

    def test_t9_binding_identity_is_preserved(self):
        advance = body(LAB, "void Advance(")
        self.assertIn("get<FormIdComponent>(aJob.Entity).Id = aJob.Candidate.Forms.Actor", advance)
        self.assertIn("get<RemoteComponent>(aJob.Entity).CachedRefId = aJob.Candidate.Forms.Actor", advance)
        self.assertNotRegex(LAB, r"(?:create|destroy)\(aJob.Entity")
        self.assertNotRegex(LAB, r"(?:emplace|remove|replace)[^\n]*<(?:RemoteComponent|PlayerComponent|LocalComponent)>")
        self.assertIn("static_cast<uint32_t>(aJob.Entity), aJob.Server, ++s_generation", advance)
        self.assertIn("aWorld.valid(aJob.Entity)", LAB)
        self.assertIn("remote->Id == aJob.Server && player->Id == aJob.Player", LAB)

    def test_t10_t11_source_fencing_precedes_all_subscribers(self):
        visit = body(DISCOVERY, "void DiscoveryService::VisitForms(")
        self.assertLess(visit.index("RemoteRespawnLab::Discovery(formId, reinterpret_cast<uintptr_t>(apReference), true)"),
                        visit.index("enqueue(ActorAddedEvent(formId))"))
        self.assertIn("m_formTokens[formId] = reinterpret_cast<uintptr_t>(apReference)", visit)
        self.assertIn("token == m_formTokens.end() ? 0 : token->second, false", visit)
        self.assertIn("previous->second != token", visit)
        discovery = body(LAB, "bool Discovery(")
        self.assertIn("job.Lifecycle.Observe(job.Lifecycle.Key(), aFormId, aAdded)", discovery)
        self.assertIn("job.Old.ActorToken == aToken", discovery)
        self.assertIn("job.Candidate.ActorToken == aToken", discovery)
        self.assertNotIn("GetById", discovery)  # never resolve stale tokens at delivery
        self.assertNotIn("enqueue", discovery)  # immutable key consumed at source
        self.assertIn("DiscoveryRoute::OldLostBeforeCommit", discovery)
        self.assertIn("DiscoveryRoute::LiveBindingLost", discovery)

    def test_reservation_before_native_exposure(self):
        advance = body(LAB, "void Advance(")
        self.assertLess(advance.index(".Reserve("), advance.index("s_creating = &aJob"))
        create = body(ACTOR, "GamePtr<Actor> Actor::Create(")
        self.assertLess(create.index("RemoteRespawnLab::BeforeSpawn"), create.index("ModManager::Get()->Spawn"))
        self.assertGreater(create.index("RemoteRespawnLab::AfterSpawn"), create.index("ModManager::Get()->Spawn"))
        before = body(LAB, "void BeforeSpawn(")
        self.assertIn("ExpectedBase != reinterpret_cast<uintptr_t>(aBase)", before)
        self.assertIn("s_creating->Candidate.ActorToken", before)
        self.assertIn("job.Candidate = {{0, aBase->formID}", before)  # Spawn owns final Actor ID assignment.
        self.assertNotIn("Delete", body(LAB, "void AfterSpawn("))

    def test_t12_failure_and_recovery_keep_old(self):
        abort = body(LAB, "void Abort(")
        self.assertIn("Lifecycle.Abort", abort)
        self.assertNotIn("Retire(aJob, aJob.Old", abort)
        self.assertLess(abort.index("if (Locked() || !World::Get().GetTransport().IsConnected())"), abort.index("DispatchRetirement(aJob)"))
        self.assertIn("CandidateCleanupPending = true", abort)
        self.assertIn("aJob.CandidateCleanupPending", body(LAB, "void DispatchRetirement("))
        tick = body(LAB, "void Tick(")
        self.assertIn("job.Lifecycle.Abort", tick)
        self.assertNotIn("Lifecycle.Invalidate", tick)
        self.assertIn("cleanup-deferred", LAB)
        self.assertIn("std::chrono::seconds(10)", LAB)
        self.assertIn("WaitingForAssignmentComponent, WaitingFor3D", LAB)
        self.assertIn("FinalRespawnActorStateSafe", LAB)
        self.assertNotIn("ExtraDataType::Interaction", LAB)

    def test_candidate_facegen_stays_private_until_commit(self):
        advance = body(LAB, "void Advance(")
        self.assertIn("FaceGenSystem::Update(aWorld, candidate, aJob.Tints)", advance)
        self.assertLess(advance.index("Lifecycle.Commit("), advance.index("get_or_emplace<FaceGenComponent>"))
        self.assertLess(advance.index("get_or_emplace<FaceGenComponent>"), advance.index("DispatchRetirement(aJob)"))
        self.assertIn("!Geometry(aJob, candidate)", advance)
        self.assertIn("headparts", body(LAB, "bool Geometry("))

    def test_inventory_uses_existing_restore_and_live_acceptance(self):
        advance = body(LAB, "void Advance(")
        projection = body(read("Code/client/Services/Generic/RemoteActorProjection.cpp"), "void Complete3D(")
        for guard in ("ScopedInventoryOverride", "ScopedEquipOverride", "ScopedUnequipOverride"):
            self.assertLess(projection.index(guard), projection.index("SetActorInventory("))
        self.assertIn("Complete3D(candidate, aJob.Build.Build.CanonicalInventory", advance)
        self.assertLess(advance.index("InventoryService::MatchesCanonicalInventory"), advance.index("Lifecycle.Commit("))
        verify = body(read("Code/client/Services/Generic/InventoryService.cpp"), "bool InventoryService::MatchesCanonicalInventory(")
        for evidence in ("GetItemCountInInventory", "item.Count", "item.Worn", "CurrentMagicEquipment"):
            self.assertIn(evidence, verify)
        self.assertNotIn("ComputeCharacterBuildInventoryHash", verify)

    def test_t13_no_network_or_manual_native_lifetime(self):
        for forbidden in (".Send(", "CharacterSpawnRequest", "RequestServerAssignment", "TakeOwnership",
                          "RequestRespawn", "RequestOwnership", "Memory::Free", "DecRef", "IncRef", "POINTER_SKYRIMSE"):
            self.assertNotIn(forbidden, LAB)
        retire = body(LAB, "void Retire(")
        self.assertEqual(retire.count("CharacterService::DeleteTempActor("), 1)
        self.assertLess(retire.index("aPair.DeleteIssued = true"), retire.index("CharacterService::DeleteTempActor("))
        self.assertIn("Resolve(aPair)", retire)
        self.assertIn("aPair.Forms.Actor != aJob.Old.Forms.Actor || aPair.ActorToken != aJob.Old.ActorToken", retire)

    def test_default_activation_without_debug_gate_retains_solo_and_recovery_guards(self):
        for source in (LAB, SERVICE, CREATION, PROBE, read("Code/client/Services/RemoteRespawnLab.h")):
            self.assertNotIn("RemoteRespawnLab::Enabled", source)
            self.assertNotIn("RemoteRespawnLab::SetEnabled", source)
        for forbidden in ("s_enabled", "bool Enabled(", "void SetEnabled(", "feature-disabled", "phase=feature-gate"):
            self.assertNotIn(forbidden, LAB)
        policy = body(read("Code/common/CharacterCreation/FinalRespawn.h"), "inline bool FinalRespawnEligible(")
        self.assertNotIn("aEnabled", policy)
        self.assertNotIn("aMaster", policy)
        self.assertIn("aConnected && aOfficialCreation && aApplied && aFinalRevision && aFinalRevision == aAppliedRevision", policy)
        constructor = body(SERVICE, "CharacterService::CharacterService(")
        self.assertIn("phase=final-rematerialization-enabled source=official-character-creation-default", constructor)
        publish = body(PROBE, "void CharacterService::OnLocalAppearanceUpdate(")
        self.assertLess(publish.index("!m_transport.IsConnected()"), publish.index("m_appearanceFinal.Queue("))
        flush = body(PROBE, "void CharacterService::FlushAppearanceFinal(")
        self.assertIn("if (AppearanceRuntimeLocked())", flush)
        self.assertIn("m_transport.IsConnected(), id,", flush)
        receive = body(LAB, "void ReceiveFinal(")
        for guard in ("!aWorld.GetTransport().IsConnected()", "!aFinal.FinalBuildRevision", "Locked()",
                      "!aFinal.IsValid()", "s_jobs.size() >= 10"):
            self.assertLess(receive.index(guard), receive.index("s_jobs[aFinal.ActorId]"))
        self.assertIn("FinalRespawnEligible(true, true, job.Build.State == CharacterBuildNetworkState::Applied", LAB)

    def test_functional_path_survives_removal_of_all_non_master_blocks(self):
        # Project IS_MASTER is generated from the branch; no branch checkout is
        # needed to verify these source regions. Only the retirement diagnostic
        # may disappear from the transaction in MASTER.
        master = re.sub(r"#if \(!IS_MASTER\).*?#endif", "", LAB, flags=re.S)
        for signature, operation in (
            ("FinalRespawnAdmissionDecision Capture(", '"unsupported-runtime"'),
            ("void Advance(", "Materialize("),
            ("void ReceiveBuild(", "job->Build = aBuild"),
            ("void ReceiveFinal(", "job.Final = aFinal"),
            ("void Tick(", "Advance(aWorld, job)"),
            ("void BeforeSpawn(", "job.Candidate ="),
            ("void CandidateBase(", "s_creating->ExpectedBase ="),
            ("void AfterSpawn(", "Lifecycle.RecordCandidate("),
            ("void DeleteRequested(", "pair->DeleteIssued = true"),
            ("bool Discovery(", "Lifecycle.Observe("),
            ("bool Tracks(", "return true"),
            ("void Disconnect(", "Lifecycle.Invalidate(")):
            self.assertIn(operation, body(master, signature))
        self.assertNotIn("phase=retirement-window", master)
        for source, signature, operation in (
            (SERVICE, "Actor* STRE::RemoteRespawnLab::Materialize(", "MaterializePrivateRemoteActor("),
            (CREATION, "void CharacterCreationService::OnNotifyCharacterBuildState(", "trigger(RequestLocalAppearanceUpdateEvent"),
            (PROBE, "void CharacterService::OnLocalAppearanceUpdate(", "m_appearanceFinal.Queue("),
            (PROBE, "void CharacterService::OnAppearanceProbe(", "RemoteRespawnLab::ReceiveFinal"),
            (ACTOR, "GamePtr<Actor> Actor::Create(", "RemoteRespawnLab::BeforeSpawn"),
            (DISCOVERY, "void DiscoveryService::VisitForms(", "RemoteRespawnLab::Discovery(")):
            production = re.sub(r"#if \(!IS_MASTER\).*?#endif", "", source, flags=re.S)
            self.assertIn(operation, body(production, signature))

    def test_functional_toggle_removed_and_passive_controls_remain_non_master(self):
        debug = read("Code/client/Services/Debug/DebugService.cpp")
        self.assertNotIn("Final Character Creation local respawn LAB", debug)
        self.assertNotIn("RemoteRespawnLab::", debug)
        update = body(debug, "void DebugService::OnUpdate(")
        ctrl_ignored = body(update, "if (!(GetAsyncKeyState(VK_CONTROL) & 0x8000))")
        self.assertIn("RequestObserveCurrentPrivateRemote();", ctrl_ignored)
        self.assertIn("SetNativeLifetimeProbeEnabled(!IsNativeLifetimeProbeEnabled());", ctrl_ignored)
        self.assertIn("!s_nativeLifetimeKeyDown", update)
        master = re.sub(r"#if \(!IS_MASTER\).*?#endif", "", debug, flags=re.S)
        self.assertNotIn("VK_F11", master)
        self.assertNotIn("SetNativeLifetimeProbeEnabled", master)

    def test_server_requires_sealed_final(self):
        server = body(read("Code/server/Services/CharacterService.cpp"), "void CharacterService::OnCharacterAppearanceUpdate(")
        self.assertLess(server.index("MatchesAppliedCharacterBuild"), server.index("ApplyCharacterAppearanceUpdate"))
        self.assertIn("build->FinalAppearance = acMessage.Packet", server)
        policy = read("Code/server/Services/CharacterAppearanceUpdate.h")
        self.assertIn("*aCommitted == aFinal", policy)
        self.assertIn("aFinal.FinalBuildRevision == aRevision", policy)


if __name__ == "__main__":
    unittest.main()
