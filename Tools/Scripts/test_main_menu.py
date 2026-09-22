"""Structural/packaging checks complementing TPTests and human Skyrim acceptance."""

import argparse
from pathlib import Path
import re
import unittest


ROOT = Path(__file__).resolve().parents[2]
DATA = ROOT / "GameFiles/Skyrim"
PACKAGE = DATA


def read(path):
    return (ROOT / path).read_text(encoding="utf-8-sig")


class MainMenuStructure(unittest.TestCase):
    def test_optional_namespace_and_defaults(self):
        config = (PACKAGE / "STRE/MainMenu/presentation.ini").read_text()
        self.assertIn("[Presentation]", config)
        self.assertIn("SkipKeyboard = 1", config)
        self.assertIn("SkipGamepad = 8192", config)
        policy = read("Code/client/MainMenu/PresentationPolicy.h")
        for name in ("STRE/MainMenu", "intro.mp4", "background.mp4"):
            self.assertIn(f'"{name}"', policy)

    def test_no_prototype_or_external_plugin_in_package(self):
        for path in PACKAGE.rglob("*"):
            if not path.is_file():
                continue
            name = path.name.lower()
            relative = path.relative_to(PACKAGE).as_posix().lower()
            self.assertNotIn("po3_mainmenuvideo", name)
            self.assertFalse(relative.startswith("mainmenuvideo/"), relative)

    def test_pending_media_rights_prevent_distribution(self):
        contract = read("docs/features/main-menu/README.md")
        if "Pending; do not distribute" in contract:
            self.assertEqual(list((PACKAGE / "STRE/MainMenu").glob("*.mp4")), [])

    def test_no_scaleform_replacement(self):
        self.assertEqual(list(DATA.rglob("startmenu.swf")), [])
        self.assertEqual(list(PACKAGE.rglob("startmenu.swf")), [])

    def test_ck_manifest_is_not_main_menu_packaging(self):
        manifest = read("GameFiles/STRE_AlternateStart.manifest.txt").lower()
        for fragment in ("mainmenu", ".mp4", "presentation.ini"):
            self.assertNotIn(fragment, manifest)

    def test_windows_dependencies_stay_in_client_or_conditional_tests(self):
        entry = read("Code/xmake.lua")
        self.assertRegex(entry, r'if is_plat\("windows"\) then\s+includes\("client"\)')
        tests = read("Code/tests/xmake.lua")
        self.assertRegex(tests, r'if is_plat\("windows"\) then\s+add_files\("../client/MainMenu/VideoPlayer.cpp"\)')
        for path in ("xmake.lua", "Code/common/xmake.lua", "Code/server/xmake.lua"):
            self.assertNotIn('"mfplat"', read(path))
            self.assertNotIn('"opencv', read(path).lower())

    def test_decoder_does_not_require_media_dll_at_process_load(self):
        client = read("Code/client/xmake.lua")
        self.assertNotIn('"mfplat"', client)
        self.assertNotIn('"mfreadwrite"', client)
        video = read("Code/client/MainMenu/VideoPlayer.cpp")
        self.assertIn('LoadLibraryExW(L"mfplat.dll"', video)
        self.assertIn("LOAD_LIBRARY_SEARCH_SYSTEM32", video)

    def test_one_existing_renderer_and_input_hook(self):
        for path in (ROOT / "Code/client/MainMenu").glob("*"):
            text = path.read_text()
            self.assertNotIn("ImGui::CreateContext", text)
            self.assertNotIn("D3D11CreateDevice", text)
            self.assertNotIn("ImGui::NewFrame", text)
        trace = read("Code/client/Services/Generic/CampaignSaveTraceService.cpp")
        hook = trace[trace.index("    HookMenuControlsProcessEvent,"):]
        self.assertLess(hook.index("ConsumePresentationInput"), hook.index("s_realMenuControlsProcessEvent"))
        self.assertNotIn("struct ButtonInputEvent :", trace)

    def test_presentation_has_no_shared_state_dependency(self):
        for path in (ROOT / "Code/client/MainMenu").glob("*"):
            includes = re.findall(r"#include[^\n]+", path.read_text())
            for include in includes:
                self.assertNotRegex(include, r"Campaign|Transport|Network|Papyrus|SaveLoad")

    def test_runtime_fence_and_provenance(self):
        runtime = read("Code/client/Games/Skyrim/MainMenuRuntime.cpp")
        self.assertIn('GetLoadedVersionString() != "1.6.1170.0"', runtime)
        sha = "ec692f0745972ba3b381e2b1df5c4c56218ee8e0"
        self.assertIn(sha, runtime)
        self.assertIn(sha, read("NOTICE.md"))
        license_file = PACKAGE / "STRE/Licenses/MainMenuVideo-GPL-3.0.txt"
        self.assertIn("GNU GENERAL PUBLIC LICENSE", license_file.read_text())
        self.assertIn("Version 3, 29 June 2007", license_file.read_text())


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--package", type=Path, help="Optional assembled Data root")
    args, remaining = parser.parse_known_args()
    if args.package:
        PACKAGE = args.package.resolve(strict=True)
    unittest.main(argv=[__file__, *remaining])
