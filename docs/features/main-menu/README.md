# STRE startup and Main Menu presentation

> **Status:** accepted v1 product contract. Owner: native client / UI.
> Delivery: [issue #86](https://github.com/matthieuAlbertelli/Skyrim-Together-Reborn-Enhanced/issues/86).
> Implementation and validation evidence: [STATUS](../../project/STATUS.md).

On the first Main Menu entry in a Skyrim/STRE process, play the STRE trailer
fullscreen with its audio. Keep the vanilla menu invisible and non-interactive
until natural completion or a configured keyboard/controller skip. Then display
the original vanilla menu actions above a silent, looping STRE video, with
Skyrim's own menu music. Returning from gameplay goes directly to the background.
There is no menu-action redesign or replacement Scaleform movie in this slice.

The intro-attempt flag belongs only to this process's presentation controller.
It is never saved to Skyrim, Papyrus, an ESP, the server or campaign state.
This local presentation does not alter the campaign invariants in
[ADR-0018](../../architecture/ADRs/ADR-0018-fixed-roster-coordinated-checkpoint-recovery.md),
the existing Return to Main Menu action, or the campaign-aware vanilla load path.

## Optional media and configuration

The repository's Data source is `GameFiles/Skyrim/`:

```text
STRE/MainMenu/presentation.ini
STRE/MainMenu/localization.ini
STRE/MainMenu/intro.mp4       (optional; rights required)
STRE/MainMenu/background.mp4  (optional; rights required; no baked branding)
STRE/MainMenu/Branding/emblem.png
STRE/MainMenu/Branding/skyrim-wordmark.png
STRE/MainMenu/branding_backdrop.png
```

Use local MP4/H.264 video and, for the intro, Windows-supported audio such as AAC.
The current resource bound is 4096 pixels per dimension and 8,847,360 pixels per
frame. The image fills the active render viewport while preserving aspect ratio;
edges may be cropped on a different aspect ratio.

Configuration is read once per process. `[Presentation]` supports `Enabled`
and `IntroAudio` as `true/false`, `SkipKeyboard` as a decimal Skyrim keyboard
scan code (1–255, default Escape = 1), and `SkipGamepad` as one decimal
Skyrim/XInput button bit (1–32768, default B = 8192). Background audio is always
disabled in this slice. Invalid values retain safe defaults; files over 4096 bytes
use defaults. Paths and network URLs are not configurable.

The intro alone displays a native Skyrim key cartouche followed by the localized
skip action at the bottom right, with a 350 ms fade-in. No gamepad hint is
shown; configured gamepad skip remains active. The prompt uses an independent,
display-only Scaleform view with the installed `sharedcomponents.swf` key art
and Skyrim's `$EverywhereMediumFont`. It never opens another vanilla menu,
changes `startmenu.swf`, or enables menu actions. The native cursor draw is
omitted only while the intro is active; normal rendering resumes on every exit.

`Language = auto` in `[Presentation]` follows Skyrim's `sLanguage:General`.
An explicit locale such as `fr` or `en` overrides it for this boot presentation;
it does not change the CEF overlay's language setting. UTF-8 `localization.ini`
contains locale sections with `SkyrimLanguage`, `SkipAction` and
`MainMenuSubtitle`. The skip action is `Passer` in
French, `Skip` in English. Add a section to add a locale without renderer edits.
The game supplies the key art for the configured scan code through its active
keyboard device, including keyboard-layout naming. No `Key.*` translations or
second binding table are maintained; old catalog key entries are ignored.
The native key resource owns its appearance (including the `Esc` cartouche),
so the earlier literal `[ÉCHAP]` / `[ESC]` text contract is superseded.

Missing/invalid action labels fall back to `en`. If the label, keyboard mapping,
key art or optional Scaleform view is unavailable, omit the hint without changing
video, skip, cursor or vanilla fallback. Unknown art never displays another key.
The catalog is limited to 16 KiB and each label to 128 valid UTF-8 bytes on one line.
They are assigned as plain text, not HTML. Native font coverage comes from the
player's installed Skyrim font configuration.

Skip remains the configured keyboard scan code (default Escape) and gamepad
button. Native `Cancel` can resolve to several context-dependent controls,
including Tab and Escape; replacing the validated binding with that action would
broaden/change it without a demonstrated robustness gain. The native prompt
therefore represents the existing binding, without dispatching Cancel/Back.

## Independent branding

The background is a silent animated plate with **no logo or text baked in**.
Optional transparent PNGs and a real localized subtitle form a separate,
non-interactive group. Draw order: video → soft dark backdrop → emblem → SKYRIM
wordmark → native subtitle → unchanged vanilla menu.
No intro branding is added over the trailer.

On the first Main Menu visit, the first decoded background frame starts a short
sequence: 150 ms with background alone, emblem fade from 150–600 ms, wordmark
fade from 600–1050 ms, subtitle fade from 1050–1500 ms. These are simple linear
fades; menu actions are already usable throughout. Loading or a retained intro
frame never starts the reveal. Every later Main Menu visit displays the final
branding as soon as the background is available, even if the first visit ended
before its reveal. Reveal timings and foreground layout remain fixed.

The group is anchored at **25% of viewport width**, entirely in the left half.
Its visual center sits around 51% of viewport height (layout anchor 52.5%).
Sizes scale proportionally and cap at 16:9 dimensions on ultrawide displays;
resize updates every layer together. The right-side menu/news area stays clear.
The subtitle uses Skyrim's `$EverywhereMediumFont` in muted gold, with bounded
fitting for longer translations. Final balance and font coverage require in-game QA.

The dedicated `branding_backdrop.png` is a static organic black/gray mist with
broad alpha feathering. It fades with the emblem, using the same existing draw
pass. Its entire canvas is preserved, avoiding a cut through the soft edge.
`[Branding]` in `presentation.ini` provides only contrast/placement tuning for
this backdrop; it never moves the logos or changes their reveal timings:

| Setting | Default | Accepted values |
|---|---|---|
| `BackdropEnabled` | `true` | `true` / `false` |
| `BackdropOpacity` | `0.35` | 0–1, multiplies PNG alpha and emblem fade |
| `BackdropScale` | `1.0` | 0.5–1.5, uniform scaling |
| `BackdropOffsetX` | `0` | −0.25–0.25, fraction of viewport width |
| `BackdropOffsetY` | `0` | −0.25–0.25, fraction of viewport height |

Size and offsets are further fitted inside the viewport's left half, keeping
all feathering on-screen. Invalid/non-finite values retain defaults; read once
at startup. Disabled or zero-opacity backdrops are not loaded. Missing/corrupt
backdrop omits only the shade, without hiding branding or changing video/input.
There is no particle animation, smoke shader or background-video modification.

`MainMenuSubtitle` uses the same automatic/explicit locale and per-key English
fallback as the skip action:

- French: `La Compagnie de l’Enfant de Dragon`.
- English: `Fellowship of the Dragonborn`.

Each PNG is optional independently; a missing/corrupt/opaque/empty image or GPU
failure omits that image. A missing/invalid subtitle omits text after attempting
English fallback. None of these failures changes video, audio, input, cursor or
vanilla-menu state. Branding is absent when presentation is disabled or no
background frame is available. Assets are attempted once per process and cached
for later visits; restart after replacing them.

PNG limits: 16 MiB compressed, 4096 pixels per dimension and 4,194,304 decoded
pixels. Straight RGBA alpha is preserved. The loader trims transparent export
margins of the emblem/wordmark in memory using alpha >= 8/255 plus one pixel of
padding; the backdrop keeps its complete canvas; it never
rewrites the originals or merges them with video. Entirely opaque or invisible
exports are omitted rather than guessing a black-background removal.

## Failure behavior

A missing or broken intro releases the menu and tries the background. A missing
or broken background exposes vanilla. Loading/frame stalls expire after eight
seconds; an intro without EOS expires after ten minutes even if frames continue.
A failed intro with audio gets at most one silent retry within the same deadline.
Audio component failure mutes video audio and leaves surviving video running.
Repeated background failure is terminal for the process.

Skip is accepted during loading too. Its press, held repeats and release do not
reach vanilla menu controls. Rendering failure or loss of the render heartbeat
releases capture; presentation never acquires a gameplay/campaign lock.
Initialization/device failure disables presentation for the process.

The implementation target is Steam Skyrim 1.6.1170 / SKSE 2.2.6. Other runtime
versions disable these presentation hooks; this is not a promise that the rest
of STRE can start on them. 1.7.x requires a separate port and validation.
Keep `po3_MainMenuVideo.dll` inactive when testing STRE presentation: detecting
that external renderer disables this feature without deleting another mod.

## Asset provenance gate

Video redistribution remains pending; local QA copies are not distribution
sources. The original `MainMenuVideo/STRE_Menu_Background2.mp4` prototype does
not define the final paths. The current local background still contains baked
branding and must be replaced by an approved clean export for final visual QA;
it must not be edited or renamed automatically.

| Required provenance | intro.mp4 | background.mp4 |
|---|---|---|
| Author / rights holder | TBD — maintainer confirmation | TBD — maintainer confirmation |
| Creation date | TBD | TBD |
| Tools and versions | TBD | TBD |
| Source/master/export procedure | TBD | TBD |
| License / redistribution permission | Pending; do not distribute | Pending; do not distribute |
| Third-party visuals/music/material | TBD | TBD |
| Restrictions and credits | TBD | TBD |
| Approved export hash and size | Pending approved export | Pending clean export |

The maintainer authorized committing and redistributing both PNGs with STRE on
2026-09-22. They are independent STRE Main Menu UI/branding assets, supplied
unchanged; this authorization does not extend to either MP4.

| Provenance | emblem.png | skyrim-wordmark.png |
|---|---|---|
| Author / responsible party | STRE project under maintainer editorial direction | Same |
| Tool | ChatGPT / OpenAI image generation | Same |
| Source / process, as reported by maintainer | Generated from the supplied replacement-logo reference, requesting the new emblem alone | Generated separately from the corresponding art direction/reference, requesting the isolated SKYRIM wordmark |
| Redistribution | Explicitly authorized by maintainer for STRE | Same |
| Creation date / model / tool version | TBD — not supplied | TBD — not supplied |
| Exact reference identity / source rights / credits / separate asset-license identifier | TBD — maintainer metadata pending | TBD — maintainer metadata pending |
| Supplied export | 1254 × 1254 RGBA, 773,485 bytes | 1672 × 941 RGBA, 579,294 bytes |
| SHA256 | `3d9dcb06307c11270b4ba4854fccd3bc0e77bc092d357730690f9a01618d66b3` | `41f3da1539e6220a7df45c4505e41c0d7004a8b108acc660ecc141cbf3918203` |

### Dark backdrop provenance

| Field | branding_backdrop.png |
|---|---|
| Responsible party | STRE project; generated in the requested Main Menu UX iteration |
| Date | 2026-09-22 |
| Tool | OpenAI built-in image generation; exact model/version not exposed |
| Source material | Original text instructions below; no external/reference image. Refinement uses only the first generated candidate |
| License | New STRE contribution under GPL-3.0-or-later, consistent with repository licensing |
| Output rights check | [OpenAI Europe Terms, Content](https://openai.com/policies/eu-terms-of-use/#content), checked 2026-09-22: output rights assigned to the user as between the user and OpenAI, to the extent permitted by law |
| Credits / restrictions | Retain STRE provenance and AI-generation disclosure; no third-party input asset or separate asset credit |
| Export | Original generated PNG copied unchanged to the Data source; 1254×1254 RGBA, 789,257 bytes |
| SHA256 | `1b52513d510af8769502a2772afcb5508e64816ef3b8aded23561f045639ad6b` |

No source-image editing/export toolchain is required: retain the selected PNG;
regeneration from the following prompts is stochastic, not byte-reproducible.
Runtime opacity/layout do not alter its pixels. This provenance is separate from
the supplied emblem/wordmark and pending MP4 rights.

<details>
<summary>Generation and refinement prompts</summary>

**Generation:**

Use case: stylized-concept. Asset type: reusable 2D game UI dark mist backdrop, square 1024x1024 PNG with a genuine transparent alpha background. Create ONLY a soft organic black/very dark neutral gray atmospheric cloud, approximately a broad vertical oval, denser and near-opaque around its large central region, gradually fading in opacity across a very broad irregular feathered perimeter to fully transparent. Subtle low-frequency wisps, smooth continuous gradients, restrained texture. This is a darkening matte behind a logo and title, to be composited over a vivid game scene at overall opacity 0.35, so the center should be dark charcoal almost black, not white/bright smoke. Keep every edge of the canvas fully transparent with at least 8 percent empty padding; all wisps must dissolve before the canvas edge. No text, logo, symbols, frame, rectangle, panel, vignette border, checkerboard, scenery, objects, particles, sparks, or lights. No external/reference images; original abstract texture.

**Refinement of that generated candidate:**

Edit this backdrop only. It must be a very soft darkening matte, NOT a visible luminous smoke ring. Remove the light gray outer ring entirely: keep RGB uniformly near-black/dark charcoal throughout the visible cloud, without bright highlights. Make the alpha falloff much broader and smoother, dissolving across the outer third of the silhouette into genuine fully transparent alpha. Preserve subtle organic low-frequency irregularity, not a clean geometric oval. The center may approach opaque black, but there must be no solid-looking perimeter or hard contour. Pull the entire cloud inward so at least 10 percent of the canvas on every side is fully transparent, including the bottom. No text, logos, scenery, particles, frame, checkerboard or added objects. Output a PNG with actual transparency; preserve the square canvas. This will be composited at 35 percent opacity behind menu branding.

</details>

No Main Menu Video upstream media is reused. Before adding approved exports,
complete this record, check file sizes against repository/hosting constraints,
and review packaging. No implicit Git LFS migration is authorized.

Architecture and code provenance: [Technical design](TECHNICAL_DESIGN.md).
All acceptance scenarios: [Test plan](TEST_PLAN.md).
