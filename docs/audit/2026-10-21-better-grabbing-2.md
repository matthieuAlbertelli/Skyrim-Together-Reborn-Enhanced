# Better Grabbing 2.0 / Steam 1.7.104 qualification evidence

> **Historical evidence:** main-PC preflight and subsequent human Solo smoke on
> October 21, 2026. [STATUS](../project/STATUS.md) owns current
> verdicts. Refs #99; branch `feat/skyrim-1.7-runtime`.

## Scope and outcome at capture

The maintainer confirmed installing/deploying official Better Grabbing 2.0 and
keeping 1.17 disabled. Codex performed a read-only deployment audit, archived
evidence in ignored `_audit/rt17-better-grabbing2/preflight-20261021T103307Z/`,
and prepared the human Solo smoke. Codex did not launch or control any game,
Vortex, Steam or server. The agent changed no deployment/staging, existing save,
STRE code or server policy. No Pc-vero access, multiplayer, push, PR, manual CI,
merge or release occurred.

Independent verdicts at preflight capture: **DLL/provenance PASS; SKSE loading NOT TESTED;
Solo manipulation NOT TESTED.** Admission and World Sync multiplayer are untested.
The compiled candidate remains source `efdeab46623fbd25fafa5c8a7fa7e0cb4ebebda7`,
identity `runtime17-efdeab46623fbd25fafa5c8a7fa7e0cb4ebebda7`; documentation HEAD
before this audit was `27ae42e5abf09b711d0d1149d1349656f762605a`.

## Provenance and exact material

