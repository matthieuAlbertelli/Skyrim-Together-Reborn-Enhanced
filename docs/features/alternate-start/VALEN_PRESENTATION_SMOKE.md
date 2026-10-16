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

## A and B history

B changes only this INFO's flags from `0x0A00` to `0x0200`. Its text and all six
WAVs are identical to private A. Other 159 INFO flags remain unchanged. After
local tests/audits and new-candidate xEdit validation, use the established DEV
build/deploy workflow, then STOP for the maintainer's manual launch. No full new
Windows/Linux CI wait is required before this smoke.

The maintainer reported manual skip FAIL for both A and deployed B. No engine
file-read capture was supplied. The later instruction explicitly abandons
Process Monitor and authorizes the normal voice pipeline for C without that
prerequisite. Do not resume instrumentation or infer an engine-load PASS.

## C: native voice prototype on this INFO only

Keep the reviewed B ESP byte-identical: Force Subtitle on, No LIP File off.
There is no VMAD/fragment timer on this INFO to remove. Its six existing silent
PCM WAVs supply the duration and remain unchanged. Treat all six segments of
this one INFO consistently; do not generate assets for any other INFO.

Use the installed Skyrim `Tools/LipGen/LipGenerator` and `LipFuzer` tools in a
private directory. The retained [asset receipt](VALEN_VOICE_C_PROTOTYPE.json)
pins the tool/input/output hashes. For each exact serialized NAM1 transcript:

```text
LipGenerator.exe <basename.wav> <NAM1 text> -Language:French -OutputFileName:<basename.lip>
xWMAEncode.exe -b 48000 <basename.wav> <basename.xwm>
LIPFuzer.exe -s <six-assets-directory\> -d <private-output-directory\> --norec -v 2
```

Run LipGenerator with its private FonixData.cdf beside it. LIPFuzer's directory
arguments require trailing separators; exit 0 alone is insufficient. Require
exactly six outputs named `stre_quest_stre_dial_valen_000d6bc3_<1..6>.fuz` under
`Sound/Voice/STRE_AlternateStart.esp/MaleWarlock/`. Each FUZ embeds its genuine
generated LIP and encoded silent XWM. Keep loose LIP/XWM and logs as private
authoring evidence; import only the six FUZ through the existing CK importer.
Append their manifest paths using the existing Windows `\` spelling returned
by the DEV collector. Before build, apply the actual manifest helper to a private
copy and require identical bytes; mixed separators otherwise create duplicates.

Audit FUZ headers, embedded hashes, native unpack round-trip, decoded mono
44100 Hz/16-bit silence and duration, frozen corpus, unchanged ESP and original
WAVs. `audit_valen_voice_prototype.py` checks the retained prototype bytes and
scope. No ESP authoring occurs, so the unchanged B xEdit receipt still applies.
After local audits/tests, build/deploy DEV and STOP for a human test. Do not
trigger CI or push a PR synchronization during this diagnostic loop.

1. The maintainer launches Skyrim and selects the specified dialogue.
2. Leave it untouched once: subtitles must remain readable, the audio silent,
   and the response progression natural through all six segments.
3. Repeat and tap the normal vanilla skip control once per segment. Require
   immediate single-step advancement, a clean return to the menu, no hang and
   no double progression. Record the control/device and observed results.
4. Check runtime typography as well: ordinary apostrophes/quotes/dashes/ellipsis,
   preserved French accents and guillemets. No runtime PASS is inferred from
   the binary audit.

Do not generalize until the maintainer reports every criterion PASS. Only then
apply the proven policy mechanically and rerun content audits. Full CI remains
required on the later merge candidate.

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
