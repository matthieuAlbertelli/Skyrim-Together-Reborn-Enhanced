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