The [official Nexus files](https://www.nexusmods.com/skyrimspecialedition/mods/134769?tab=files)
identify version 2.0, uploaded September 16, 2026 at 18:20 by SkyrimThiago.
The [author's description](https://www.nexusmods.com/skyrimspecialedition/mods/134769?tab=description)
announces Skyrim 1.7 support and requires Address Library. SkyPrompt is optional.

Current installer/network logs establish this chain (UTC):

1. 10:23:43.963: update metadata selects version 2.0 / Nexus file **806222**.
2. 10:23:47.154: generated Nexus protocol request targets mod **134769**, file
   **806222**; successful `getDownloadURLs` is recorded. Query credentials are
   omitted from retained excerpts.
3. Download identity `3c852825-0159-41bc-bfcb-e56f2b35c3a8` completes; local archive
   MD5 `e0c0cb20bcbcb0b73417ff89607094cd` agrees with the installer hash event and
   successful Nexus hash lookup. SHA-256 below is independently calculated.
4. 10:23:51: installer removes the former deployed mod files, extracts the 2.0
   archive into the reused `Better Grabbing-134769-1-17-1735821127` directory,
   and reports successful installation. The current deployment manifest names
   that same source. The directory label does not describe its current bytes.
5. The archive's DLL, staging DLL and deployed `Data/SKSE/Plugins/BetterGrabbing.dll`
   have identical SHA-256. Staging/deployment are the same NTFS file, two hardlinks.

| Material | Bytes | SHA-256 |
|---|---:|---|
| Installed 2.0 archive | 9,385,742 | `ecea384c32f852fb0e49bb9f773988a84b030b904ab4ac9a5a4933fb9b637f34` |
| 2.0 DLL: archive = staging = deployed | 3,534,848 | `55c974ec69e2389b4432bb25a0dd6690d2e453ea9f16bc4e2adf683449afee7a` |
| Retained 1.17 archive | 338,823 | `4046ab69e65aaeafea95df771ad46ee6415911ec8c6bd105e8ded96fc0ffc9bc` |
| DLL inside 1.17 archive, inactive | 960,512 | `8fdd71dd612253ae1b9d3f16bbcc3f1b01639154325cee2cc42ed03e2b949695` |

The archive is named `Better Grabbing 134769 2.0 2026-09-16T18-20Z s6Og0dpie.7z`.
The DLL was inspected as data using Windows tar and pefile, never loaded as code.
Recursive inspection finds one BetterGrabbing DLL under the game and one under
Vortex staging; the only active SKSE plugin DLL is this 2.0 binary. The old archive
is outside the plugin scan path. No active/prioritized 1.17 DLL was found.

Evidence: `archives.json`, `dlls.json`, `deployment.json`, `vortex-provenance.json`
and `provenance-chain.json`. SHA-256 provenance is a local archive-to-install
comparison plus the Nexus/installer chain, not an independently signed publisher
checksum or a reproducible-build attestation. The public linked source tree's
version resource still describes 0.1.12; it was not treated as the 2.0 source.
Historical LevelDB SST entries were inspected but excluded from current-state
conclusions: their newer WAL was locked. No database was opened for writing or
application controlled. Installer/network logs independently establish the new
installation; historical SST version/enabled fields do not override actual bytes.

## Static SKSE and runtime declarations

The deployed PE is AMD64 (`0x8664`), file version **2.0.0.0**, product **2.0.0**.
It exports `SKSEPlugin_Load`, `SKSEPlugin_Query` and `SKSEPlugin_Version`.
The static version record declares:

- `dataVersion=1`, `pluginVersion=0x02000000`, name `BetterGrabbing`.
- `versionIndependenceEx=0x3`: NoStructUse and **AddressLibraryV5** bits set.
- `versionIndependence=0x1`: post-AE Address Library bit set.
- Minimum SKSE field zero. The compatibility array contains sixteen `0x01000000`
  values; it is not an explicit enumeration of 1.7.104. The Address Library
  independence declarations supply the compatibility route.

Bit meanings were checked against the
[official SKSE header](https://github.com/ianpatt/skse64/blob/master/skse64/PluginAPI.h).
The 1.17 archive's DLL instead has extended flags `0x1`, without the V5 bit,
and an old internal file version 0.1.12.0. The PE author field is the placeholder
`AUTHOR_NAME`; author provenance comes from Nexus/install evidence above.
These flags are compatibility declarations, not proof of ABI correctness or
successful SKSE initialization. No incompatible declaration was identified.

Installed runtime inputs retain the accepted exact hashes:

| Input | SHA-256 |
|---|---|
| Steam SkyrimSE.exe 1.7.104.0 | `846efccf0c1374d71f892907f46549560f2fcb0a75cb87a3eed438baa0f1402f` |
| Official SKSE 2.3.1 runtime DLL | `d5158f8c61a2257138692940ed27048c40d6ff62676e64791bdc83d4549fb94e` |
| versionlib-1-7-104-0.bin, format 5 | `8aab3dd251d135b849bd983f86a4a205c920fa3e81f8e30c0e63ccfef9423842` |

All **293 deployed STRE files** and **914 accepted installed content files** match
the lot-4 installed baselines, including previously accepted differences against
repository sources. `preservation.json` retains each comparison. Existing server
configuration SHA-256 is `0935301d85cfd58a0338d54225bd290868f3f2d497fff43ad775fa45e804e14b`:
`sRequiredNativePlugins=BetterGrabbing.dll`, `bAllowSKSE=true`,
`bEnableModCheck=false`. Native-plugin admission remains independent of ordinary
ESP/ESM mod checking; no server was started and no admission was attempted.

## Additional Solo attestation and corroboration

The maintainer explicitly confirmed that **Continue loaded the intended Solo
save** and that **appearance, equipment, spells, creation state and dialogues**
were retained. These are human PASS observations for that character, not an
exhaustive class/race/sex/spell/dialogue compatibility matrix.

Eight logs were captured with stable hashes before the Better Grabbing smoke.
The new client file preserves all 5,213,539 bytes of the earlier lot-5 capture and
adds 20,564 bytes, including these events on October 21 (Europe/Paris):

- 12:20:19.175: ordinary main menu, no campaign/admission.
- 12:20:25.347: `MainMenu.Continue` enters `ContinueLastSavedGame`.
- 12:20:25.513: target resolves to the previously validated quick-save;
  Ordinary, `admission=false`, `gateLocked=false`, `AllowVanilla`.
- 12:20:28.069: native loading completes. SKSE reads the same co-save identity.

`continue-client-delta.log` and `continue-evidence.json` retain the exact private
save identity and relevant events. Logs corroborate route, target and completed
load; they do not compare individual restored character properties. No new exit
code is inferred. This Continue run precedes the 12:23 installation of 2.0.
The preflight SKSE log contains no BetterGrabbing loading event. The existing
BetterGrabbing log is stale (October 19, internal 0.1.12.0) and proves nothing
about the newly deployed DLL.

## Human smoke and runtime evidence

The maintainer was asked to launch SkyrimTogether.exe on the main PC, verify the
menu, load the validated Solo save, hold E on an ordinary movable object, test
translation/rotation/release, confirm subsequent character/camera/interaction
controls, and quit normally. Installed controls map right mouse to translation
and left mouse to rotation. No server or multiplayer campaign belongs to this test.

The maintainer subsequently reported every requested step working, followed by
normal exit. Postflight evidence was captured at 12:42:23-12:42:26 Europe/Paris in
ignored `_audit/rt17-better-grabbing2/postflight-20261021T104223Z/`.

| Independent verdict | Evidence |
|---|---|
| DLL/provenance PASS | Preflight chain above; DLL hash unchanged after the run |
| SKSE loading PASS | Fresh SKSE 2.3.1 log: BetterGrabbing.dll, version 02000000, loaded correctly, handle 1; initialization completes |
| Solo manipulation PASS | Maintainer confirms grab, translation, rotation, release and subsequent controls; no per-transform trace is claimed |

The new BetterGrabbing log identifies version **2-0-0-0** at 12:40:19.651,
loads five translations and reports plugin initialization at 12:40:19.653.
Its optional Grab and Throw absence is informational. The fresh SKSE and plugin
logs contain no runtime incompatibility or Address Library error. No statement
relies on the old October 19 plugin log.

The client rotates during this run. The entire 5,234,103-byte preflight client
prefix is retained in `tp_client.1.log`; its suffix plus the new `tp_client.log`
yield 20,563 new bytes / 185 lines. The two next rotations match their prior
contents. The old oldest rotation remains in the preflight archive. Correlated
events (Europe/Paris):

- 12:40:14.219: start of the newly captured segment.
- 12:40:18.022: SKSE startup hooks installed; one such sequence in the segment.
- 12:40:25.354: main menu, `wasAdmitted=false`, `campaign=none`.
- 12:40:31.581: human chooses Continue; at 12:40:31.786 the same validated
  quick-save is classified Ordinary/AllowVanilla, outside campaign admission.
- 12:40:35.171: native load completes; SKSE reads the same co-save.
- 12:41:19.204: final captured client event, Journal opened. This is not an
  exit timestamp. Normal Quit is human-attested, without an observed exit code.

The client contains the existing Discord error 4 and animation-signature
warnings; optional intro/custom menu remains disabled by its historical guard.
CEF starts a fresh log, not an append: nine error diagnostics comprise six
performance-counter messages, two GCM registration responses and one Chromium
registry message; the OAuth configuration warning also remains. These are
retained diagnostics, not plugin-loading failures or a new gameplay FAIL.
Eight logs have stable captured hashes. `runtime-analysis.json`, `delta.json`
and the complete logs retain the evidence and its bounds.

Postflight rechecks all 293 STRE files, 914 accepted contents and three runtime
inputs: every hash still matches. DLL, installed Better Grabbing configuration
and server policy hashes are also unchanged. No rebuild/test-binary execution
occurred. Validation consists of static/hash checks, the human run and its logs.
Both original downloaded archives are retained privately under postflight
`retained-archives/`, with verified SHA-256 in `retained-archives.json`; no plugin
binary or third-party archive is added to the repository. Documentation diff
checking and 72 local link-path checks pass; no gameplay unit suite was rerun.

## Prepared second-PC prerequisites; not executed

The
[World Sync contract](../features/world-sync/BETTER_GRABBING_INTEGRATION.md) and
[ADR-0018](../architecture/ADRs/ADR-0018-fixed-roster-coordinated-checkpoint-recovery.md)
are unchanged. This Solo PASS does not establish handshake admission, observer
representation, release/settlement, interruption safety or collective recovery.

Preparation for a later, separately authorized second-PC deployment requires:

1. Pin the official external archive/DLL and installed configuration, retain 1.17
   disabled, and verify Skyrim/SKSE/AL and actual deployment hashes on each PC.
2. Match A/B client and server compiled identity exactly; verify native/UI/content
   manifests and load order, without overwriting accepted content or existing saves.
3. Qualify actual SKSE loading and local manipulation on B after its human install;
   preserve the server's required-plugin policy and test real admission separately.
4. Only after environment consistency and admission, execute the canonical
   [roster-2 protocol](../features/alternate-start/TEST_PLAN.md#steam-17104-manual-qualification-sequence)
   with fresh isolated campaigns and both completion orders. The server remains
   persistent authority; the roster is sealed/immutable/full; recovery restores
   one committed checkpoint with each member's own native save.

**Stop before any second-PC action or multiplayer launch.** Nothing in this
preflight authorizes those operations or clears their acceptance gates.
