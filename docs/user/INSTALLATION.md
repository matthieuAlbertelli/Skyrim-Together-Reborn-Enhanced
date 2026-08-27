# Installing Skyrim Together Reborn Enhanced

> **Public alpha installation guide**
>
> This is the canonical end-user installation guide for STRE. Developer build instructions live in [`docs/development/BUILDING.md`](../development/BUILDING.md).

## Supported alpha setup

The current STRE alpha is validated primarily on:

- Windows x64;
- Steam `The Elder Scrolls V: Skyrim Special Edition`;
- Skyrim runtime `1.6.1170`;
- STRE `0.3.0-alpha.1`;
- Address Library for SKSE Plugins;
- SKSE64 matching Skyrim `1.6.1170`;
- Better Grabbing installed separately.

Other Skyrim runtimes or stores may work in the future, but they are not part of the current STRE support target.

STRE is a fork of Skyrim Together Reborn. **Do not install a separate Skyrim Together Reborn client on top of STRE.** The STRE release already contains the Skyrim Together client, server and game plugin files it needs.

For first-time testing, use a clean Skyrim mod profile with only the dependencies listed below.

## 1. Prepare Skyrim

1. Install Skyrim Special Edition from Steam.
2. Update it to runtime `1.6.1170`.
3. Launch Skyrim once normally from Steam and reach the main menu.
4. Close the game.

The current STRE compatibility matrix is maintained in [`docs/testing/COMPATIBILITY_MATRIX.md`](../testing/COMPATIBILITY_MATRIX.md).

## 2. Install Address Library

Install **Address Library for SKSE Plugins** from Nexus Mods.

Current Address Library releases provide an all-in-one package containing the runtime databases. The resulting `.bin` files must be available under:

```text
Skyrim Special Edition\Data\SKSE\Plugins\
```

STRE itself uses Address Library to resolve Skyrim runtime addresses.

Address Library:
https://www.nexusmods.com/skyrimspecialedition/mods/32444

## 3. Install SKSE64

STRE can start a matching installed SKSE runtime itself. Better Grabbing is an SKSE native plugin, so the STRE multiplayer setup requires SKSE64 to be installed even though upstream Skyrim Together can run without it.

For Skyrim `1.6.1170`, install the matching Anniversary Edition SKSE64 build.

Official SKSE site:
https://skse.silverlock.org/

Install SKSE normally into the Skyrim game directory so the matching `skse64_*.dll` is present next to `SkyrimSE.exe`.

For an STRE session, **launch `SkyrimTogether.exe`, not `skse64_loader.exe`**. STRE searches the Skyrim directory for the matching SKSE DLL and starts SKSE from inside the Skyrim Together process.

## 4. Install Better Grabbing

STRE does not redistribute Better Grabbing.

Install Better Grabbing separately through your mod manager or manually. For `0.3.0-alpha.1`, this guide targets Better Grabbing `1.17`.

Better Grabbing:
https://www.nexusmods.com/skyrimspecialedition/mods/134769

After installation, verify that this file exists:

```text
Skyrim Special Edition\Data\SKSE\Plugins\BetterGrabbing.dll
```

STRE multiplayer servers require `BetterGrabbing.dll` by default through the native-plugin policy.

## 5. Download STRE

Open the STRE GitHub Releases page:

https://github.com/matthieuAlbertelli/Skyrim-Together-Reborn-Enhanced/releases

Download the playable Windows archive named like:

```text
STRE-v<version>-windows-x64.zip
```

For this release:

```text
STRE-v0.3.0-alpha.1-windows-x64.zip
```

Do **not** download GitHub's automatically generated:

```text
Source code (zip)
Source code (tar.gz)
```

Those are source archives, not playable builds.

## 6. Install STRE

### Recommended: Vortex

1. Open Vortex.
2. Select Skyrim Special Edition.
3. Open **Mods**.
4. Choose **Install From File**.
5. Select `STRE-v0.3.0-alpha.1-windows-x64.zip`.
6. Enable the mod.
7. Deploy your mods.
8. Open the **Plugins** section and make sure these plugins are enabled:

```text
SkyrimTogether.esp
STRE_AlternateStart.esp
```

The STRE archive already uses a Skyrim `Data`-relative layout. Do not add another `Data` folder around it.

### Manual installation

Extract the **contents** of the STRE archive directly into:

```text
<Skyrim Special Edition>\Data\
```

After extraction, the important paths should include:

```text
Data\SkyrimTogether.esp
Data\STRE_AlternateStart.esp
Data\SkyrimTogetherReborn\SkyrimTogether.exe
Data\SkyrimTogetherReborn\SkyrimTogetherServer.exe
Data\scripts\
Data\meshes\
```

Do not extract the archive into:

```text
<Skyrim Special Edition>\
```

because the release archive is already laid out relative to `Data`.

## 7. Verify the installation

Before launching, check the following:

