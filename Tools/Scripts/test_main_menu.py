"""Structural/packaging checks complementing TPTests and human Skyrim acceptance."""

import argparse
import configparser
import hashlib
from pathlib import Path
import re
import subprocess
import unittest


ROOT = Path(__file__).resolve().parents[2]
DATA = ROOT / "GameFiles/Skyrim"
PACKAGE = DATA
ASSEMBLED_PACKAGE = False


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
            if relative.startswith("stre/mainmenu/") and path.suffix.lower() == ".mp4":
                self.assertIn(relative, ("stre/mainmenu/intro.mp4", "stre/mainmenu/background.mp4"))

    def test_distribution_media_match_approved_exports(self):
        contract = read("docs/features/main-menu/README.md")
        if ASSEMBLED_PACKAGE:
            clips = list((PACKAGE / "STRE/MainMenu").rglob("*.mp4"))
        else:
            # Include staged distribution changes in pre-commit checks, while
            # leaving the maintainer's untracked, uncleared intro private.
            names = subprocess.check_output(
                ["git", "-c", f"safe.directory={ROOT.as_posix()}", "ls-files", "--",
                 "GameFiles/Skyrim/STRE/MainMenu"], cwd=ROOT, text=True)
            clips = [ROOT / name for name in names.splitlines() if name.lower().endswith(".mp4")]
        for clip in clips:
            self.assertEqual(clip.name, "background.mp4", "Intro music permission is pending; never package it")
            self.assertGreater(clip.stat().st_size, 0)
            self.assertLessEqual(clip.stat().st_size, 100 * 1024 * 1024)
            with clip.open("rb") as media:
                self.assertEqual(media.read(8)[4:], b"ftyp")
                media.seek(0)
                digest = hashlib.sha256()
                for chunk in iter(lambda: media.read(1024 * 1024), b""):
                    digest.update(chunk)
            self.assertIn(digest.hexdigest(), contract, "Update the canonical provenance record for a changed export")
            self.assertIn(f"{clip.stat().st_size:,}", contract)

    def test_localized_hint_catalog_and_renderer_boundary(self):
        catalog = configparser.ConfigParser()
        catalog.read(PACKAGE / "STRE/MainMenu/localization.ini", encoding="utf-8-sig")
        self.assertEqual(catalog["fr"]["SkipAction"], "Passer")
        self.assertEqual(catalog["en"]["SkipAction"], "Skip")
        self.assertFalse(any(key.startswith("key.") for section in catalog.values() for key in section))
        renderer = read("Code/client/Games/Skyrim/MainMenuPrompt.cpp")
        for literal in ('"ÉCHAP"', '"Passer"', '"ESC"', '"Skip"', '"fr"', '"en"'):
            self.assertNotIn(literal, renderer)
        self.assertIn('FormatText(uiMovie, "_root.STREIntroPrompt.Action", m_action.c_str()', renderer)
        self.assertIn('".text").c_str(), Value(apText)', renderer)
        self.assertNotIn("htmlText", renderer)
        imgui = read("Code/client/Services/Generic/ImguiService.cpp")
        video_draw = imgui[imgui.index("bool ImguiService::RenderMainMenuTexture"):imgui.index("void ImguiService::Reset")]
        self.assertNotIn("AddText", video_draw)

    def test_native_prompt_is_inert_optional_and_uses_installed_resources(self):
        prompt = read("Code/client/Games/Skyrim/MainMenuPrompt.cpp")
        header = read("Code/client/Games/Skyrim/MainMenuPrompt.h")
        for forbidden in ("QueueMessage(", "RegisterMenu(", "ShowCursor(", "HandleEvent(", '"startmenu"'):
            self.assertNotIn(forbidden, prompt)
        self.assertIn('"sharedcomponents"', prompt)
        self.assertIn('"$EverywhereMediumFont"', prompt)
        self.assertIn("keyboard, m_scanCode, art", prompt)
        self.assertIn("children.Names.empty()", prompt)
        self.assertIn("m_attempted = true", prompt)
        self.assertIn('GetLoadedVersionString() != "1.6.1170.0"', prompt)
        self.assertIn("UI_MESSAGE_RESULTS::kIgnore", header)
        self.assertIn("void RefreshPlatform() override {}", header)
        runtime = read("Code/client/Games/Skyrim/MainMenuRuntime.cpp")
        draw = runtime[runtime.index("void PostDisplayHook"):runtime.index("void CursorPostDisplayHook")]
        self.assertIn("presentation && presentation->Render()", draw)
        self.assertIn("s_prompt && presentation->HidesCursor()", draw)
        self.assertIn("s_prompt->ReleaseMovie()", draw)
        self.assertFalse(list(PACKAGE.rglob("sharedcomponents.swf")))

    def test_branding_localization_and_inert_draw_order(self):
        catalog = configparser.ConfigParser()
        catalog.read(PACKAGE / "STRE/MainMenu/localization.ini", encoding="utf-8-sig")
        self.assertEqual(catalog["fr"]["MainMenuSubtitle"], "La Compagnie de l’Enfant de Dragon")
        self.assertEqual(catalog["en"]["MainMenuSubtitle"], "Fellowship of the Dragonborn")
        prompt = read("Code/client/Games/Skyrim/MainMenuPrompt.cpp")
        for section in ("fr", "en"):
            self.assertNotIn(catalog[section]["MainMenuSubtitle"], prompt)
        self.assertIn('FormatText(uiMovie, path, m_text.c_str()', prompt)
        runtime = read("Code/client/Games/Skyrim/MainMenuRuntime.cpp")
        draw = runtime[runtime.index("void PostDisplayHook"):runtime.index("void CursorPostDisplayHook")]
        self.assertLess(draw.index("presentation->Render()"), draw.index("s_subtitle->Render"))
        self.assertLess(draw.index("s_subtitle->Render"), draw.index("s_postDisplay(apMenu)"))
        presentation = read("Code/client/MainMenu/MainMenuPresentation.cpp")
        input_code = presentation[presentation.index("bool Presentation::CapturesInput"):presentation.index("void Presentation::OpenCurrent")]
        self.assertNotIn("branding", input_code.lower())
        self.assertIn("m_brandingReveal.Enter(m_observedEpoch != 0)", presentation)
        self.assertIn("const bool background = !intro && !retained", presentation)

    def test_branding_package_has_optional_independent_png_assets(self):
        import struct
        header = read("Code/client/MainMenu/Branding.h")
        for name in ("Branding/emblem.png", "Branding/skyrim-wordmark.png", "branding_backdrop.png"):
            self.assertIn(f'"{name}"', header)
            path = PACKAGE / "STRE/MainMenu" / name
            if path.exists():  # omission remains a supported player package
                data = path.read_bytes()
                self.assertLessEqual(len(data), 16 * 1024 * 1024)
                self.assertEqual(data[:8], b"\x89PNG\r\n\x1a\n")
                width, height, depth, color = struct.unpack(">IIBB", data[16:26])
                self.assertEqual((depth, color), (8, 6))
                self.assertLessEqual(max(width, height), 4096)
                self.assertLessEqual(width * height, 4194304)
        texture = read("Code/client/MainMenu/BrandingTexture.cpp")
        for token in ("GUID_ContainerFormatPng", "GUID_WICPixelFormat32bppRGBA", "D3D11_USAGE_IMMUTABLE"):
            self.assertIn(token, texture)
        self.assertNotIn("GENERIC_WRITE", texture)

    def test_backdrop_uses_existing_pass_and_emblem_fade(self):
        config = configparser.ConfigParser()
        config.read(PACKAGE / "STRE/MainMenu/presentation.ini", encoding="utf-8-sig")
        self.assertTrue(config["Branding"].getboolean("BackdropEnabled"))
        self.assertEqual(config["Branding"].getfloat("BackdropOpacity"), 1.0)
        renderer = read("Code/client/Services/Generic/ImguiService.cpp")
        order = [renderer.index(token) for token in (
            "list.AddImage(apTexture", "drawBranding(aBackdrop", "drawBranding(aEmblem", "drawBranding(aWordmark")]
        self.assertEqual(order, sorted(order))
        self.assertIn("aOpacity.Emblem * aBackdropConfig.Opacity", renderer)
        presentation = read("Code/client/MainMenu/MainMenuPresentation.cpp")
        self.assertIn("m_config.Backdrop.Enabled && m_config.Backdrop.Opacity > 0", presentation)
        self.assertIn("m_directory / cBackdropFile, false", presentation)
        self.assertIn("m_backdrop = {}", presentation)

    def test_cursor_draw_gate_keeps_native_and_overlay_ownership(self):
        runtime = read("Code/client/Games/Skyrim/MainMenuRuntime.cpp")
        self.assertIn("FindAddressById(215246)", runtime)
        hook = runtime[runtime.index("void CursorPostDisplayHook"):runtime.index("bool MusicAtCreateHook")]
        self.assertIn("!presentation || !presentation->HidesCursor()", hook)
        self.assertIn("s_cursorPostDisplay(apMenu)", hook)
        presentation = read("Code/client/MainMenu/MainMenuPresentation.cpp")
        self.assertIn("return m_playingIntro && CapturesInput();", presentation)
        for text in (runtime, presentation):
            for call in ("ShowCursor(", "SetCursor(", "SetCursorVisibility(", "SetCursorVisible("):
                self.assertNotIn(call, text)

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
        ASSEMBLED_PACKAGE = True
    unittest.main(argv=[__file__, *remaining])
