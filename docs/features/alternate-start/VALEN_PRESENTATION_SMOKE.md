# Valen presentation correction and ordered runtime experiment

Steam 1.7.104.0 / SKSE 2.3.1 is the sole future development target. The
1.6 adapter descriptions and observations below remain historical; optional
presentation stays disabled on 1.7 until separately qualified. See
[the native register](../../development/RUNTIME_1_7_NATIVE_CONTRACTS.md) and
[STATUS](../../project/STATUS.md) for current implementation and executed scope.

[STATUS](../../project/STATUS.md) owns observed results. This procedure does not
grant xEdit or runtime PASS. Do not continue Helgen acceptance until this defect
is resolved. PR #97 remains DRAFT and #96 remains open.

## Runtime text projection

Keep the canonical JSON/review corpus unchanged. `valen_presentation.py` applies
only `’ → '`, `‘ → '`, `“ → "`, `” → "`, `— → -`, `… → ...` during private xEdit
authoring. Accents, guillemets, whitespace and response segmentation are preserved.
The reader checks serialized DIAL FULL, INFO RNAM, INFO NAM1 and both narrative
quest objective NNAM fields. It rejects other changes, including any alteration
of graph links, CTDA, scripts, record identities, masters or record flags.
The corpus's Git LF SHA256 is
`88EB2A8690EA3E7BDD7FBE9DF40DA7228B6A4946FB1280107BEB70FDF655E030`;
the session additionally pins its raw checkout bytes. Editorial fingerprint:
`9B29AFABA5E75CF9436817AA77619B3408DB9530D5884F896E05C94EA122D4F5`.

The historical matrix writer/receipt auditor retain their original text contract
so old review evidence can still be reproduced. Use the new bounded presentation
adapter for this correction; it never edits those historical sessions.

## One INFO, six response segments

Use **Parlez-moi de vous. → Qui êtes-vous vraiment ?**. The INFO is
`STRE_INFO_VALEN_SELF_010_R`, local FormID `0D6BC3` in STRE_AlternateStart.esp.
It is repeatable, conditioned only on Valen, and has no fragment/quest effect.
First response: **Un barde, si vous tenez aux réponses courtes.** The subsequent
five segments permit checking both next-response and return-to-menu behavior.

The plugin's NPC VTCK is `0009843A` (MaleWarlock). The path for response 1 is:

```text
Data/Sound/Voice/STRE_AlternateStart.esp/MaleWarlock/stre_quest_stre_dial_valen_000d6bc3_1.wav
```

This uses the actual INFO response number, owning QUST and DIAL, with xEdit's
`InfoFileName` algorithm from its bundled `Edit Scripts/Export dialogues.pas`.
The six existing WAV durations are 3.75, 4.55, 7.683333, 7.683333, 3.75 and
4.683333 seconds. All are mono 16-bit PCM at 44100 Hz, zero samples. Preserve
their hashes across A/B. Do not regenerate or shorten them for the experiment.

