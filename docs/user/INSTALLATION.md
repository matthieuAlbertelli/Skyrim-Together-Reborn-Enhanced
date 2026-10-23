# Installing STRE 0.5.0-alpha.1 — unpublished candidate

Target: **Skyrim Special Edition Steam 1.7.104.0, Windows x64 only**.
The port is integrated into main; this Alpha is being prepared locally and is
**not yet a public download**. All features remain experimental. A development
build is not evidence that a complete campaign or recovery is qualified.

The historical [0.4.0-alpha.1 release](https://github.com/matthieuAlbertelli/Skyrim-Together-Reborn-Enhanced/releases/tag/stre-v0.4.0-alpha.1)
requires Skyrim 1.6.1170. **Do not install that ZIP on Skyrim 1.7.104.0.**
The following procedure is for a separately identified 0.5 package once supplied
for authorized testing, or published after its release gates pass. It does not
ask you to downgrade an existing installation.

## Obtain and verify the package

The planned publication is `stre-v0.5.0-alpha.1`; its expected asset is
`STRE-v0.5.0-alpha.1-windows-x64.zip`, accompanied by SHA-256 checksums.
That asset does not exist as a public release in this preparation. Local files
marked PREPACKAGE are tests of their recorded source SHA, not the future tag.
Do not substitute GitHub source ZIPs, an older release, installed Data, or a
mod-manager staging folder for the player artifact. Compare the downloaded ZIP
with its supplied SHA-256 before extracting it.

## Install the prerequisites separately

Use legitimate, complete Steam game files and the official distributions of:

| Component | Exact requirement |
|---|---|
| Skyrim | Steam `SkyrimSE.exe` **1.7.104.0**, Windows x64; other 1.7 builds/stores are outside the target |
| SKSE64 | **2.3.1**, including `skse64_1_7_104.dll` beside SkyrimSE.exe and its scripts; its Windows file resource is `0.2.3.1` |
| Address Library | Runtime database `Data/SKSE/Plugins/versionlib-1-7-104-0.bin` (v13 package / V5 data); a similarly named database for another runtime is insufficient |
| Better Grabbing | Official **2.0**, with `Data/SKSE/Plugins/BetterGrabbing.dll`; 1.17 is not the 1.7 baseline |
| EEKs Fireplace Resource | Separately installed resource from EEKs Resource Repository, including textures referenced by the Vanilla Textured fireplace; no direct Embers HD dependency |

Dependency sources: [SKSE](https://skse.silverlock.org/),
[Address Library](https://www.nexusmods.com/skyrimspecialedition/mods/32444),
[EEKs Resource Repository](https://www.nexusmods.com/skyrimspecialedition/mods/31562?tab=files).
Use the official SkyrimThiago Better Grabbing 2.0 distribution identified by
the release's dependency checks; a renamed old DLL does not satisfy the policy.
Keep compatible dependency versions identical across players. Do not layer a
separate upstream STR client over STRE or enable incompatible legacy native
plugins. Better Grabbing's successful Solo test does not establish multiplayer
object manipulation/interruption correctness. Never remove its server admission
requirement to bypass a missing plugin.

## Install the STRE files

1. Back up your mod setup, local saves and server campaign state before changing
   versions. Use a clean test profile and a separate test campaign/save set.
   Do not overwrite an existing campaign to test a package.
2. Extract the ZIP to inspect it. Its root is the content for **Skyrim/Data**:
   `SkyrimTogetherReborn/`, `STRE/`, `Scripts/`, `Meshes/`, `Textures/`, `Sound/`,
   `Seq/` and the two ESPs, plus VERSION, INSTALLATION and licence notices.
   Install that root as one mod in your manager, or copy that root into Data
   manually. Do not add an extra nested Data directory. Manager deployment is
   an explicit human step; verify the files in the actual game installation.
3. Enable `SkyrimTogether.esp` and `STRE_AlternateStart.esp`, loading required
   Bethesda masters first. Follow their declared master dependencies; place
   STRE_AlternateStart after SkyrimTogether. Do not enable Live Another Life
   (`Alternate Start - Live Another Life.esp`) with this alternate start.
   Keep plugins/order identical across players. No ESL conversion is required.
4. Preserve the package's PEX and corresponding PSC content. Do not substitute
   locally authored scripts, voices or older plugins. The silent/menu branding
   resources are included; the private `intro.mp4` and private Valen exports are
   not. Absence of the intro is expected, not an installation failure.

## Launch and connect

Launch **`Data/SkyrimTogetherReborn/SkyrimTogether.exe`** and select the installed
SkyrimSE.exe if requested. It loads the matching SKSE DLL; skse64_loader.exe is
not the STRE launch entry. Check the menu and dependencies before joining.

For hosting, use **SkyrimTogetherServer.exe from the same candidate** with its
working directory set to SkyrimTogetherReborn. Its ordinary no-argument run
creates/reads `config/STServer.ini` relative to that directory. Stop it before
editing the configuration; set the intended session/network/access settings,
then restart it manually. Preserve `ModPolicy:sRequiredNativePlugins` including
BetterGrabbing.dll. Configuration and campaign state are local server data,
never included credentials from a player archive. The default server port is
**10578**; expose only the intended endpoint using your normal firewall/router
configuration. Give players the host address and port, then connect through
the STRE overlay/lobby. Server discovery is not proof of build compatibility.

Every client and server must use the **same exact compiled candidate**, not
merely the same `VERSION` text. Mismatched build negotiation fails closed.
Create a fresh test lobby with the complete intended roster before starting.
Campaign v1 seals that roster; replacement and late join after sealing are
forbidden, and a disconnect enters recovery lock. Do not work around those gates.

## Saves, recovery and support limits

No 1.6-to-1.7 save/campaign migration, mixed 0.4/0.5 session or downgrade of new
save/campaign state is qualified. Preserve the prior version's whole matched
backup set: server campaign database and companion state, every roster member's
own `.ess` and `.skse`, plugins and dependency/version inventory. Never pair an
old server database with new native saves. Returning to an old version requires
its matching old environment and backups, not loading 0.5 state into 0.4; that
rollback is not requested or qualified here.

Earlier development save/load PASS does **not** establish disconnect, crash or
collective checkpoint recovery. World Sync multiplayer manipulation, Recovery,
Valen DialogueProgress and wider gameplay matrices have open limits. Check the
[current status](https://github.com/matthieuAlbertelli/Skyrim-Together-Reborn-Enhanced/blob/main/docs/project/STATUS.md),
[compatibility matrix](https://github.com/matthieuAlbertelli/Skyrim-Together-Reborn-Enhanced/blob/main/docs/testing/COMPATIBILITY_MATRIX.md)
and [release notes](https://github.com/matthieuAlbertelli/Skyrim-Together-Reborn-Enhanced/blob/main/docs/production/releases/0.5.0-alpha.1.md).
Those online links refer to current main; a local unpublished candidate can
precede the linked documentation. Keep its included VERSION and source identity.

If startup/admission fails, stop and retain package/source SHA, dependency
versions, the relevant client/SKSE/server diagnostics and observations. Do not
share passwords, private addresses, complete saves or personal configs publicly.
See [support](https://github.com/matthieuAlbertelli/Skyrim-Together-Reborn-Enhanced/blob/main/SUPPORT.md). Do not treat a running process as a
successful installation or repeat a failure without first identifying its cause.
