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
        self.assertLess(reset.index('!= "1.6.1170.0"'), reset.index("POINTER_SKYRIMSE"))
        camera = read("Code/client/Games/Skyrim/Interface/Menus/RaceSexMenu.h")
        camera = camera[camera.index("GetRaceSexMenuCamera("):]
        self.assertLess(camera.index('!= "1.6.1170.0"'), camera.index("Cast<RaceSexMenu>"))
        grab = read("Code/client/Games/Skyrim/PlayerCharacter.cpp")
        grab = grab[grab.index("bool PlayerCharacter::TryEndGrabObject()"):]
        self.assertLess(grab.index('!= "1.6.1170.0"'), grab.index("VersionDbPtr"))


if __name__ == "__main__":
    unittest.main()
