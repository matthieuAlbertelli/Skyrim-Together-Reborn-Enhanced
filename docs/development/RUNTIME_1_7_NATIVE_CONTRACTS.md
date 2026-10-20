# Native admission contracts for Steam 1.7.104

This document owns the finite native proof obligations used by
[`NativeContracts.h`](../../Code/common/Runtime/NativeContracts.h).
[STATUS](../project/STATUS.md) owns implemented/validated results;
[#99](https://github.com/matthieuAlbertelli/Skyrim-Together-Reborn-Enhanced/issues/99)
owns operational follow-up. The [lot-2 audit](../audit/2026-10-20-runtime-1.7.104-lot2.md)
records the examined files, tests and provenance. Nothing here changes the stable
1.6.1170 support claim or [ADR-0018](../architecture/ADRs/ADR-0018-fixed-roster-coordinated-checkpoint-recovery.md).

## Meaning of evidence

**Established/source** means the cited implementation for the exact target
supports the particular signature/access, not that the engine was tested.
**Adapted/source** additionally means STRE's narrow runtime guard admits that
access. **Unknown** means a named obligation remains, not confirmed
incompatibility. An Address Library entry alone establishes none of a function's
calling convention, object layout, instruction span or semantic ordering.

The source-only obligations below can use exact-target upstream as evidence
without re-reversing every inherited call. Obligations marked binary require the
actual Steam 1.7.104 EXE or SKSE DLL. Merely relabelling them source-qualified
cannot satisfy the compiled policy. Every mandatory obligation is required;
optional and diagnostic omissions cannot unlock or block unrelated campaign
features. The gate authorizes at most preparation of a later smoke, never a
campaign compatibility declaration.

Source anchors used below:

- **U885**: [upstream port #885](https://github.com/tiltedphoques/TiltedEvolution/pull/885),
  merge `6f9634979310cf1887a1dbf8f74ef13b71d0387a` (initially 1.7.99).
- **U104**: [exact target check](https://github.com/tiltedphoques/TiltedEvolution/commit/24dc4c4e8c4744b36ff86f1f129a1c8e3d716356).
- **U183**: [v1.8.3 snapshot](https://github.com/tiltedphoques/TiltedEvolution/tree/4917189f6ac382e874f9ee735c4fef6a350e449e).
  Local comparisons are against this exact commit; no merge/cherry-pick.
- **S231**: [official SKSE 2.3.1](https://github.com/ianpatt/skse64/tree/7ff865f4a27d6dc936ab5fd0533ff2d706c8f857).
  `skse64_common/skse_version.h` selects runtime 1.7.104.
- **CL131**: [CommonLibSSE-NG v13.1.0](https://github.com/alandtse/CommonLibSSE-NG/tree/12d61f3c39f7d4ba1b1ab543df2bca0e613dc747).
  `include/SKSE/Version.h` explicitly sets latest flat AE to 1.7.104.
  VR layouts/overloads are not evidence for the flat Steam runtime.
- **AL104**: v5 `versionlib-1-7-104-0.bin`,
  SHA-256 `8aab3dd251d135b849bd983f86a4a205c920fa3e81f8e30c0e63ccfef9423842`.
  All RVAs below are image-relative, not loaded absolute addresses.

## Mandatory bootstrap

Consumer paths in these tables are relative to `Code/`. “Init” means
`Initializer::RunAll`, after the source gate, before hook commit/game entry.
Offsets, sizes and RVAs are hexadecimal unless an explicit decimal unit or
Address Library ID is given.

| Key / origin | Consumer and exact contract | Resolution / use | Evidence and outstanding obligation |
|---|---|---|---|
| B01-loader — modified upstream | `immersive_launcher/loader/{ExeLoader,SteamCeg}.cpp`, `Launcher.cpp`, `client/main.cpp`, `ScriptExtender.cpp`: SteamStub unencrypted-section handling; mapping sentinel before CRT IAT hooks; STRE hook commit before `StartSKSE`, before WinMain | EXE preflight; then mapping, startup hooks and engine entry | **Established/source**, U885/U104/U183/S231; lot-1 bounded PE/AL preflight tests. B02/B03 independently require actual inputs. Loader order does not prove a particular EXE/DLL. |
| B02-target-image — added STRE qualification | `Runtime/Executable.cpp`, `VersionDb.h`, loader and native patch sites: exact x64 PE, version 1.7.104.0, Steam import/`.bind` entry, image extent and patched instruction spans | Before mapping; game bytes consumed by mapping sentinel and Init | **Unknown/binary**: missing legitimate `SkyrimSE.exe` 1.7.104. Check imports/sections/entry, every required AL address against its section, and spans/call sites listed in the initializer annex (including UI `82082+682`, `36548+FE`, `51538+15`). Inherited source is useful; presence of a relocation does not validate patched bytes. |
| B03-skse-binary — modified upstream | `client/ScriptExtender.cpp`: `skse64_1_7_104.dll`, resource `0.2.3.1`, x64 module and `StartSKSE` export/startup ABI | Read-only preflight; export invoked after STRE hook commit | **Unknown/binary**: official S231 source and filename/resource contract established, actual DLL unavailable. Inspect PE/resource/exports and recorded package provenance before admission; DLL presence never certifies plugin initialization. |
| B04-rtti-registration — inherited | `client/Games/Skyrim/RTTI.{h,cpp}`, `VersionDb.h`: lazy `RttiLocator`, cast function ID 109689 and the three absent registrations | C++ globals allocate wrappers without resolution; `Get`/`Cast` resolves on demand | **Established/source**: actual constructor probe plus source consumer audit, detailed below. No production consumer of the three missing IDs; no removal/replacement. |
| B05-upstream-hooks — inherited native boundaries, STRE callback additions separated below | `client/SkyrimVM64.cpp`, `TiltedOnlineApp.cpp`, `Games/Memory.cpp` and the finite initializer annex: WinMain 36544; MainLoop 36564; VM update 53926/destructor 40412; form allocation/free 68115/68117; remaining inherited hook signatures/patch recipes | Sentinel during mapping; other hooks at Init; callbacks during runtime | **Established/source reference**, U104/U183. 31 initializer bodies are unchanged after comments/whitespace normalization; this does **not** qualify changed callbacks or new fields in those files. Actor changes belong to C12, animation seating observation to C13, UI changes to C09, Papyrus binding additions to C01. Actual patch spans still belong to B02. |

## Mandatory creation and campaign

| Key / origin | Consumer and exact native contract | Resolution / use | Evidence, missing proof and consequence |
|---|---|---|---|
| C01-papyrus-vm — modified upstream | `client/Games/Skyrim/Misc/GameVM.h`, `Games/Misc/BSScript.cpp`: VM pointer `+210`, inactive counter `+32690`; 104788 registration, 55739 binding, 104359 signature matching; STRE global-bool wrappers reuse native function ABI | Init hooks, engine script binding and gameplay calls | **Established/source**, U885/U183 GameVM adaptation already applied in lot 1; STRE extra functions reuse the same registration ABI. No Papyrus state reconstruction or new persistence authority. |
| C02-save-function — added STRE | `Games/Skyrim/CampaignNativeSave.cpp`: 35727, `bool(BGSSaveLoadManager*, int32 device, uint32 stats, const char* name)` | Resolve/hook at Init; execute on owned save request at engine process boundary | **Established/source**: CL131 `BGSSaveLoadManager::{Save_Impl,Save}` and S231 `GameData.h::Save_Internal`; AL104 RVA **6227F0** equals S231. Correct signature does not validate saved bytes or collective commit. |
| C03-load-function — added STRE | `CampaignSaveLoadGate.cpp`: 35728, `bool(manager*, const char*, int32, uint32, bool checkMods)`; singleton 403340 | Init; exact validated per-player native load | **Established/source**: CL131 exact declaration/call and singleton; AL104 **622F30** equals S231 `Load_Internal`. S231 names its last argument `UInt32`; the `bool` declaration is specifically supported by CL131, not inferred from the RVA. |
| C04-save-process — added STRE | `CampaignNativeSave.cpp`: 35772, `void(manager*)`, trampoline invokes original and drains owned save work | Init hook; engine save/load processing | **Established/source**: S231 `GameData.h::ProcessEvents_Internal` **625B90**, matching AL104; `Hooks_SaveLoad.cpp` calls that original before handling SKSE requests. This proves the boundary/signature, **not** the separate opaque Continue branch in C07. |
| C05-save-path — added STRE | `CampaignNativeSaveCompletion.cpp`: getter 109278 returns utility pointer; vtable slot **2**, `errno_t(this,const char*,char*,bool,bool)`; destination capacity 104 hex | Getter resolves at Init; virtual call on save boundary before filesystem completion worker | **Established/source**: CL131 `BSWin32SaveDataSystemUtility.h/.cpp`, base virtual order; AL104 **159A8D0**. Worker hashing/sidecar checks are ordinary filesystem logic, not an unknown Skyrim ABI. |
| C06-save-selection — added STRE | `CampaignSaveLoadGate.cpp`: manager `saveGameList+100`, BSTArray size 18 hex (`data+0`, `capacity+8`, `size+10`), entry filename `+0`, 35766 `bool(manager*)`; list index zero for most-recent and reverse UI-index mapping | Resolve at Init; manual/most-recent load selection | **Unknown/binary**, partially established/source: CL131 proves fields and 35766 signature; AL104 **624DE0**. Missing target proof of list ordering/selection semantics and UI index mapping before choosing the canonical save. No blanket “save manager layout incompatible” claim. |
| C07-continue-dispatch — added STRE, **production despite Trace name** | `Services/Generic/CampaignSaveTraceService.cpp` plus `CampaignSaveLoadGate.cpp`: queue creator 35769 `void(this,uint32)`; queue vtable 206618 slots **5/6** `bool(this,RequestRef*)`; Request size18, opcode+C; manager queues **358/3B8**; manager **240/248/250/258/260/261**; LoadRequest size28, resolved source+18; source filename **BB0**; dispatcher 442580 `bool(this,SourceRef*,uint32,bool,bool,uint32)`; FxDelegate vtable242193 slot1 | Init resolves/hooks; Continue callback tags queue roots, descendants and exact native load target before policy admission | **Unknown/binary**: AL104 creator **625850**, processor **625B90**, dispatcher **627DE0** exist; CL131 proves Request prefix/queue types but leaves crucial payload/manager meanings unnamed. Need target disassembly of producer → queues → `D0000100`/`D0000010` branches → 442580, allocations, argument widths, filename lifetime and selection. Generic request RTTI and unchanged field offsets cannot establish this chain. Missing proof blocks startup, not just tracing. |
| C08-save-load-callbacks — added STRE | `CampaignSaveTraceService.cpp`: manual adapters 52914/52915, `void(const NativeFxDelegateArgs&)`; args size38, args pointer+28/count+30; GFxValue size18/type+8/value+10; quick-save handler52251; MenuControls vtable215773 slot1 | Init hooks; input/Journal/Continue interception before vanilla transitions | **Unknown/binary**: CL131 establishes public Scaleform/input structures; AL104 adapters **990F20/990F30**, quick handler **961DD0**. Need exact target registration literals, forwarding bodies and parameter semantics. Save/load policy consumers make these mandatory. |
| C09-pause-menu — modified upstream | `Games/Skyrim/Interface/UI.{h,cpp}`, `IMenu.h`, `Services/Generic/CampaignRuntimeGateService.cpp`, `CampaignSaveLoadGate.cpp`: singleton400327, UI map+128 and pause count+160; pointer-key CRC scatter insertion; IMenu flags+1C/size30; UI queue400445 and13631 `void(this,const BSFixedString*,uint32,void*)`; event-source registration and MenuControls toggle | Init inherited UI hook; after native load, before first playable update; menu-open/close lifecycle | **Unknown/binary**, partially established/source: CL131 agrees on UI fields, public menu/message ABI and IDs. STRE replaces map hash policy and requires reachability from the engine lookup; prove the **target's** hash/lookup/insert compatibility and guard-menu processing, including the event sources used in `CampaignSaveTraceService` (+8 save utility/UI, +18 manager). Later engine smoke must prove pause/input/CEF/network coexistence. No inference from matching `sizeof`. |
| C10-reset-main — added STRE | `Games/Skyrim/MainMenuRuntime.cpp::RequestSkyrimMainMenu`: 403449 pointer-to-singleton, quit+10/reset+11/fullReset+12 | Resolve on the marshalled return-to-menu request; write **only reset+11** | **Adapted/source**: CL131 `Main.h` flat event-sink bases+runtime data and `Main.cpp` singleton; AL104 **3230530**. Exact 1.6.1170/1.7.104 wrapper guard; optional presentation hook remains 1.6-only. Event-confirmed recovery departure behavior unchanged. |
| C11-creation-console — added STRE | `Services/Generic/CharacterCreationService.cpp::ExecutePlayerConsoleCommand`: Script factory type19; SCPT text+38, size80; one-byte ScriptCompiler; 441582 `void(this,compiler*,uint32,ref*)`, system compiler1 | At local character progression initialization; factory/heap and compile call are synchronous | **Established/source**: CL131 `Script.h/.cpp` uses 441582 for AE ≥1.6.1130, same fields/compiler/signature. STRE supplies the one-byte compiler object and retains its existing SCPT lifetime policy. Legacy21890 fallback is not proof for another runtime. |
| C12-final-appearance — modified upstream + added STRE | `Services/Generic/{RemoteRespawnLab,RemoteActorProjection,CharacterService}.cpp`, `Games/Skyrim/Actor.cpp`, `Forms/TESNPC.h`: ActorState words+8/+C within actor state; bit decoding; **life9** movement inhibition; private TESNPC clone/final descriptor/headpart checks; actor creation/base binding/spawn and deferred native retirement | Inherited constructor hooks at Init; final authoritative Applied snapshot → safe capture → natural-join replacement → old representation retirement | **Unknown/binary**, source narrowed: CL131 confirms ActorState bit locations but enum names stop at life8; it does not prove STRE's life9 safety exception. U183 supports base creation/materialization signatures, not STRE's additional lifetime/retirement window or private NPC accesses. Need target `SetDontMove` setter/clearer and transition users, constructor/spawn/destructor boundaries 40245/40246/37175/19742 and the added Capture/Geometry/Retire accesses listed below. Exact1.6 guards stay. This is mandatory by ADR-0023; no SwitchRace/hot-apply fallback. |
| C13-local-seating — added STRE using inherited calls | `Services/Generic/CreationSeating.cpp`, `Games/Animation.cpp`: local actor/seat/cell/marker identity and root/graph state; Papyrus `GetSitState`; inherited MoveTo56626 and Activate19796 `bool(this,activator*,uint8,object*,int32,char)`; furniture/action event payloads | After local final Applied; one approach and one native activation per existing intent/token; passive event observation | **Unknown/binary/behavior**: inherited callable boundaries have U183 references; that does not attest target furniture dispatch, event payloads or local-only acceptance semantics previously demonstrated on 1.6.1170. Confirm target wrappers/dispatch and event writers before changing the exact runtime guard. Visible sitting/controls remain a later human smoke obligation; no business behavior changed. |

C12's additional field/use set is bounded by `ObserveBinding`, `Capture`,
`Geometry`, `Advance` and `Retire`: TESForm identity/type/temporary flags;
Actor baseForm/parentCell/race/position/rotation/actorState and STRE extension;
TESNPC raceForm/actorData sex/headparts pointer+count; ActorValues, native
inventory/factions and animation graph projection through existing wrappers.
`Actor::Create` now supplies a chosen cell/worldspace/transform and calls the
STRE lifetime observers. `CharacterService::DeleteTempActor` is the retirement
consumer; recording a delete intent is not proof that the engine has released
the old actor/private base. CL131 supports public layouts (including NPC class
+1C0, color+246 and relationships+250), while the target life9 transition and
the added lifetime assumptions remain the concrete disassembly obligations.

C09 also reads/writes `MenuControls::canBeOpened` at +83, singleton401263.
CL131 and S231 name this byte **unk83**, not a menu-enable Boolean. They
corroborate its storage position but do not prove STRE's interpretation.
The local/S231 view has size90; CL131's flat view has size88 (its VR view90
must not be substituted). STRE only accesses the prefix here, so this tail-size
discrepancy is not itself proof of a broken access or a 1.7 change. Target
MenuControls handlers must establish the +83 meaning alongside native map/hash
and guard menu/event processing. No speculative padding or policy change.

## Optional and diagnostics

| Key / origin | Consumers, timing and contract | Evidence / retained behavior |
|---|---|---|
| O01-grab-release — added STRE | `PlayerCharacter.cpp::TryEndGrabObject`, called by `InventoryService` for forced release; ID40557 `void(PlayerCharacter*)`, resolved/called on demand | **Adapted/source**: CL131 flat `PlayerCharacter.cpp::DestroyMouseSprings`, AL104 **754110**. Exact 1.6.1170/1.7.104 guard, null-resolution failure unchanged. Better Grabbing integration/admission is optional to basic campaign progression and still requires its own plugin evidence. No VR overload. |
| O02-menu-presentation — added STRE | `MainMenuRuntime`/`MainMenuPrompt`: MainMenu215698/CursorMenu215246 slots4/6; music calls52110+2A/52137+2DD; native movie ABI | **Unknown**, Init returns before resolution on 1.7.104. Vanilla fallback permitted. C10 is a separate mandatory recovery boundary and is not disabled with presentation. |
| O03-dialogue-progress — added STRE | `DialogueProgressRuntime`: DialogueMenu215255 slots4/5, TopicManager401099, GFx release82270 and private views | **Unknown**, exact1.6 guard before resolution. Remains optional; no Valen timing/dialogue/content change. |
| D01-appearance-diagnostics — absent from required production flow | `SwitchRaceProbe`, `DoReset3DProbe`, `SexRebuildDiffProbe`, `Character{Sex,Combined}Appearance`, `CharacterAppearanceProbe`, camera snapshot in `CharacterCreationService` | Remain guarded/off for 1.7.104. S231 proves `Actor::SetRace`37925 at **6AD970**, CL131 corroborates TESRace skeletons+98/flags+108 and embedded RaceSex camera+E8. Those facts do **not** require reactivation. `GetRaceSexMenuCamera` feeds presentation logging only. Initial final appearance uses C12, not the legacy humanoid allowlist/live SwitchRace route. |

## The three missing RTTI entries

| ID | Symbol | All relevant consumers | Verdict |
|---:|---|---|---|
| 392519 | `NiObjectNET` | Explicit registry instantiation/declaration; `NiObjectNET.h`, inheritance/includes in `NiAVObject.h` and `NiProperty.h`. No `Cast`/locator consumer of this static type outside registry. | No current mandatory blocker; lazy `Get` would return null. S231 independently names RTTI at RVA20DF188, but no replacement AL ID is inferred. |
| 394235 | `CreationClubMenu` | Registry and forward declaration only | No current mandatory blocker; lazy `Get` would return null. No replacement invented from the menu name. |
| 398437 | `NiParticleSystem` | Registry and forward declaration only | No current mandatory blocker; lazy `Get` would return null. S231 RTTI RVA216AAA0 is corroborating symbol evidence, not an AL replacement. |

`RttiLocator<T>` construction only allocates a `VersionDbPtr<void*>` storing
the ID. It does not call `Get`, `FindAddressById` or the game's cast routine.
`DynamicCast(109689)` is lazy too. `RttiLocator<T>::Get` later converts the wrapper;
an absent sparse AL entry returns null. `Cast<T,U>` does **not** reject null RTTI
arguments before calling the engine, so adding such a consumer is unsafe until
qualified. Neither C++ inheritance nor explicit template instantiation is a
runtime RTTI lookup.

The isolated probe compiles the actual `RTTI.cpp` and actual `VersionDb`/pointer
code, substitutes only the allocator with `std::unique_ptr`, and counts database
accesses through the singleton definition. It loads AL104 with a synthetic image
extent but never dereferences a relocated address or calls Skyrim. It verifies
zero pre-main database accesses, null results for all three, a present TESForm
control, and invalidation after `Clear`. The structural regression scans client
sources for new consumers; full-game/runtime behavior is not tested.

## Finite initializer closure

The [initializer annex](../audit/2026-10-20-runtime-1.7.104-lot2-initializers.md)
enumerates **45 active initializer declarations**: 31 unchanged initializer
bodies, one changed body (Actor), 13 added declarations relative to U183. It
records identifiers, classifications and contract ownership. A commented-out
PlayerControls initializer is excluded. TESNPC's added initializer resolves
14375 but its hook is commented out; no production execution of that body is
inferred. RTTI C++ globals and the mapping sentinel are separately covered by
B04/B01, not missed because they are outside `RunAll`.

The existing generic `Games/SaveLoad.cpp` buffer/FormId serialization hooks are
unchanged upstream and belong to B05/C12, not C02's campaign `.ess` save path.
`Games/Skyrim/SaveLoad.cpp::BGSSaveLoadManager::Save(SaveData*)` is a stub and is
not the production campaign save implementation.

## Gate and later validation

`CampaignAbiVerified` is derived from all 22 entries. The 18 mandatory entries
include eight unsatisfied obligations: **B02, B03, C06, C07, C08, C09, C12, C13**.
There is no runtime override, environment flag, skipped-ID success or partial
campaign admission. Tests reject missing/duplicated/reordered obligations,
demotion of a mandatory contract, and source evidence substituted for required
binary evidence. Qualified wrappers do not enable any other runtime/path.

After static obligations are closed, follow the existing alternate-start,
world-sync, native save/load and collective recovery test plans in a separately
authorized environment. Preserve server authority, a complete sealed roster,
per-member native saves and one collective committed checkpoint. No test of
admission policy establishes that Skyrim, Papyrus, load ordering, furniture or
the campaign works.
