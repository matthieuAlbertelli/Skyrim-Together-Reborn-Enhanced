# Main Menu acceptance and validation

> **Status:** canonical acceptance plan for [#86](https://github.com/matthieuAlbertelli/Skyrim-Together-Reborn-Enhanced/issues/86).
> Owner: native client / UI / maintainer QA.
> Executed results belong only in [STATUS](../../project/STATUS.md).

## Automated checks

Use the repository's Catch2 TPTests and Python unittest structural checks.
From the relevant checkout, explicitly use `-P .` for XMake when it is nested
inside another checkout:

```powershell
xmake config -P . -m debug -y
xmake build -P . -y -j 6 TPTests
./build/windows/x64/debug/TPTests.exe "[main-menu]"
./build/windows/x64/debug/TPTests.exe
./build/windows/x64/debug/TPTests.exe "[main-menu-media]"
./build/windows/x64/debug/TPTests.exe "[main-menu-branding-assets]"
xmake build -P . -y -j 6 SkyrimImmersiveLauncher
python Tools/Scripts/test_main_menu.py
python Tools/Scripts/audit_ck_packaging.py
git diff --check
```

Repeat relevant build/tests in releasedbg. The explicit hidden media suite
requires Windows Media Foundation including the H.264 encoder; it creates and
removes a tiny synthetic silent MP4 and uses a WARP DX11 device. It covers actual
frame transfer, EOS, looping, missing/corrupt media and cancellation during load.
It does not prove audio audibility, Skyrim render order or native input routing.
The portable suite covers first entry, natural end/skip, return/reopen, missing
media, repeated errors, stalled/endless media, disabling, malformed config,
canonical paths, and held keyboard/mouse/gamepad input.
Localization tests cover French/English auto selection, explicit override,
English fallback, an added locale, UTF-8/BOM, plain-text markup characters,
ignored legacy key entries, independent subtitle fallback, invalid UTF-8 and
unavailable/oversized/malformed labels. Branding tests exercise first/return/early
exit timing, absent-background gating, deterministic fades, invalid clocks,
proportions, vertical centering and left-half anchoring at standard/ultrawide
aspects. Backdrop coverage includes section isolation, opacity/scale/offset
bounds, invalid/non-finite values, disabled/missing layers and preserving the
full feathered canvas at extreme settings. Windows WARP tests
generate their own PNG fixtures and verify actual straight-alpha texture pixels,
transparent-margin cropping, missing/corrupt/opaque/empty/oversized files and
null-device failure. The explicit asset smoke decodes all three PNGs from
the repository; run it from the repository root. It does not validate native
Scaleform or visual composition in Skyrim. Structural
checks verify no translated rendering strings, inert/optional native prompt
ownership, no redistributed native movie and that CursorMenu's draw gate chains to vanilla without
changing Win32/CEF cursor ownership. Default source checks inspect committed
media for redistribution; `--package` checks all media actually assembled.
Maintainer-owned staged/untracked QA clips must remain excluded from commits.

Structural checks cover Windows-only dependencies, shared renderer/input use,
absence of network/persistence and SWF changes, CK isolation, GPL notice, and
the Data package layout without po3 prototypes/DLLs. Check changed Markdown
relative links with the available documentation tooling.

## Human in-game acceptance

Target the [reference platform](../../testing/COMPATIBILITY_MATRIX.md): Steam
Skyrim 1.6.1170 / SKSE 2.2.6. Record source commit, binary hash, exact video hashes,
configuration, resolution/aspect, controller and relevant MainMenu logs.
Use separately approved local media; never publish prototype captures/assets
whose rights are still pending. Keep po3 Main Menu Video inactive.

1. Start a fresh process. Confirm fullscreen intro, audible/synchronized trailer
   audio, no vanilla menu/buttons/cursor interaction and no competing menu music.
2. Allow natural completion, then repeat in a new process with keyboard skip and
   again with controller skip. Hold skip/Enter/A through transition; nothing may
   activate invisibly. Test mouse clicks, thumbsticks and unrelated buttons too.
3. Confirm transition without native-background flash, menu entries unchanged,
   silent background looping and normal vanilla menu music.
4. Exercise Continue/New/Load/Settings/Credits/Quit as available, including the
   campaign-aware Continue/Resume path. Load gameplay, return to Main Menu and
   confirm direct background with no trailer replay. Repeat the round trip.
5. Test configured alternative skip bindings, loading-time skip, Alt-Tab/focus
   loss/regain, resolution/window changes and 16:9 plus 21:9 where available.
6. In separate fresh processes, omit intro; omit background; omit both; use
   corrupt intro and corrupt background. Confirm prompt fallback and usable
   vanilla controls. Restore files afterward.
7. Test unavailable/muted audio endpoint, unsupported codec, a stalled media
   source and disabled presentation. Audio problems must not hold the menu.
   Review bounded fallback logs and check no runaway retries.
8. With Skyrim in French then English (and explicit `Language=fr/en`), verify
   the **native key cartouche** followed by `Passer` / `Skip`, native font,
   bottom-right alignment and fade. No gamepad hint, no quantity-menu elements,
   and no hint on the background. Rebind skip to 57 and confirm native Space
   artwork; test a letter in French/English keyboard layouts. An unavailable
   key symbol or missing catalog must omit the hint without disabling skip.
   Confirm labels containing accents and `<`, `&`, `>` remain plain text.
   Test resize/focus transitions, 16:9 and 21:9 without stretched key art.
   Where practical, use a separate disposable mod profile with a missing or
   incompatible `sharedcomponents` override: only the hint may disappear.
   Do not rename/remove the live game's BSA or replace another mod's resources.

9. Keep the mouse stationary at the center, then move it during the intro:
   no native cursor may be drawn. Check immediate normal cursor behavior after
   Escape, gamepad skip, natural EOS, failure/disable and return from gameplay,
   without requiring mouse motion. Alt-Tab both during and after the intro;
   the desktop cursor and later CEF/vanilla interaction must behave normally.

10. With an approved **background without baked logo/text**, observe the first
    background frame, then the emblem, wordmark and localized subtitle reveal
    over 1.5 seconds. Menu actions and vanilla music must already be available.
    No branding may appear over the trailer or retained transition frame. Verify
    video → soft backdrop → logos → native subtitle → vanilla draw order. Check
    PNG transparency/edges, native subtitle font, accents/apostrophe, hierarchy,
    legibility, center-left grouping (around 25% width / 51% height), and clearance
    from right-side vanilla actions/news at 16:9, 21:9, windowed
    resize and Alt-Tab. Record an approved capture for visual review.
11. Leave during a partial reveal, load gameplay and return repeatedly: branding
    must be final on the first available background frame, without intro/reveal
    replay. In fresh processes omit each PNG, both PNGs, or the subtitle key;
    test corrupt/opaque images and a missing/invalid catalog. Only affected
    decorations may disappear (English fallback first for text), with background,
    native actions, music, cursor and skip behavior unaffected. Test disabled
    presentation and missing/corrupt background: no orphan branding on vanilla.

12. Check the backdrop against the dragon, fire and blue barrier: background
    detail must remain visible, with a soft organic edge and no rectangular
    clipping/halo. Confirm its fade follows the emblem. In separate launches
    try `BackdropEnabled=false`, opacity 0 / 0.35 / 0.45, scale limits and both
    offset signs. The entire backdrop stays in the left half; foreground layout,
    reveal timing and menu interaction stay unchanged. Omit/corrupt only the
    backdrop PNG: logos/text/video must continue. Restore defaults afterward.

A native render reset may intentionally disable presentation for that process;
vanilla must remain usable. Unknown runtime testing must never be reported as
1.7.x support. A separate porting mission owns that validation.

## Deployment acceptance

Before running the maintainer's local build-and-deploy-dev.ps1, compare all
auto-importable CK-owned outputs with repository hashes without importing.
Any unrelated difference blocks that script. It has maintainer-specific absolute
paths: a worktree build alone does not make it deploy the worktree.

Use existing XMake/install and playable-package commands for a reviewable local
package. Verify binaries under SkyrimTogetherReborn and Data content under
STRE/MainMenu plus STRE/Licenses. No main-menu path enters the CK manifest.
Optional videos can be absent. No po3 DLL/prototype config may enter the STRE
package. No deployment or in-game PASS may be inferred from a successful build.
