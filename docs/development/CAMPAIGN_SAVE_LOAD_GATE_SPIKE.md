# Campaign save-load gate technical spike

> **Status:** compiled instrumentation awaiting in-game validation.
> **Issue:** [#60](https://github.com/matthieuAlbertelli/Skyrim-Together-Reborn-Enhanced/issues/60)

## Question

Can STRE finish loading a deliberately marked Skyrim save, establish a real
native pause, and keep the local player behind that barrier before the first
playable post-load tick, while CEF and STRE networking remain alive?

This spike measures engine timing only. It does not identify real campaign
saves and does not define checkpoint IDs, fingerprints, `.stre` files,
`RecoveryLock`, roster state, or any new local authority.

## Audited boundaries

STRE is not loaded as a conventional SKSE plugin. `ScriptExtender.cpp` locates
the matching `skse64_*.dll`, calls its exported `StartSKSE`, and retains only
the module handle. STRE does not export `SKSEPlugin_Load`, receive an
`SKSEInterface`, or obtain an `SKSEMessagingInterface`. The SKSE
`PreLoadGame`/`PostLoadGame` notification API therefore is not directly
available to this client, and this spike does not invent access to it or add a
bridge DLL.

The spike uses two native boundaries already compatible with STR's integration
model:

1. `BGSSaveLoadManager::Load_Impl`, Address Library AE ID `35728`, is wrapped
   using the existing VersionDb/`TP_HOOK` infrastructure. Its entry and return
   produce `NativeLoadEnter` and `NativeLoadReturn`. This is the public native
   save-load-manager call boundary used by CommonLibSSE-NG; the spike does not
   claim visibility into every internal form, Papyrus, or streaming operation.
2. Skyrim's existing `TESLoadGameEvent`, already consumed by
   `DiscoveryService`, is the first confirmed engine event after the loaded
   runtime has been reconstructed. It produces `PostLoad` and requests the
   guard menu synchronously from that event callback.

The exact relationship between the native call return and `TESLoadGameEvent`
is deliberately logged. No deeper load-pipeline hook is added before the
in-game ordering proves one necessary.

Reference implementations used for the audit:

- [SKSE save/load hook and notifications](https://github.com/ianpatt/skse64/blob/4c1b425415c15f4655c73abb4682023baeb99d48/skse64/Hooks_SaveLoad.cpp)
- [CommonLibSSE-NG save/load manager](https://github.com/CharmedBaryon/CommonLibSSE-NG/blob/main/src/RE/B/BGSSaveLoadManager.cpp)
- [CommonLibSSE-NG Address Library IDs](https://github.com/CharmedBaryon/CommonLibSSE-NG/blob/main/include/RE/Offsets.h)

## Local gate and native pause

`CampaignRuntimeGate` is a local barrier with four states:

```text
Open -> ArmedDuringLoad -> LockedAfterLoad -> Released
```

Only explicit release can leave `LockedAfterLoad`. Guard-menu destruction and
CEF presentation changes are observations and cannot release the gate.

`STRECampaignGateMenu` is an empty native `IMenu` with exactly the safety flags
needed by this spike:

- `kPausesGame`;
- `kModal`;
- `kDisablePauseMenu`;
- no `kAllowSaving`.

The connected-menu hook in `UI.cpp` explicitly recognizes this menu and returns
before STR's `UnfreezeMenu` allow-list processing. Its pause flag is therefore
not removed when connected.

While locked, the service reuses existing input primitives:

- `DInputHook` remains enabled, preventing Skyrim gameplay input while the
  existing raw-window path stays available to CEF;
- `MenuControls::SetToggle(false)` prevents gameplay menu opening;
- the modal pausing menu blocks movement, interaction, combat, console, and
  simulation, and does not permit saving;
- `F2`/right Control can still open the existing STRE CEF surface.

The previous `MenuControls` value is restored and `UiSurfaceService` reapplies
its normal input-capture state only after explicit development release.

## Development-only controls

The controls are compiled only when `IS_MASTER` is false:

- `F8`: mark exactly the next native load as `CampaignManaged`;
- `F10`: explicitly release the gate after the experiment.

Both controls use the existing `WM_INPUT`/`ProcessKeyboard` raw-input path that
keeps `F2` available to CEF while Skyrim is paused. Their raw key events are
consumed, and their actions run on key-up without mixing raw-input events with
`GetKeyState` modifier state. The old `WM_KEYUP` development path is not used
because it did not deliver the release key while the native guard menu held the
game paused.

No save-name convention or persistent marker is introduced.

## Networking hypothesis to test

`TiltedOnlineApp::Update` calls the gate probe and then `World::Update`.
`World::Update` drains `RunnerService`, triggers `UpdateEvent`, and thereby
calls `TransportService::HandleUpdate`. The gate logs
`TransportUpdateWhileLocked` from that real transport callback.

The unresolved engine question is whether the hooked Skyrim VM update continues
to call `TiltedOnlineApp::Update` while `UI::GameIsPaused()` is true. Code alone
does not prove that condition. The 60-second in-game run must show continuing
transport heartbeat logs and live CEF/network behavior; absence of those logs
falsifies the hypothesis.

## Expected log vocabulary

Every gate record contains a monotonically increasing microsecond timestamp,
the last STRE frame number, and the synchronized transport tick:

```text
[STRE][CampaignGate] DevArmRequested
[STRE][CampaignGate] PreLoad
[STRE][CampaignGate] GateArmed
[STRE][CampaignGate] NativeLoadEnter
[STRE][CampaignGate] NativeLoadReturn
[STRE][CampaignGate] PostLoad
[STRE][CampaignGate] PauseMenuRequested
[STRE][CampaignGate] PauseMenuPostDisplay
[STRE][CampaignGate] GameIsPaused value=true
[STRE][CampaignGate] FirstWorldUpdateAfterLoad GameIsPaused=true
[STRE][CampaignGate] TransportUpdateWhileLocked
```

The ordering, rather than the mere presence of the records, decides the spike.
If `FirstWorldUpdateAfterLoad GameIsPaused=false` precedes menu activation, the
normal menu path is too late and a narrower engine gate must be investigated in
a follow-up. If the menu is active and `GameIsPaused=true` before that first
update, no deeper hook is justified.

The first smoke run observed a small number of STRE updates before
`GameIsPaused=true`. That observation does not by itself demonstrate a playable
Skyrim simulation tick, so this control-only follow-up deliberately adds no
engine hook. The final smoke test must judge the player, AI, game time, Havok,
and active-effect clocks directly while preserving the captured ordering.

## Manual in-game procedure

1. Build and deploy the non-master `SkyrimTogetherClient` from this branch.
2. Start Skyrim with a save located in active gameplay: nearby moving NPCs,
   running game time, a visible duration effect, and interactable physics.
3. Open `tp_client.log` for live observation and press and release `F8` once.
   Confirm
   `DevArmRequested nextLoad=CampaignManaged`.
4. Load the chosen ordinary save through Skyrim's normal load UI. The marker is
   consumed by this one load only.
5. Do not connect for 60 seconds. Verify that the player, NPC/AI, game time,
   Havok, and effect duration remain frozen; movement, activation, combat,
   console, save, load, and gameplay menus must be unavailable.
6. During the same minute, press `F2` or right Control and verify that the CEF
   surface remains live. Attempt a server connection and verify live messages
   and recurring `TransportUpdateWhileLocked connected=true` records.
7. Press and release `F10` only after collecting the evidence. Confirm a
   `Released` record and normal playable input before ending the smoke test.
8. Extract the trace with:

   ```powershell
   Select-String -Path tp_client.log -Pattern '\[STRE\]\[CampaignGate\]|Load|Pause|FirstWorldUpdate'
   ```

9. Preserve the complete ordered extract, game/runtime version, branch SHA,
   whether CEF connected, and the observed frozen/not-frozen result for each
   item. Do not mark the spike proven from compilation or code inspection alone.