The flag definitions are confirmed by [xEdit's SSE definitions](https://github.com/TES5Edit/TES5Edit/blob/dev-4.1.6/Core/wbDefinitionsTES5.pas)
and [Mutagen's Skyrim enum](https://github.com/Mutagen-Modding/Mutagen/blob/dev/Mutagen.Bethesda.Skyrim/Records/Major%20Records/DialogResponses.cs):
Force Subtitle is `0x0200`, No LIP File is `0x0800`.

## D: native permission gate

A, B and C all received a human manual-skip FAIL. The maintainer permanently
abandons voice-asset experiments and engine instrumentation for this defect.
The C receipt and historical implementation remain evidence only; do not generate
or generalize further LIP/FUZ assets. Restore the existing private A ESP exactly
(`31B8DD797A68BB51542F4C24986C82B21A2154E916B42D80B4574B1D7DB03471`),
retain every existing WAV, and remove the six C FUZ from packaging and installed
locations after byte-verified backups. The explicit authorization exempts this
identical restoration from a new xEdit pass; no new attestation is asserted.

`DialogueProgressRuntime` is restricted to Steam **1.6.1170.0**. It chains the
original Dialogue Menu AdvanceMovie, then observes the open menu, movie and live
`MenuTopicManager.currentTopicInfo`. Both source-file origin and winning override
must be `STRE_AlternateStart.esp`, with the corresponding loaded plugin index;
vanilla-origin overrides and other mods are excluded. Unknown/temporary/ESL
forms fail closed. No hard-coded loaded INFO prefix is used.

GFx GetVariable reads `DialogueMenu_mc.bAllowProgress` as a Boolean. False starts
a monotonic 750 ms deadline for that menu/movie/INFO/segment/speaker/session.
If the same response is active at expiry and still false, SetVariable writes
true once with `kNormal`, never a sticky assignment. Re-read confirms the write.
True cancels pending work. A new false edge after vanilla skip starts another
750 ms deadline, including a skip between frame observations. A failed write is
not retried every frame. Missing movie, INFO or variable and non-Boolean values
reset the state and make no write.

Menu hide/show, identity changes, Main/Loading Menu opening, native load entry,
and network connect/disconnect invalidate state. Session resets are serialized
with the UI write. Borrowed native/GFx pointers never escape a frame. The native
message handler always forwards to vanilla; no keys are intercepted, no SkipText
is called, and no SWF or timed auto-advance is installed. Temporary logs record
`INFO`, `plugin`, `allowProgress=false arm`, and confirmed `allowProgress=true`,
with at most 128 transition events per process.

The pure `ProgressGate` tests cover non-STRE and absent state, the exact 750 ms
boundary, repeated true/false cycles, six consecutive segments, failed writes,
and menu/INFO/movie/speaker/response/session resets. The adapter's actual GFx
operation remains a human runtime gate.

## Native ABI references

The adapter uses the same guarded GFx ABI as the existing Main Menu adapter,
without adding a CommonLib/SKSE dependency. Published CommonLibSSE-NG definitions:

- [MenuTopicManager](https://github.com/CharmedBaryon/CommonLibSSE-NG/blob/main/include/RE/M/MenuTopicManager.h): AE singleton 401099, currentTopicInfo at 0x70;
  lastSelectedDialogue at 0x38, parentTopicInfo/currentResponse at 0x30/0x40.
- [DialogueMenu vtable](https://github.com/CharmedBaryon/CommonLibSSE-NG/blob/main/include/RE/Offsets_VTABLE.h): AE 215255; inherited IMenu ProcessMessage/AdvanceMovie slots 4/5.
- [GFxMovie](https://github.com/CharmedBaryon/CommonLibSSE-NG/blob/main/include/RE/G/GFxMovie.h): SetVariable/GetVariable slots 0x10/0x11, normal set mode 0.
- [TESForm](https://github.com/CharmedBaryon/CommonLibSSE-NG/blob/main/include/RE/T/TESForm.h) and
  [BSStaticArray](https://github.com/CharmedBaryon/CommonLibSSE-NG/blob/main/include/RE/B/BSTArray.h): source-file array at 0x08, data/size at 0x00/0x08.

Runtime version, memory readability, callable slots and target types are checked
before access; unknown compatibility fails closed. Readable memory is not proof
of runtime behavior, so the human smoke is mandatory.

## Local workflow and human smoke

Actual human acceptance belongs in [STATUS](../../project/STATUS.md). Preserve
the accepted D gate, WAVs and flags unless a new reproducible defect appears;
continue the [runtime finalization sequence](TEST_PLAN.md#runtime-finalization-sequence-for-96)
on the existing build. Do not repeat completed voice-asset experiments.

If a new bug requires a correction, run targeted local tests, content audits,
client build, then the established DEV build/deploy and STOP. No CI or push during
this loop. The internal collector and
manifest helper must be byte-idempotent. Verify no experimental FUZ/LIP/XWM is
reintroduced and the restored A ESP/WAV hashes match Data/Git/deployment.

The maintainer launches Skyrim and selects the specified INFO. Leave it untouched
once: require visible readable subtitles, silence and normal idle duration.
Repeat and wait more than 750 ms before pressing the ordinary vanilla control
used for other NPCs. Require each segment to skip, several successive skips,
clean next-segment and menu return, and no double-skip. Confirm vanilla dialogues
remain unchanged and French typography is preserved. Record actual control and
observations; neither tests nor a successful GFx write constitutes runtime PASS.

PR #97 stays DRAFT, #96 stays open. After runtime completion, report final local
evidence and stop for explicit approval before push, full CI, Ready or merge.
Do not change GitHub workflows as part of this mission.
