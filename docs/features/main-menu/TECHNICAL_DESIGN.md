# Main Menu technical design

> **Status:** feature-local design for [#86](https://github.com/matthieuAlbertelli/Skyrim-Together-Reborn-Enhanced/issues/86).
> Owner: native client / UI. Actual evidence lives in [STATUS](../../project/STATUS.md).

## Integration audit and choice

`SkyrimTogetherClient` is already a static library linked into
`SkyrimTogether.exe` by `SkyrimImmersiveLauncher`. The launcher loads Skyrim
and supplies a stable game directory. `MainMenuRuntime` already owns the
native top-level return-to-menu boundary. That operation remains unchanged.

`BSGraphicsRenderer` already creates the renderer and calls
`RenderSystemD3D11::OnRender` at renderer End. `ImGuiImpl` owns the existing
context and the DX11 backend restores native render state. `TiltedUi` owns CEF;
the CEF overlay requires a loaded player/NiNode and is therefore unsuitable for
this boot menu without broad changes. The older `D3D11Hook/Present` helpers are
unused in this client; installing another swapchain hook would duplicate work.

Use a small process-local `MainMenuPresentation` plus `VideoPlayer` inside
the existing client. A local ImDrawList renders through the existing backend at
MainMenu PostDisplay, before the original movie. It does not create a context,
start a second ImGui UI frame, replace a swapchain or modify `startmenu.swf`.
The current D3D viewport determines the cover/crop each frame, including resize.
The intro suppresses the original PostDisplay; background always chains to it.
The last intro texture is retained during asynchronous background preparation
to avoid exposing the native background between clips.

A standalone SKSE plugin is unnecessary: STRE already has the device, native
hooks, input and packaging. No SKSE messaging dependency is added. XMake already
includes client sources recursively and excludes the client from Linux targets.
Only portable policy is compiled into Linux TPTests.

## Decoder and ownership

Use Windows Media Foundation Media Engine in frame-server mode. It schedules
decode and audio internally, exposes EOS/error notifications and transfers
frames to a BGRA DX11 texture using the existing device. This avoids OpenCV's
additional build/package/dependency surface, duplicated decode/audio threads,
and a separate codec DLL distribution. No OpenCV, FFmpeg, CommonLib or new
package version is introduced. Windows SDK import GUIDs/COM libraries are used;
`mfplat.dll` is loaded explicitly from System32, so its absence fails open
instead of preventing the executable from loading.

The runtime adapter owns one presentation object for the process. Controller and
media mutations occur on the render path. Native menu/input callbacks exchange
atomic values; the input latch belongs only to MenuControls' input thread.
Media callbacks own a separate reference-counted atomic notification object and
never call Skyrim, ImGui or the immediate D3D context. Shutdown releases the
engine/texture and notification ownership, with no detached STRE decode thread.
The DXGI manager shares the existing device with Media Foundation and enables
the device's multithread protection for that use.

Microsoft API contracts:
[frame transfer](https://learn.microsoft.com/en-us/windows/win32/api/mfmediaengine/nf-mfmediaengine-imfmediaengine-transfervideoframe),
[events](https://learn.microsoft.com/en-us/windows/win32/api/mfmediaengine/ne-mfmediaengine-mf_media_engine_event).
Stream-rendering error parameters are unspecified: mute audio and rely on the
bounded video watchdog rather than guessing a stream ID.

## Policy and native boundary

`PresentationPolicy` owns portable configuration, held-button suppression and:

```text
WaitingForMainMenu -> PlayingIntro -> TransitionToMenu -> PlayingBackground
                            |                                  |
                         skip/EOS/error                      error/stall
                            |                                  |
                            +-> background attempt       VanillaFallback
menu close -> WaitingForMainMenu (intro-attempt flag retained)
unsupported/renderer failure -> Disabled
```

Missing intro begins directly at background/fallback. Missing background never
delays menu interaction. Timeouts and resource bounds are declared in the
feature's product contract, not copied into configuration. No retry can reset
the intro's original deadline. Close/reopen edges are recorded even between
render frames, so a menu refresh cannot replay the intro.

`MainMenuRuntime.cpp` and its `MainMenuPrompt.cpp` adapter contain feature-specific
version checks, relocation IDs, call-site offsets and virtual slots. Both accept
exactly `1.6.1170.0`. The hook adapter validates readable/executable memory, direct-call opcodes and rel32 range
before changing the MainMenu virtual entries or either music call site. All
write permissions are acquired before any patch; failure keeps originals.
Each music call site retains its own original.

Main Menu Show/Hide messages delimit presentation. Intro input is consumed at
the existing MenuControls ProcessEvent hook before native dispatch; the public
InputEvent ABI was extracted from CampaignSaveTrace into a shared header. No
second hook is stacked on that function. Scaleform/user input messages to the
Main Menu are also suppressed during capture. Held buttons are swallowed through
their release; native menu actions resume with fresh presses.

The music hooks return the upstream predicate's “already playing” result only
while intro audio owns presentation. The normal native predicate is restored
afterward; no user volume, music setting or campaign lock is modified.
A short renewable render lease prevents stale input/music capture. End-of-frame
maintenance stops media after menu close or lost rendering. Device reset
disables this optional presentation rather than attempting unsafe device recovery.

The existing CampaignMainMenuEnteredEvent, Continue interception and
RequestSkyrimMainMenu remain their owners' contracts. There is no server,
network, save, Papyrus, ESP or shared authority in this feature.

## Intro hint and cursor ownership

Localization audit: the Angular UI uses Transloco with `assets/i18n/*.json`,
an English fallback, and a selected locale stored in CEF localStorage. Native
notifications send translation keys to that browser. There is no synchronous
native translation service available before the first menu. Reading browser
storage or starting a CEF surface for this hint would couple startup to that UI.

Use a small feature-local UTF-8 `localization.ini` catalog, with the native
console's existing bundled SimpleIni parser (char-only, no additional conversion
library). Reuse the established locale/key/fallback pattern. `Language=auto`
matches the catalog's `SkyrimLanguage` aliases against the existing native
INISettingCollection; explicit `Language` selects a catalog section. Both files
are read once with bounds. Presentation labels share this resolver with independent per-key fallback; an unavailable hint
does not alter presentation state.

### Native prompt audit and choice

The installed 1.6.1170 `Skyrim - Interface.bsa` was inspected read-only. Its
`startmenu.swf` embeds `Components.CrossPlatformButtons` and its own key exports;
menu ActionScript chooses art through `SetPlatform`/`PCArt` and composes the
control and label. `Shared.ButtonTextArtHolder` uses the delegate callback
`GetButtonFromUserEvent` for semantic prompts. These are movie components, not a
standalone native `ShowPrompt` service. `IMenu::kHasButtonBar` and
`kIsTopButtonBar` describe menu-stack/button-bar participation; setting them
neither creates a movie nor supplies its controls. A menu-independent invocation
of the complete vanilla button bar is not established by the exposed STRE ABI.
The stock `sharedcomponents.swf` also exports key cartouches and a generic
`Button`; its older CrossPlatformButtons table does not cover Escape. Reusing a
full interactive menu/control would require unwanted context/callback behavior
or adapting its internal script contract.

Choose preference **2: installed Skyrim resources rendered by Scaleform**.
`IntroPrompt` loads a private `sharedcomponents` movie through the same
BSScaleformManager API already used by TradePreviewHostMenu. It hides the
library's existing root display children (including quantity-menu samples), then
attaches only the native key symbol and a plain text field using
`$EverywhereMediumFont` with embedded fonts enabled. No new SWF, Flash compiler,
asset extraction at runtime, plugin, hook or replacement menu is required.
The existing ImGui path draws video/black cover and optional branding images,
without text.

The host has no menu flags, registration, input handling or delegate actions;
`RefreshPlatform` is deliberately inert. It never renders or modifies the Main
Menu movie; that movie is consulted only for the current viewport. The existing
PostDisplay seam draws the prompt after a successful intro video pass, guarded
by the same live intro lease as cursor suppression. Skip/EOS/fallback, menu
close, disabled/reset presentation and a lost render lease cease drawing and
release the private movie/delegate on the render path. Loading is attempted at
most once per process. Resource/API/layout failure disables only the hint and
logs once; it never changes the presentation controller or input latch.
Viewport changes relayout the key and action without stretching the glyphs.
Long labels that cannot fit safely omit the hint.

The active keyboard's `BSInputDevice::GetKeyMapping` resolves the configured
`SkipKeyboard` scan code to the native export name. The installed English and
French keyboard definitions both resolve scan code 1 to `Esc`, and 57 to `Space`;
letter mappings differ with keyboard layout. The catalog therefore contains no
key labels. Missing exports are detected by the attached clip's dimensions;
there is no guessed substitute or custom ImGui fallback.

Retain scan-code skip. The installed PC `controlmap.txt` maps Menu Mode `Cancel`
through both gameplay `Tween Menu` and `Pause`, while `Back` is also the name of
a gameplay movement action. A semantic replacement would depend on the active
context, potentially add Tab or follow gameplay remaps, and change the validated
Escape/configured-binding contract. No improvement justifies that change here.
The native device lookup improves display correctness without dispatching input.

ABI references, used for interface verification rather than copied components:
[BSScaleformManager](https://github.com/CharmedBaryon/CommonLibSSE-NG/blob/main/src/RE/B/BSScaleformManager.cpp),
[GFxMovie](https://github.com/CharmedBaryon/CommonLibSSE-NG/blob/main/include/RE/G/GFxMovie.h),
[GFxMovieView](https://github.com/CharmedBaryon/CommonLibSSE-NG/blob/main/include/RE/G/GFxMovieView.h),
[GFxValue](https://github.com/CharmedBaryon/CommonLibSSE-NG/blob/main/include/RE/G/GFxValue.h),
[BSInputDevice](https://github.com/CharmedBaryon/CommonLibSSE-NG/blob/main/include/RE/B/BSInputDevice.h).
The optional adapter checks exact runtime, native pointers and required methods
before using these APIs. It shares the presentation hook memory preflight.
All acquired managed GFx values, the movie and its delegate are released.
No CommonLib binary dependency or new native patch is introduced.

Resource provenance: cartouches and fonts remain Bethesda resources owned by the
installed game and are read through its loader. STRE redistributes none of them,
no extracted/recompiled SWF, and no decompiled ActionScript. New adapter code is
STRE GPL code; the existing Main Menu Video attribution is unchanged.

### Cursor ownership

Cursor audit: `UiSurfaceService::ApplyInputCapture` owns the CEF texture cursor
and existing Win32 hiding for interactive overlay surfaces. Its focus handling
lives in InputService. The native Main Menu has a separate Scaleform
[CursorMenu](https://github.com/CharmedBaryon/CommonLibSSE-NG/blob/main/include/RE/C/CursorMenu.h),
rendered through inherited
[IMenu::PostDisplay](https://github.com/CharmedBaryon/CommonLibSSE-NG/blob/main/src/RE/I/IMenu.cpp).
The native MenuCursor Win32 visibility counter is another mechanism; it must
not be borrowed to hide this movie.

The 1.6.1170 adapter now also preflights CursorMenu's primary vtable (AE ID
215246), slot 6, before any presentation patch. Its hook skips only that draw
while the published state is PlayingIntro and the existing input/render lease
is live. Close, skip, EOS, fallback, disable/reset and lease expiry therefore
chain to the original draw without restoring a cached visibility value.
No ShowCursor/SetCursor call, UI message, cursor flag mutation or CEF cursor
override is added. Alt-Tab cannot unbalance a counter owned by this feature;
native/overlay focus and lifecycle handling continue normally. The visual result
and focus transitions still require the feature's human acceptance matrix.

## Branding composition

`Presentation` owns three optional `BrandingTexture` objects and a portable
`BrandingReveal`. The existing PostDisplay pass draws video, backdrop, emblem and wordmark
through one local ImGui draw list/backend; `MainMenuRuntime` then draws the
native subtitle and finally invokes the unchanged vanilla Main Menu display.
There is no new hook, context, device, input surface or registered native menu.
`SubtitleOpacity()` is a render-thread frame result, never a capture lease.
Missing components do not feed errors into the video controller.

`BrandingReveal` starts only on an actual background texture, excluding the
retained intro frame. The existing menu-close epoch distinguishes the first
visit from every return, including a close/reopen between render frames or an
exit during the intro/reveal. Timings and viewport-relative left-half layout are
pure functions tested independently. The product contract owns their values.
Resize recomputes layout without restarting the animation. Assets and the inert
subtitle movie remain cached across gameplay and are released on reset/destruction;
failed loads are not retried in a frame loop.

The shared layout anchors every foreground layer at one quarter of viewport
width and an approximately centered vertical group. A 16:9 size cap preserves
proportions on ultrawide without dragging the group toward the screen center.
`LayoutBackdrop` fits the complete texture into the left half and preserves its
aspect ratio, including bounded scale/offset adjustments. The existing bounded
INI parser reads `[Branding]` independently from `[Presentation]`; non-finite,
out-of-range or malformed values retain defaults. These knobs tune a replacement
background's contrast without exposing general logo layout or reveal settings.

Backdrop alpha multiplies the PNG's alpha, configured opacity and **existing
emblem fade**. No additional clock/state or input policy is introduced. The
texture is attempted once with the other branding resources, only when enabled
and opacity is positive. Failure affects only the shade. No texture load occurs
in the per-frame draw list; no particle system, shader or new dependency is added.

Image-loader audit: TiltedUI already uses Windows WIC through DirectXTK for its
cursor PNG. That helper produces a GPU texture but does not expose pixels for
alpha bounds. This feature uses the same OS codec directly, with a bounded
in-memory PNG snapshot, dimension checks before RGBA allocation, alpha-bound
measurement and one immutable DX11 upload. Emblem/wordmark loads trim export
margins; the backdrop opts out of trimming to preserve the full feathered
canvas, including sub-threshold alpha at its perimeter. No external decoder package or
DirectXTK upgrade is needed. `windowscodecs` is a Windows-only system link.
[WIC format conversion](https://learn.microsoft.com/en-us/windows/win32/api/wincodec/nf-wincodec-iwicformatconverter-initialize)
produces straight RGBA for the existing ImGui alpha blend; no premultiplied-alpha
mismatch, GPU readback, file rewrite or runtime asset extraction is involved.
COM initialization balances only its own successful call and accepts an existing
STA. HRESULTs are logged once per image. File/pixel budgets cap normal decode
work; as with other in-process OS decoders, there is no hard CPU preemption.

`PresentationMovie` factors the existing private library load, child hiding,
viewport copy and movie/delegate release from `IntroPrompt`. `MenuSubtitle`
reuses that inert host plus the same plain-text formatter/native font. It needs
no keyboard mapping, so missing keyboard art cannot suppress the subtitle.
The intro prompt still releases at intro exit and retains its own once-only
initialization guard. Both hosts stay behind the same 1.6.1170 adapter and use
only previously audited native methods. No new native relocation is introduced.
A library/font/text-layout failure affects only its own display component.

`ResolveText` parses the catalog once for both labels. Locale selection is shared;
each key independently falls back to English, then empty. The same size,
single-line and valid UTF-8 checks apply to both. Native labels use `.text`, never
HTML or script evaluation. Translations and images remain separate from video.
PNG provenance and the unbranded-background requirement belong to
[the asset record](README.md#asset-provenance-gate); the Main Menu Video
attribution below is unchanged.

## Provenance and licensing

Audited upstream:
[powerof3/MainMenuVideo](https://github.com/powerof3/MainMenuVideo/tree/ec692f0745972ba3b381e2b1df5c4c56218ee8e0),
commit `ec692f0745972ba3b381e2b1df5c4c56218ee8e0` (master rechecked 2026-09-22),
powerofthree, GPL-3.0-or-later per its vcpkg manifest.

Reviewed Manager, VideoPlayer, Hooks, ImGui Renderer and CMake/vcpkg inputs.
The adaptation is confined to the before-menu PostDisplay ordering and the two
menu-music predicate interception points in MainMenuRuntime, with attribution in
that source. STRE supplies version/preflight checks, separate originals,
process-local policy, input suppression and existing-renderer integration.
The decoder, config and controller are new STRE code; upstream OpenCV decoding,
threading, whole Manager, separate ImGui renderer, DLL and assets are not copied.

[NOTICE](../../../NOTICE.md) records the attribution. The upstream GPL text is
retained at
[MainMenuVideo-GPL-3.0.txt](../../../GameFiles/Skyrim/STRE/Licenses/MainMenuVideo-GPL-3.0.txt)
and travels with Data in the existing package flow. STRE remains GPL-3.0-or-later.
Video provenance is separate and belongs to the [product contract](README.md).

## Build, packaging and ADR

Client Windows links add `mfuuid`, `ole32`, `oleaut32` and `windowscodecs`. The hidden
Windows media smoke in TPTests additionally uses the OS encoder to generate a
temporary H.264 fixture; it is not a runtime dependency or a shipped video.
No executable/DLL is added to Data/SKSE/Plugins. The existing playable workflow
copies GameFiles/Skyrim to Data and binaries to SkyrimTogetherReborn. The existing
XMake install and dev staging flow therefore need no new deploy script.

Repo-authored INI/media never belong in the Alternate Start CK manifest or its
auto-import filter. Explicit CK import is only for files actually authored in
live Data. Never delete another mod's po3 files; any future prototype cleanup
must be restricted to verified STRE-owned staging roots.

No ADR is proposed: this feature extends the existing MainMenuRuntime boundary
and rendering/input infrastructure without a new plugin, global pipeline,
third-party decoder dependency, authority or persistent contract. Under the
[ADR policy](../../architecture/ADRs/README.md), these reversible local choices
belong here. A future runtime port edits this adapter and validates its hooks,
not the state machine or decoder. 1.7.x is explicitly unvalidated.
