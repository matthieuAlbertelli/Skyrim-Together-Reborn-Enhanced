"""Bootstrap ordering checks: no game execution or installed-file mutation."""
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]


def read(path):
    return (ROOT / path).read_text(encoding="utf-8-sig")


class RuntimePortBootstrap(unittest.TestCase):
    def test_preflight_precedes_mapping_and_all_game_hooks(self):
        source = read("Code/immersive_launcher/Launcher.cpp")
        startup = source[source.index("int StartUp("):source.index("bool LoadProgram(")]
        self.assertLess(startup.index("LoadProgram(*LC)"), startup.index("InstallStartHook()"))
        self.assertLess(startup.index("InstallStartHook()"), startup.index("RunTiltedInit("))
        self.assertNotIn("InstallPathRouting(", startup)
        self.assertNotIn("steam::Load(", startup)
        load = source[source.index("bool LoadProgram("):source.index("void InitClient()")]
        ordered = ["TargetVersionString", "InspectExecutable(", "Supports(", "RunTiltedPreflight(",
                   "InstallPathRouting(", "steam::Load(", "LC.SetLoaded()", "loader.Load("]
        self.assertEqual([load.index(x) for x in ordered], sorted(load.index(x) for x in ordered))

    def test_unverified_campaign_cannot_start_or_install_initializers(self):
        source = read("Code/client/main.cpp")
        preflight = source[source.index("bool RunTiltedPreflight("):source.index("void RunTiltedInit(")]
        self.assertLess(preflight.index("VersionDb::Get().Load"), preflight.index("ValidateScriptExtender"))
        self.assertIn("!STRE::Runtime::CampaignAbiVerified", preflight)
        self.assertIn("return false;", preflight[preflight.index("CampaignAbiUnavailable"):])
        init = source[source.index("void RunTiltedInit("):source.index("void RunTiltedApp()")]
        self.assertLess(init.index("!STRE::Runtime::CampaignAbiVerified"), init.index("std::make_unique"))
        self.assertLess(init.index("exit(4)"), init.index("InstallHooks2"))
        self.assertLess(init.index("TP_HOOK_COMMIT"), init.index("LoadScriptExtender("))

    def test_skse_installs_early_without_claiming_plugin_initialization(self):
        app = read("Code/client/TiltedOnlineApp.cpp")
        skse = read("Code/client/ScriptExtender.cpp")
        self.assertNotIn("LoadScriptExender", app)
        self.assertNotIn("LoadScriptExtender", app)
        self.assertIn("aGamePath / STRE::Runtime::RequiredSkseDll", skse)
        self.assertNotIn("directory_iterator", skse)
        self.assertIn("plugin initialization continues during game startup", skse)
        self.assertNotIn("SKSE is active", skse)
        transport = read("Code/client/Services/Generic/TransportService.cpp")
        self.assertIn("request.UserNativePlugins = CollectNativePlugins()", transport)

    def test_client_server_keep_the_same_exact_source_build_identity(self):
        self.assertIn('${GIT_COMMIT_LONG}', read("Code/common/BuildInfo.h.in"))
        self.assertIn('set_policy("build.always_update_configfiles", true)', read("Code/common/xmake.lua"))
        self.assertIn("request.Version = BUILD_COMMIT", read("Code/client/Services/Generic/TransportService.cpp"))
        self.assertIn("acRequest->Version != BUILD_COMMIT", read("Code/server/GameServer.cpp"))

    def test_unproven_native_paths_are_guarded_before_layout_access(self):
        reset = read("Code/client/Games/Skyrim/MainMenuRuntime.cpp")
        reset = reset[reset.index("bool RequestSkyrimMainMenu()"):]
        self.assertLess(reset.index("SupportsQualifiedNativeAccess"), reset.index("POINTER_SKYRIMSE"))
        self.assertIn("NativeContract::ResetMain", reset)
        camera = read("Code/client/Games/Skyrim/Interface/Menus/RaceSexMenu.h")
        camera = camera[camera.index("GetRaceSexMenuCamera("):]
        self.assertLess(camera.index('!= "1.6.1170.0"'), camera.index("Cast<RaceSexMenu>"))
        grab = read("Code/client/Games/Skyrim/PlayerCharacter.cpp")
        grab = grab[grab.index("bool PlayerCharacter::TryEndGrabObject()"):]
        self.assertLess(grab.index("SupportsQualifiedNativeAccess"), grab.index("VersionDbPtr"))
        self.assertIn("NativeContract::GrabRelease", grab)
        appearance = read("Code/client/Services/Generic/RemoteRespawnLab.cpp")
        appearance = appearance[appearance.index("FinalRespawnAdmissionDecision Capture("):]
        self.assertLess(appearance.index("SupportsQualifiedNativeAccess"), appearance.index("auto observed = ObserveBinding"))
        self.assertIn("NativeContract::FinalAppearance", appearance)
        seating = read("Code/client/Services/Generic/CreationSeating.cpp")
        self.assertIn("NativeContract::LocalSeating", seating)

    def test_save_selection_reads_are_protected_and_copied_before_forwarding(self):
        source = read("Code/client/Games/Skyrim/CampaignSaveLoadGate.cpp")
        probe = source[source.index("MostRecentPointerProbe ProbeMostRecentTargetPointer("):
                       source.index("NativeTargetObservation ObserveMostRecentTarget(")]
        self.assertIn("__try", probe)
        self.assertIn("__except (EXCEPTION_EXECUTE_HANDLER)", probe)
        self.assertIn("STRE::Runtime::MostRecentSaveName(", probe)
        self.assertNotIn("ppEntries[0]", probe)
        # Admission must still use a bounded copied observation, not persist
        # the native BSString pointer across the engine call.
        self.assertIn("strnlen_s(", source)
        self.assertIn("kMaximumObservedSaveName", source)
        self.assertIn("observation.Target.assign(apTarget, probe.Length)", source)

    def test_missing_rtti_remain_lazy_and_have_no_new_native_consumers(self):
        # A new consumer must reopen B04; inherited base declarations alone do
        # not instantiate Cast<NiObjectNET> or resolve its RttiLocator.
        source = read("Code/client/Games/Skyrim/RTTI.cpp")
        constructor = source[source.index("RttiLocator<T>::RttiLocator"):source.index("template <class T> const void*")]
        self.assertNotIn(".Get(", constructor)
        self.assertNotIn("GetPtr(", constructor)
        self.assertNotIn("FindAddress", constructor)
        for name, address_id in (("NiObjectNET", 392519), ("CreationClubMenu", 394235), ("NiParticleSystem", 398437)):
            self.assertIn(f"registerRtti_{name}({address_id})", source)
        allowed = {
            "Games/Skyrim/RTTI.h", "Games/Skyrim/RTTI.cpp",
            "Games/Skyrim/NetImmerse/NiObjectNET.h",
            "Games/Skyrim/NetImmerse/NiAVObject.h",
            "Games/Skyrim/NetImmerse/NiProperty.h",
        }
        for path in (ROOT / "Code/client").rglob("*"):
            if path.suffix not in {".h", ".cpp"}:
                continue
            relative = path.relative_to(ROOT / "Code/client").as_posix()
            if relative not in allowed:
                text = path.read_text(encoding="utf-8-sig")
                for name in ("NiObjectNET", "CreationClubMenu", "NiParticleSystem"):
                    self.assertNotIn(name, text, relative)
        for relative in allowed - {"Games/Skyrim/RTTI.h", "Games/Skyrim/RTTI.cpp"}:
            text = read("Code/client/" + relative)
            self.assertNotIn("Cast<", text)
            self.assertNotIn("RttiLocator<", text)


if __name__ == "__main__":
    unittest.main()
