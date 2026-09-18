"""Passive probe source fences; no Skyrim runtime or native lifetime claims.

Run with Python's standard library. TPTests covers the observation window.
Verified addresses/ABIs are documented in the native lifetime observation audit;
these fences prevent new hook targets or mutators silently entering the probe.
"""
from pathlib import Path
import re
import unittest

ROOT = Path(__file__).resolve().parents[2]


def read(path):
    return (ROOT / path).read_text()


def body(source, marker):
    opening = source.index("{", source.index(marker))
    depth = 1
    for end in range(opening + 1, len(source)):
        depth += (source[end] == "{") - (source[end] == "}")
        if not depth:
            return source[opening + 1:end]
    raise AssertionError("Unclosed body: " + marker)


PROBE = read("Code/client/Games/Skyrim/NativeLifetimeProbe.cpp")
HEADER = read("Code/client/Games/Skyrim/NativeLifetimeProbe.h")
CODE = re.sub(r'//[^\n]*|/\*.*?\*/|"(?:\\.|[^"\\])*"', "", PROBE, flags=re.S)
SERVICE = read("Code/client/Services/Generic/CharacterService.cpp")
ACTOR = read("Code/client/Games/Skyrim/Actor.cpp")
REFR = read("Code/client/Games/Skyrim/TESObjectREFR.cpp")
DISCOVERY = read("Code/client/Services/Generic/DiscoveryService.cpp")