```text
<Skyrim>\SkyrimSE.exe
<Skyrim>\skse64_*.dll

<Skyrim>\Data\SkyrimTogether.esp
<Skyrim>\Data\STRE_AlternateStart.esp

<Skyrim>\Data\SkyrimTogetherReborn\SkyrimTogether.exe
<Skyrim>\Data\SkyrimTogetherReborn\SkyrimTogetherServer.exe

<Skyrim>\Data\SKSE\Plugins\BetterGrabbing.dll
<Skyrim>\Data\SKSE\Plugins\*.bin
```

If one of the STRE files is missing, reinstall the STRE release archive.

If `BetterGrabbing.dll` is missing, multiplayer connection will be rejected by a default STRE server.

## 8. Launch STRE

Launch:

```text
<Skyrim>\Data\SkyrimTogetherReborn\SkyrimTogether.exe
```

You can add this executable as a Vortex tool for convenience.

On the first launch, Skyrim Together may ask which game executable it should use. Select:

```text
SkyrimSE.exe
```

Do **not** select `skse64_loader.exe`.

Once in game, the Skyrim Together UI can be opened with:

```text
F2
```

or:

```text
Right Ctrl
```

## 9. Host a local server

The playable STRE archive includes the server.

Run:

```text
<Skyrim>\Data\SkyrimTogetherReborn\SkyrimTogetherServer.exe
```

On first launch, the server creates its configuration under:

```text
<Skyrim>\Data\SkyrimTogetherReborn\config\STServer.ini
```

The default server port is:

```text
10578
```

The STRE server also requires Better Grabbing by default:

```ini
[ModPolicy]
sRequiredNativePlugins = BetterGrabbing.dll
```

For a first same-PC test, start the server and connect to:

```text
127.0.0.1:10578
```

For another PC on the same LAN, connect using the host PC's local IPv4 address.

For internet hosting, firewall/router configuration is environment-specific. The upstream Skyrim Together server guide remains the reference for port forwarding and networking:

https://wiki.tiltedphoques.com/tilted-online/guides/server-guide/windows-setup/regular-setup

## 10. First multiplayer test

For the first validation, keep the setup intentionally small:

1. launch the STRE server;
2. launch STRE on both PCs;
3. load the intended STRE save/character flow;
4. connect both clients to the same server;
5. verify both players are visible;
6. drop and pick up a simple item;
7. grab and release a simple movable object;
8. verify normal interaction/dialogue still works.

Do not add a large mod list until this baseline works.

## Updating STRE

For an alpha update:

1. close Skyrim and `SkyrimTogetherServer.exe`;
2. back up `state\stre-server.sqlite3` and every roster member's Skyrim save
   profile before upgrading a campaign;
3. download the new `STRE-v<version>-windows-x64.zip`;
4. remove the previous STRE mod/version from your mod manager;
5. install the new archive;
6. deploy;
7. verify `SkyrimTogether.esp` and `STRE_AlternateStart.esp` are enabled;
8. update external dependencies if the new release notes require it.

Avoid mixing files from two STRE releases.

All clients and the server must use the exact same release build. Mixed
`0.2.0-alpha.1` / `0.3.0-alpha.1` sessions fail the exact-build handshake.
Campaign database downgrade is not supported: keep the matching executable and
database backup together if you need to return to an earlier installation.

`Alternate Start - Live Another Life` must not be active with
`STRE_AlternateStart.esp`. SkyUI is not in the validated campaign Journal/load
matrix for this alpha; if a save browser or Journal crash occurs, reproduce with
the vanilla Journal UI before reporting it as an STRE campaign-load failure.

## Uninstalling STRE

With Vortex, disable/remove the STRE mod and deploy again.

For a manual installation, remove only files that came from the STRE release. Do not delete shared SKSE, Address Library or Better Grabbing files unless you also want to uninstall those dependencies.

## Troubleshooting

### "Failed to load Skyrim Address Library"

Verify that the current Address Library database files exist under:

```text
Data\SKSE\Plugins\
```

and that they support your Skyrim runtime.

### Server says Better Grabbing is missing

Verify:

```text
Data\SKSE\Plugins\BetterGrabbing.dll
```

Then make sure matching SKSE64 files are installed in the Skyrim game directory and restart STRE.

### Better Grabbing is installed but does nothing

Confirm that you launched through:

```text
Data\SkyrimTogetherReborn\SkyrimTogether.exe
```

and that matching SKSE64 files are installed next to `SkyrimSE.exe`.

### Skyrim Together asks for the game executable

Select:

```text
SkyrimSE.exe
```

If the wrong executable was previously selected, use the normal Skyrim Together reset procedure and select `SkyrimSE.exe` again.

### Client logs

STRE/Skyrim Together client logs are written under the Skyrim Together runtime log directory as `tp_client.log`.

When reporting a multiplayer bug, include:

- STRE version;
- Skyrim runtime version;
- Better Grabbing version;
- number of connected players;
- exact reproduction steps;
- client logs from the affected machines;
- server log if relevant;
- crash dump if one was produced.

## Development builds

Players should use GitHub Releases.

If you want to compile STRE from source instead, see:

[`docs/development/BUILDING.md`](../development/BUILDING.md)
