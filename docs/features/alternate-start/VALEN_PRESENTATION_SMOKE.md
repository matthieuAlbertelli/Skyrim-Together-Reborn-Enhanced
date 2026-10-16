# Valen presentation correction and ordered runtime experiment

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

## One INFO, unchanged audio

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

## A: current behavior and actual file reads

The reported c48be63 failure establishes the general symptom; it does not supply
a trace for this selected INFO. Obtain that trace before concluding the WAV is
resolved by the engine. A can be measured on the deployed c48be63 baseline; the
private A candidate additionally fixes typography while retaining every flag.

1. The maintainer starts [Process Monitor](https://learn.microsoft.com/en-us/sysinternals/downloads/procmon),
   clears the capture and includes `Process Name is SkyrimSE.exe` plus paths
   containing `\Sound\Voice\STRE_AlternateStart.esp\`. Capture before launching
   a fresh Skyrim process, so a cached asset cannot hide the first read. The agent
   does not launch Skyrim. Keep the capture bounded to this short test.
2. Record the effective profile/load order and installed ESP/WAV hashes. If an
   override changes Valen's VoiceType or the INFO, stop and resolve it first.
   Navigate to the selected INFO, note the time, and leave response 1 untouched.
   Compare its observed duration to the 3.75-second PCM duration.
3. Require successful `CreateFile` and nonzero successful `ReadFile` activity for
   the exact `_000d6bc3_1.wav` path by SkyrimSE.exe, correlated to this reply.
   Search for failed alternate paths/extensions too. Preserve the PML and a
   filtered CSV, including time, PID, operation, path, result and detail. A file
   listing or an editor's read is insufficient. OS reads establish consumption
   of bytes; duration/visual observation is still needed for playback behavior.
4. Repeat the INFO and tap the ordinary vanilla dialogue-skip control once after
   reading each segment. First verify that same binding advances a voiced vanilla
   dialogue in this profile. Record actual control/device, durations and result;
   do not substitute console stage changes or synthetic key spam.
5. Exit Skyrim completely before changing variants. If the expected WAV is not
   read, investigate resolution before B. Do not infer a No LIP File defect.

## B, then C only if needed

B changes only this INFO's flags from `0x0A00` to `0x0200`. Its text and all six
WAVs are identical to private A. Other 159 INFO flags remain unchanged. After
local tests/audits and new-candidate xEdit validation, use the established DEV
build/deploy workflow, then STOP for the maintainer's manual launch. No full new
Windows/Linux CI wait is required before this smoke.

Repeat the same capture, untouched timing trial and single-tap skip trial. PASS
requires sufficient idle subtitle time, immediate single-step vanilla skip,
clean progression through the next response and back to the menu, silence,
and no hang or double advance. Preserve exact ESP/WAV hashes and evidence for
each variant. B is an experiment, not a proposed proven fix.

Only after a recorded B failure with confirmed WAV resolution may C generate
a genuine SSE-compatible LIP for the **first response only**, with No LIP File
still off. Keep other responses as controls; use an actual compatible generator,
not an invented empty LIP or renamed arbitrary file. Its absence for the other
five responses must not be mistaken for failure of the first-line experiment.
Do not generalize until a variant satisfies every criterion in game. Then apply
that proven presentation policy mechanically, generate only required assets,
and rerun the full content audits. Full CI is required on the merge HEAD.

## Private preparation

From a clean committed feature branch after `git fetch origin`:

```powershell
./Tools/Scripts/Invoke-StreModding.ps1 -Action Prepare -Operation FixValenPresentation -Variant A -ShowXEdit
./Tools/Scripts/Invoke-StreModding.ps1 -Action Prepare -Operation FixValenPresentation -Variant B -ShowXEdit
```

The usual configured AutoMod/xEdit/Data paths apply. Each invocation preserves
its own baseline, masters, companion hashes, plan and official xEdit script,
executes an immutable dry-run, then creates an independently audited candidate.
No Papyrus compilation, audio generation, manifest mutation or Data write occurs.
Preparation stops at `HUMAN_XEDIT_REQUIRED`. The old c48be63 human receipt cannot
validate new bytes: follow the [official xEdit boundary](../../development/MODDING_TOOLCHAIN.md#official-xedit-boundary)
on each candidate that will be deployed. No receipt is synthesized by this tool.
