# Campaign save-load gate technical spike

> **Status:** human-validated on 14 August 2026; production campaign wiring is
> outside this spike.
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

## Validation-only controls

The human test used temporary non-production controls: `F8` marked exactly the
next native load as `CampaignManaged`, and `F10` explicitly released the gate.
They ran through the existing `WM_INPUT`/`ProcessKeyboard` raw-input path, with
`F2` as the known-working CEF control sample. A diagnostic build recorded the
virtual key, event type, scan code, E0/E1 flags, development-control dispatch,
and service availability.

Those keyboard controls and raw-input probes were removed after successful
validation. The final spike branch does not reserve F8 or F10 and contains no
production campaign trigger. Production arming, validation, and release must be
driven by the future authoritative campaign/save/recovery flows; this spike does
not implement #28, #55, or #56.

No save-name convention or persistent marker is introduced.

## Networking validation

`TiltedOnlineApp::Update` calls the gate probe and then `World::Update`.
`World::Update` drains `RunnerService`, triggers `UpdateEvent`, and thereby
calls `TransportService::HandleUpdate`. The gate logs
`TransportUpdateWhileLocked` from that real transport callback.

The in-game run confirmed that CEF remained available and STRE networking stayed
alive while `UI::GameIsPaused()` was true. The existing update/transport path is
therefore sufficient for this validated scenario; the spike adds no alternative
network loop.

## Validation log vocabulary

The temporary raw-input probe recorded its direct keyboard fields. Every
`CampaignGate` record contains a monotonically increasing microsecond timestamp,
the last STRE frame number, and the synchronized transport tick:

```text
[STRE][RawInputProbe] virtualKey=113 type=... scanCode=... E0=... E1=...
[STRE][RawInputProbe] virtualKey=119 type=... scanCode=... E0=... E1=...
[STRE][RawInputProbe] virtualKey=121 type=... scanCode=... E0=... E1=...
[STRE][RawInputProbe] CampaignGateDevControlObserved key=... type=...
[STRE][RawInputProbe] CampaignGateDevServiceAvailable value=true
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

The trace showed a small number of STRE updates before `GameIsPaused=true`.
Direct observation nevertheless found no free gameplay progression: Skyrim
gameplay and vanilla menus were fully frozen. This satisfies the intended
post-load safety invariant and does not justify a deeper or more invasive engine
hook. The spike does not claim that no engine-internal work occurs while a save
is reconstructed or before the pause menu reports active.

## Human validation — passed

On 14 August 2026, a deployed `SkyrimTogether.exe` was exercised in-game with a
live save and the validation-only controls. The run confirmed:

- `F2` opened the STRE/CEF path;
- the next managed load could be armed and loading the save activated
  `CampaignRuntimeGate`;
- Skyrim gameplay and vanilla menus were fully frozen while locked;
- CEF remained available and STRE networking remained alive;
- explicit gate release restored normal Skyrim gameplay.

The temporary F8 arm control became reliable after the F2/CEF path had been
activated. That behavior is not a production dependency and warrants no further
spike scope unless it later affects automatic campaign arming or release.

Conclusion: the existing native load boundary, post-load event, guard menu, and
STRE update path are sufficient for the tested invariant. No additional engine
hook is required by this evidence. The campaign runtime/admission foundation
from #28 does not yet wire this gate into production; coordinated save selection
and checkpoint creation remain #55, while recovery lock and collective restore
remain #56.
