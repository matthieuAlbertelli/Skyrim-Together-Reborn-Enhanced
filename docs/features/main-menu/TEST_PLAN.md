# Main Menu acceptance and validation

> **Status:** canonical acceptance plan for [#86](https://github.com/matthieuAlbertelli/Skyrim-Together-Reborn-Enhanced/issues/86).
> Owner: native client / UI / maintainer QA.
> Executed results belong only in [STATUS](../../project/STATUS.md).

## Automated checks

Use the repository's Catch2 TPTests and Python unittest structural checks.
From the relevant checkout, explicitly use `-P .` for XMake when it is nested
inside another checkout. Use a new short local build directory for each mode,
with the source/receipt mapping described in the
[Runtime17TwoPc workflow](../../development/RUNTIME_1_7_TWO_PC_DEV.md).
Never reuse an accepted candidate's outputs. Example for an allocated Debug root:

```powershell
$r17Build = 'C:\STRE-R17\<new-run-id>\build'
xmake config -P . -m debug -y -o $r17Build
xmake build -P . -y -j 6
& "$r17Build/windows/x64/debug/TPTests.exe" "[main-menu]"
& "$r17Build/windows/x64/debug/TPTests.exe"
& "$r17Build/windows/x64/debug/TPTests.exe" "[main-menu-media]"
& "$r17Build/windows/x64/debug/TPTests.exe" "[main-menu-branding-assets]"
python Tools/Scripts/test_main_menu.py
python Tools/Scripts/audit_ck_packaging.py
git diff --check
```

Repeat the build and these tests in releasedbg with its own fresh root. Retain
commands, exit codes and nonzero test/assertion counts; a filter with no cases
is not PASS. Build/link client, launcher and server from the same source identity.
The native-adapter cases reject unsupported runtimes, altered call bytes,
wrong originals and out-of-range hooks. The production permission transaction
is exercised at each acquisition failure and success, verifying unchanged
originals on failure and reverse permission release. O02 cannot bypass the
mandatory campaign contracts.

The explicit hidden media suite
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
media in the Git index (including staged additions); `--package` checks all media
actually assembled. The approved background must match the canonical export
hash/size. Intermediate MP4s and po3 media are rejected. The uncleared local intro
must remain untracked and outside assembled distribution packages.

Structural checks cover Windows-only dependencies, shared renderer/input use,
absence of network/persistence and SWF changes, CK isolation, GPL notice, and
the Data package layout without po3 prototypes/DLLs. Check changed Markdown
relative links with the available documentation tooling.

## Progressive human trial on 1.7.104

The maintainer alone chooses when to deploy and launch. The porting mission
stops before either operation. Use the existing command from the primary
checkout, followed by its human Vortex handoff and installation verification:

```powershell
pwsh -NoProfile -File .\build-and-deploy-dev.ps1 -Runtime17TwoPc
```

The command rebuilds: record its actual candidate/receipt rather than assigning
the offline candidate's SHA to a later build. Keep the generated client/server
set together. No new account, Steam installation or general compatibility audit
is required. The installed presentation INIs/media need no change for this port.

| Stage | Maintainer observation |
|---|---|
| A — startup | Launch SkyrimTogether normally. Check animated background, logos/localized subtitle, native music, usable menu and normal Quit. If the local QA intro is present, observe playback/audio and keyboard skip; if absent, mark intro playback NOT TESTED, never PASS |
| B — intro and controls | In subsequent fresh processes, let the intro end naturally, then test skip while holding the key through transition. No unintended menu activation; cursor and controls return normally. Record controller skip only if actually tested. Check focus/resize as available |
| C — Solo return | Load the already-tested Solo character, return to Main Menu and confirm direct background/branding without intro replay. No repeat of the multiplayer qualification is requested |
| D — reversible fallback | With the game closed, the maintainer may back up the existing presentation.ini and temporarily set its existing Enabled setting to false. Launch, check usable vanilla menu and Quit, then restore the exact backup. No BSA or other mod file changes. Optional absent-media tests belong to the broader matrix below |

Record PASS/FAIL/NOT TESTED per observed component, including audible intro,
native hint, input routing and visual composition. Offline WARP/MF results do
not fill these cells. The local intro remains private and excluded from the
common payload until music permission is recorded. Do not ask for old
multiplayer logs; investigate only a new presentation session if an anomaly is
reported. Previous multiplayer acceptance and its scope remain unchanged.

## Broader human acceptance matrix

These additional cases describe coverage, not a request to repeat every test
before the progressive trial or to rerun previously accepted multiplayer paths.

Target the [reference platform](../../testing/COMPATIBILITY_MATRIX.md): Steam
1.7.104.0 / SKSE 2.3.1 with the qualified presentation adapters. The historical
1.6.1170 / SKSE 2.2.6 results do not establish 1.7 presentation acceptance.
Record source commit, binary hash, exact video hashes,
configuration, resolution/aspect, controller and relevant MainMenu logs.
Use the distributed background and separately approved local intro for private
QA; do not distribute the intro while music permission is pending. Keep po3
Main Menu Video inactive. The distributed package without intro must open
directly on background/branding and the usable vanilla menu.

1. Start a fresh process. Confirm fullscreen intro, audible/synchronized trailer
   audio, no vanilla menu/buttons/cursor interaction and no competing menu music.
2. Allow natural completion, then repeat in a new process with keyboard skip and
   again with controller skip. Hold skip/Enter/A through transition; nothing may
   activate invisibly. Test mouse clicks, thumbsticks and unrelated buttons too.
3. Confirm transition without native-background flash, menu entries unchanged,
   silent background looping and normal vanilla menu music. Observe several
   complete forward → reverse → forward cycles and both direction changes.
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
    try `BackdropEnabled=false`, opacity 0 / 0.35 / 1.0, scale limits and both
    offset signs. The entire backdrop stays in the left half; foreground layout,
    reveal timing and menu interaction stay unchanged. Omit/corrupt only the
    backdrop PNG: logos/text/video must continue. Restore defaults afterward.

A native render reset may intentionally disable presentation for that process;
vanilla must remain usable. Unknown runtime testing must never be reported as
support for any other runtime. Exact-target native proof does not establish
in-game presentation acceptance.

## Deployment acceptance

Use the existing Runtime17TwoPc route above. Do not invoke its Legacy route or
the CK importer. Its generated native/UI package comes from the selected 1.7
worktree and its managed content snapshot comes from installed A, with personal
configurations and campaign state protected. A reviewable offline XMake install
directory is not a deployed installation. No main-menu path enters the CK manifest.
Optional videos can be absent. The product payload currently includes only
`background.mp4`; `intro.mp4` awaits music permission. Public packages use the
tracked distribution source; Runtime17TwoPc applies its existing managed-content
allowlist to installed A. Neither route may include the private QA intro.
Verify the final MP4's size,
SHA256, H.264/MP4 format and absence of audio with `ffprobe -v error -show_format
-show_streams -of json <file>`; fully decode with `ffmpeg -v error -xerror -i
<file> -f null NUL` on Windows. These are offline checks, not runtime dependencies.
No po3 DLL/prototype config or intermediate forward/reverse/pingpong export may
enter the package. No deployment or in-game PASS may be inferred from a build.

## Historical Steam 1.7.104.0 lot-4 startup plan

The following original startup plan is retained as history. It is not current
authorization for Codex to launch applications or access either installation.
The current presentation trial is the maintainer-only A-D protocol above;
subsequent Solo/multiplayer attestations and their limits are recorded in STATUS.

These are **prepared checks**, not PASS evidence. Follow the local DEV
[native experiment procedure](../../development/EXPERIMENTAL_RUNTIME_1_7.md).
Lot 3 satisfies the static campaign ABI gate. Verify the actual deployed
candidate and dependencies; never bypass a subsequently missing condition.

### First main-menu-only runtime gate

The lot-4 authorization covers only the main PC and this bounded procedure:

1. Rehash exact runtime/SKSE/AL and candidate. Verify actual local DEV deployment,
   plugins, process state and exit access. The normal Windows account, Steam
   installation, Vortex staging, Documents/AppData and launcher HKCU are explicitly
   authorized. No separate Windows account or full game copy is required.
2. Use the official STRE launcher. Record process creation, image mapping,
   native hooks, SKSE/plugin initialization and CEF readiness separately.
3. Observe the menu entries and harmless keyboard/mouse navigation, then Quit
   normally. Never select New, Continue or Load; do not start a server.
   Ask for necessary human observations if the window is inaccessible.
4. Collect logs/exits/dumps and compare protected content/saves/state. Stop on
   wrong destination, memory-corruption symptoms or uncontrolled writes.
   Diagnose each failure before a targeted correction/rebuild/retest.
5. PASS requires the observed usable menu and normal exit, supported by startup
   logs without critical native errors. A build, live process or black screen
   cannot be PASS. Record disabled O02/O03/D01 presentation separately. Stop
   after this gate; broader gameplay requires later authorization.

### Later expanded scenarios

The following broader scenarios require separate authorization after startup
acceptance; they are not included in the main-menu-only gate.

1. In a separately authorized environment, record the exact EXE, SKSE, Address
   Library, native plugin and client/server hashes. Verify distinct early
   diagnostics for 1.6.1170, 1.7.99, another 1.7 patch, GOG/unknown distribution,
   missing/truncated/wrong-version Address Library and absent/wrong SKSE.
   With the exact lot-3 dependencies and all 18 static contracts satisfied,
   admission may proceed; removing required evidence must still reject before
   game mapping or hooks.
2. Only with the reviewed, explicitly authorized candidate:
   verify startup ordering and SKSE plugin initialization independently of DLL
   load; reject missing required plugins without weakening admission policy.
3. Run the existing intro/menu/skip scenarios with separately permitted media,
   keyboard/controller input, load/new game, and repeated return from gameplay.
   Record the intended vanilla fallback for each still-disabled optional
   presentation path. An absent custom intro/subtitle is not proof of a port.
4. Archive logs, captures, exact candidate identity and PASS/FAIL/BLOCKED per
   scenario. Historical 1.6 acceptance and an offline build cannot fill this lot.
