"""Slice 3 source contracts. Complements TPTests; no Skyrim/native validation."""
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
        capture = body(LAB, 'bool Capture(')
        self.assertIn('phase=actor-state-accepted', capture)
        self.assertIn('ActorStateDiagnostic(observed.Flags1, observed.Flags2)', capture)
        self.assertLess(advance.index('"gate-accepted"'), advance.index('"transaction-reserved"'))
        self.assertLess(advance.index('"transaction-reserved"'), advance.index('"candidate-create-enter"'))

    def test_current_binding_rejections_name_each_existing_guard(self):
        capture = body(LAB, 'bool Capture(')
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
        self.assertIn('if (!Resolve(aJob.Old))', capture)

    def test_rejected_binding_log_uses_the_observation_that_was_tested(self):
        capture = body(LAB, 'bool Capture(')
        self.assertIn('auto observed = ObserveBinding(aWorld, found);', capture)
        self.assertIn('LogBindingRejection(aJob, observed, "gate-rejected", reason)', capture)
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

    def test_t14_t15_off_master_and_solo_gates(self):
        self.assertIn("std::atomic_bool s_enabled{false}", LAB)
        enabled = body(LAB, "bool Enabled(")
        self.assertIn("#else\n    return false;", enabled)
        for fn in ("ReceiveFinal", "ReceiveBuild", "Tick", "BeforeSpawn", "AfterSpawn", "Disconnect"):
            signature = ("void " + fn + "(")
            self.assertIn("#if (!IS_MASTER)", body(LAB, signature))
        publish = body(PROBE, "void CharacterService::OnLocalAppearanceUpdate(")
        self.assertLess(publish.index("!m_transport.IsConnected()"), publish.index("m_appearanceFinal.Queue("))
        flush = body(PROBE, "void CharacterService::FlushAppearanceFinal(")
        self.assertIn("!STRE::RemoteRespawnLab::Enabled()", flush)

    def test_hotkey_and_menu_share_transition(self):
        debug = read("Code/client/Services/Debug/DebugService.cpp")
        self.assertIn("Final Character Creation local respawn LAB", debug)
        self.assertIn('"Ctrl+F11"', debug)
        self.assertEqual(debug.count("RemoteRespawnLab::SetEnabled("), 2)
        update = body(debug, "void DebugService::OnUpdate(")
        self.assertIn("VK_CONTROL", update)
        self.assertIn("!s_nativeLifetimeKeyDown", update)
        self.assertIn("#if (!IS_MASTER)", update)

    def test_server_requires_sealed_final(self):
        server = body(read("Code/server/Services/CharacterService.cpp"), "void CharacterService::OnCharacterAppearanceUpdate(")
        self.assertLess(server.index("MatchesAppliedCharacterBuild"), server.index("ApplyCharacterAppearanceUpdate"))
        self.assertIn("build->FinalAppearance = acMessage.Packet", server)
        policy = read("Code/server/Services/CharacterAppearanceUpdate.h")
        self.assertIn("*aCommitted == aFinal", policy)
        self.assertIn("aFinal.FinalBuildRevision == aRevision", policy)


if __name__ == "__main__":
    unittest.main()