class NativeLifetimeProbeContract(unittest.TestCase):
    def test_current_selection_requires_one_existing_private_bound_remote(self):
        selection = body(PROBE, "void ObserveCurrentPrivateRemote(")
        for guard in ("aWorld.view<RemoteComponent, PlayerComponent>()", "selected != entt::null",
                      "selected == entt::null", "RemotePlayerAppearanceBaseComponent",
                      "any_of<LocalComponent, WaitingForAssignmentComponent, WaitingFor3D>",
                      "remote.CachedRefId != form->Id", "ClassifyNativeLifetimeBinding(",
                      "extension->IsRemotePlayer()", "actor->formID != form->Id", "base->formID != baseId",
                      "actor->baseForm != base", "native-binding-changed-during-selection"):
            self.assertIn(guard, selection)
        self.assertLess(selection.index('reject("no-current-remote-player")'), selection.index("s_enabled.store(true)"))
        self.assertLess(selection.index('reject("native-binding-not-ready")'), selection.index("InstallVerifiedHooks()"))
        self.assertIn('reject("disable-existing-probe-first")', selection)
        self.assertIn('reject("unsupported-runtime")', selection)

    def test_current_fallback_is_observation_only_and_rejects_known_aliases(self):
        selection = body(PROBE, "void ObserveCurrentPrivateRemote(")
        classify = selection[selection.index("const auto evidence ="):selection.index("Record observed;")]
        self.assertIn("identity != nullptr", classify)
        self.assertIn("identity ? identity->ActorFormId : 0", classify)
        self.assertIn("identity ? identity->BaseFormId : 0", classify)
        self.assertNotRegex(selection, r"if\s*\([^)]*!identity")
        for guard in ('reject("private-provenance-mismatch")', "aWorld.view<FormIdComponent>()",
                      "otherId == form->Id", "otherActor->baseForm == base",
                      "aWorld.view<RemoteComponent>()", "otherRemote.Id == remote.Id",
                      "otherRemote.CachedRefId == form->Id", 'reject("shared-current-base")',
                      'reject("ambiguous-current-binding")'):
            self.assertIn(guard, classify)
        self.assertLess(selection.index('reject("ambiguous-current-binding")'), selection.index("InstallVerifiedHooks()"))
        for evidence in ("phase=current-private-remote-evidence", "selectionEvidence={}", "provenancePresent={}",
                         '"durable-provenance"', '"validated-current-binding-fallback"'):
            self.assertIn(evidence, selection)
        # No production appearance, materialization or server path may use this relaxation.
        callers = [p for tree in ("Code/client", "Code/server") for p in (ROOT / tree).rglob("*.cpp")
                   if "ClassifyNativeLifetimeBinding(" in p.read_text()]
        self.assertEqual([p.name for p in callers], ["NativeLifetimeProbe.cpp"])
        master = re.sub(r"#if \(!IS_MASTER\).*?#endif", "", PROBE, flags=re.S)
        self.assertNotIn("ClassifyNativeLifetimeBinding(", master)

    def test_current_request_is_one_shot_on_game_update_and_non_master(self):
        request = body(PROBE, "void RequestObserveCurrentPrivateRemote(")
        self.assertEqual(request.strip(), "s_observeCurrentRequested.store(true);")
        tick = body(PROBE, "void TickNativeLifetimeProbe(")
        self.assertIn("s_observeCurrentRequested.exchange(false)", tick)
        self.assertLess(tick.index("ObserveCurrentPrivateRemote(aWorld, aServiceTick)"), tick.index("if (!s_enabled.load())"))
        master = re.sub(r"#if \(!IS_MASTER\).*?#endif", "", PROBE, flags=re.S)
        self.assertNotIn("ObserveCurrentPrivateRemote", master)
        self.assertNotIn("s_observeCurrentRequested", master)
        debug = read("Code/client/Services/Debug/DebugService.cpp")
        self.assertEqual(debug.count("RequestObserveCurrentPrivateRemote();"), 2)
        self.assertIn("GetAsyncKeyState(VK_SHIFT) & 0x8000", debug)
        self.assertIn('"Passive native lifetime probe (observe current private remote)", "Shift+F11"', debug)
        master_debug = re.sub(r"#if \(!IS_MASTER\).*?#endif", "", debug, flags=re.S)
        self.assertNotIn("RequestObserveCurrentPrivateRemote", master_debug)

    def test_current_mode_reuses_the_passive_record_without_creation_history(self):
        selection = body(PROBE, "void ObserveCurrentPrivateRemote(")
        for capture in ("observed.Entity = static_cast<uint32_t>(selected)", "observed.Server = remote.Id",
                        "observed.ActorForm = form->Id", "observed.BaseForm = baseId",
                        "observed.ActorToken = reinterpret_cast<uintptr_t>(actor)",
                        "observed.BaseToken = reinterpret_cast<uintptr_t>(base)",
                        "observed.Session = ++s_nextSession", "observed.Start = Clock::now()", "s_current = observed"):
            self.assertIn(capture, selection)
        self.assertIn("Record observed;", selection)
        self.assertNotIn("Window.", selection)
        self.assertNotIn("NativeLifetimeStage(", selection)
        self.assertNotIn("natural-creation-selected", selection)
        self.assertNotIn("DiscoveryKnown =", selection)
        self.assertNotIn("GetHandle(", re.sub(r"//[^\n]*", "", selection))
        after_lookup = selection[selection.index("TESObjectREFR::GetByHandle(handle)"):]
        self.assertNotRegex(after_lookup, r"\b(?:actor|base)\s*->")

    def test_hotkey_and_checkbox_share_the_same_non_master_transition(self):
        debug = read("Code/client/Services/Debug/DebugService.cpp")
        update = body(debug, "void DebugService::OnUpdate(")
        draw = body(debug, "void DebugService::OnDraw(")
        hotkey = update[update.index("#if (!IS_MASTER)"):update.index("if (GetAsyncKeyState(VK_F6))")]
        self.assertIn("(GetAsyncKeyState(VK_F11) & 0x8000) != 0", hotkey)
        self.assertIn("if (nativeLifetimeKeyDown && !s_nativeLifetimeKeyDown)", hotkey)
        self.assertIn("SetNativeLifetimeProbeEnabled(!IsNativeLifetimeProbeEnabled());", hotkey)
        self.assertIn("s_nativeLifetimeKeyDown = nativeLifetimeKeyDown;", hotkey)
        self.assertNotIn("#endif", hotkey)
        self.assertLess(update.index("if (!BSGraphics::GetMainWindow()->IsForeground())"), update.index("VK_F11"))
        self.assertNotIn("m_showDebugStuff", hotkey)
        checkbox = draw[draw.index("bool nativeLifetimeEnabled ="):draw.index('ImGui::MenuItem("Network"')]
        self.assertIn("bool nativeLifetimeEnabled = IsNativeLifetimeProbeEnabled();", checkbox)
        self.assertIn("SetNativeLifetimeProbeEnabled(nativeLifetimeEnabled);", checkbox)
        for control in (hotkey, checkbox):
            self.assertEqual(control.count("SetNativeLifetimeProbeEnabled("), 1)
            self.assertNotRegex(control, r"Send\(|Connect\(|Close\(|spdlog::|InstallVerifiedHooks\(|s_enabled")
        # Removing non-master blocks removes both controls; no MASTER shortcut.
        master = re.sub(r"#if \(!IS_MASTER\).*?#endif", "", debug, flags=re.S)
        self.assertNotIn("VK_F11", master)
        self.assertNotIn("SetNativeLifetimeProbeEnabled(", master)

    def test_default_disabled_and_explicit_debug_activation(self):
        self.assertIn("std::atomic_bool s_enabled{false}", PROBE)
        enable = body(PROBE, "void SetNativeLifetimeProbeEnabled(")
        self.assertIn('GetLoadedVersionString() != "1.6.1170.0"', enable)
        self.assertRegex(enable, r"if \(aEnabled\)\s+InstallVerifiedHooks\(\);")
        debug = read("Code/client/Services/Debug/DebugService.cpp")
        toggle = debug.index("SetNativeLifetimeProbeEnabled(")
        self.assertGreaterEqual(debug.rfind("#if (!IS_MASTER)", 0, toggle), 0)
        self.assertIn("Passive native lifetime probe (next private remote)", debug)
        # No startup, network or service path may silently arm the recorder.
        callers = [p for p in (ROOT / "Code/client").rglob("*.cpp")
                   if "SetNativeLifetimeProbeEnabled(" in p.read_text()]
        self.assertEqual(sorted(p.name for p in callers), ["DebugService.cpp", "NativeLifetimeProbe.cpp"])

    def test_no_native_mutation_network_or_appearance_apply(self):
        for forbidden in (r"\b(?:Create|New|Spawn|Delete|Destroy|Free|MoveTo|Reset3D|SwitchRace|SetBaseForm|SetActorInventory|EquipItem|UnequipItem|TakeOwnership|Send|SendMessage|SetPosition|Enable|Disable)\s*\(",
                          r"\b(?:GamePtr|NiPointer|BSTSmartPointer)\s*<", r"\b(?:IncRef|DecRefHandle|GetHandle)\s*\(",
                          r"\b(?:malloc|free|new|delete)\b", r"\b(?:emplace|emplace_or_replace|remove|destroy)\s*[<(]",
                          r"(?:TransportService|AppearanceApply|Messages/|RemoteMaterializationLifecycle)"):
            self.assertNotRegex(CODE, forbidden)
        record = body(PROBE, "struct Record")
        self.assertNotIn("*", record)
        self.assertNotIn("GamePtr", HEADER)

    def test_native_hooks_are_only_the_two_verified_scalar_destructors(self):
        self.assertIn("using DeletingDestructor = void*(void*, uint32_t);", PROBE)
        self.assertEqual(re.findall(r"Verified\((\d+), (0x[0-9a-f]+)", PROBE),
                         [("40288", "0x728090"), ("24888", "0x3c4be0")])
        self.assertEqual(PROBE.count("TP_HOOK_IMMEDIATE("), 2)
        verify = body(PROBE, "bool Verified(")
        for check in ("FindAddressById", "GetModuleHandleW", "VirtualQuery", "MEM_COMMIT", "PAGE_GUARD", "PAGE_EXECUTE", "ReadProcessMemory", "actual == aBytes"):
            self.assertIn(check, verify)
        self.assertNotIn("TESFormDeleteEvent", CODE)
        self.assertNotIn("HookCharacterDestructor", CODE)
        self.assertNotIn("37175", CODE)

    def test_destructors_forward_exactly_once_preserving_arguments_and_result(self):
        for name, original, base in (("Character", "s_realCharacterDeletingDestructor", "false"),
                                      ("Npc", "s_realNpcDeletingDestructor", "true")):
            hook = body(PROBE, "void* Hook" + name + "DeletingDestructor(")
            self.assertEqual(hook.count(original + "("), 1)
            self.assertIn(f"void* result = {original}(apObject, aFlags);", hook)
            self.assertIn(f"DestructorEnter(apObject, {base}, aFlags)", hook)
            self.assertIn(f"DestructorReturn(session, {base}, aFlags)", hook)
            self.assertIn("return result;", hook)
            self.assertNotIn("apObject", hook[hook.index("DestructorReturn"):])
        entry = body(PROBE, "uint64_t DestructorEnter(")
        self.assertIn("static_cast<const TESForm*>(apObject)->formID", entry)
        returned = body(PROBE, "void DestructorReturn(")
        self.assertNotIn("apObject", returned)
        self.assertNotIn("GetById", returned)

    def test_polling_compares_tokens_without_native_pointer_dereference(self):
        poll = body(PROBE, "void TickNativeLifetimeProbe(")
        self.assertIn("TESForm::GetById(sample.ActorForm)", poll)
        self.assertIn("TESForm::GetById(sample.BaseForm)", poll)
        self.assertIn("TESObjectREFR::GetByHandle(sample.Handle)", poll)
        self.assertIn("aWorld.valid(entity)", poll)
        self.assertIn("remote->CachedRefId == sample.ActorForm", poll)
        self.assertNotRegex(poll, r"\b(?:actor|base|p)\s*->")
        self.assertNotRegex(CODE, r"(?:reinterpret_cast|static_cast)<[^>]*\*>\([^)]*(?:Token|token)\)")
        self.assertNotIn("GetNiNode(", PROBE)
        self.assertIn("identity-reused-stop-correlation", poll)

    def test_only_one_natural_pair_and_no_timeout_completion_claim(self):
        creation = body(PROBE, "NativeLifetimeCreationScope::NativeLifetimeCreationScope(")
        self.assertRegex(creation, r"if \(s_current.Session\)\s+return;")
        self.assertIn("s_current.Session = ++s_nextSession", creation)
        self.assertIn("PollDue(elapsed)", PROBE)
        self.assertIn("Window.Expired(elapsed)", PROBE)
        self.assertIn("NOT-OBSERVED-WITHIN-WINDOW-is-not-leak-proof", PROBE)
        self.assertNotIn("s_current =", body(PROBE, "void NativeLifetimeDisconnect("))
        self.assertEqual(SERVICE.count("NativeLifetimeCreationScope lifetime("), 2)
        for marker in ("void CharacterService::OnCharacterSpawn(", "Actor* CharacterService::CreateCharacterForEntity("):
            caller = body(SERVICE, marker)
            gate = caller.index("if (acMessage.BaseId == GameId{} && acMessage.IsPlayer)")
            self.assertLess(gate, caller.index("NativeLifetimeCreationScope lifetime("))
            self.assertLess(caller.index("NativeLifetimeCreationScope lifetime("), caller.index("MaterializePrivateRemoteActor("))

    def test_delete_and_spawn_calls_are_not_duplicated(self):
        deletion = body(REFR, "void TESObjectREFR::Delete(")
        self.assertEqual(deletion.count("s_pDelete(this);"), 1)
        self.assertLess(deletion.index("NativeLifetimeDeleteEnter(this)"), deletion.index("s_pDelete(this);"))
        self.assertLess(deletion.index("s_pDelete(this);"), deletion.index("NativeLifetimeDeleteReturn(lifetimeSession)"))
        creation = body(ACTOR, "GamePtr<Actor> Actor::Create(")
        self.assertEqual(creation.count("ModManager::Get()->Spawn("), 1)
        self.assertLess(creation.index('"pre-spawn"'), creation.index("ModManager::Get()->Spawn("))
        self.assertLess(creation.index("ModManager::Get()->Spawn("), creation.index('"post-spawn"'))
        self.assertIn("pActor, false, spawnHandle", creation)

    def test_discovery_dispatch_remains_unconditional(self):
        # Passive observations remain unconditional. Slice 3 alone routes its own
        # staged/retired actors before legacy subscribers; unrelated events pass.
        self.assertIn("NativeLifetimeDiscovery(formId, true);", DISCOVERY)
        self.assertIn("NativeLifetimeDiscovery(formId, false);", DISCOVERY)
        self.assertIn("if (!STRE::RemoteRespawnLab::Discovery(", DISCOVERY)
        self.assertNotIn("if (!NativeLifetime", DISCOVERY)
        self.assertNotIn("return", body(PROBE, "void NativeLifetimeDiscovery(").split('Log(aAdded ?')[1])
        self.assertIn("s_current.Root = aAdded ? 1 : -1;", PROBE)

    def test_recording_does_not_touch_the_debug_actor_collector(self):
        self.assertNotIn("m_actors", PROBE)
        debug = read("Code/client/Services/Debug/DebugService.cpp")
        self.assertEqual(debug.count("m_actors.emplace_back("), 1)
        observe = body(PROBE, "void Observe(")
        self.assertIn("const auto error = GetLastError();", observe)
        self.assertIn("SetLastError(error);", observe)


if __name__ == "__main__":
    unittest.main()
