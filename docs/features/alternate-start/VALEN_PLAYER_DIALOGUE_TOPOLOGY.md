# Valen Player Dialogue topology

[STATUS](../../project/STATUS.md) owns validation results. The navigation failure
reported on D suspends Hadvar testing until the menu smoke below passes.

## Diagnosis before mutation

The deployed D ESP (`31B8DD797A68BB51542F4C24986C82B21A2154E916B42D80B4574B1D7DB03471`)
has eleven logical root categories. RECRUIT has 22 class-conditioned INFOs with
real Responses. The other ten categories have eleven INFOs in total, including
Helgen NAV_0/NAV_1, with **no TRDT/NAM1 Response and no shared response**.
Their branches already have Top-Level DNAM=1; changing those branch flags is
not the proposed remedy. The reported surviving RECRUIT choice is consistent
with the missing-response hypothesis, but does not by itself prove causation.

Two other serialized defects were found: the generator flattened root links
into almost every response without an explicit root-return topic, and the
Helgen starting topic mixed two navigation INFOs with conditional survivor
responses. The latter cannot provide an unconditional stage >=20 menu entry.
Six explicit Helgen report branches were also Top-Level, even though their
choices belong inside the Helgen category.

The local read-only vanilla inspection used Skyrim.esm SHA256
`2BBC77FDEC35A70EF96B710F8C525E50A1DB9E63E11A391A0EB9EE8F56D36107`.
INFO ownership was read from type-7 GRUPs, and localized NAM1 from the installed
English ILSTRINGS. No Bethesda payload is included in the synthetic fixtures.
Examples and limits of the evidence:

- `DA03VileReturn0` / DIAL `0001C4C8`, INFO `0001C4E2`: one Response and two
  Link To choices, without Invisible Continue. This is a normal multi-choice
  Player Dialogue pattern.
- `BardSongsLute01` INFO `000960DA` contains a single-space NAM1 and a real
  TRDT. This establishes that the field can be serialized, **not** that a blank
  Player Dialogue router has already passed a runtime test. Its sound record
  and scene context are not copied.
- The inventory found 3,260 Top-Level starting INFOs with direct Responses and
  567 with a shared response; four had neither. No empty-text multi-link example
  was found. This does not prove that every vanilla INFO needs a directly
  embedded Response, nor that this proposed router fixes Skyrim.

The [official xEdit SSE definitions](https://github.com/TES5Edit/TES5Edit/blob/dev-4.1.6/Core/wbDefinitionsTES5.pas)
define TPIC as **Previous Topic**, not owning Topic. The historical STRE author
incorrectly wrote the owning DIAL into TPIC. The corrected adapter removes that
unintended dependency; ownership remains the INFO's GRUP. The
[official scripting API](https://tes5edit.github.io/docs/13-Scripting-Functions.html)
provides the container operations used to move the two existing Helgen routers
without changing their FormIDs. Private evidence: `_audit/valen-topology-20261016/`.

## Current authoring contract

`valen_navigation.py` derives navigation from the frozen corpus and historical
matrix plan. `STRE_ValenNavigation.pas` allocates and serializes in official
xEdit. Historical plans and receipts remain intact.

The eleven category branches stay Top-Level. Only the six internal first-report
branches change to Normal, reached through Helgen's explicit links. No internal
submenu becomes a Starting Topic. RECRUIT retains its existing NPC Responses
and repeatability, under the same topology rules as the other categories.

Each purely navigational INFO receives one numbered TRDT and a single ASCII
space in NAM1. It has No LIP File, no Force Subtitle, no Say Once, no Invisible
Continue and no fragment; EXIT additionally retains Goodbye. These are adapter
responses without narrative text or visible subtitle. No voice asset is
generated: no duration is attributed to a missing WAV. Whether the engine
advances this blank response cleanly into its choices is an explicit human
smoke criterion, not an automated PASS.

Links expose the node's follow-ups and its enclosing submenu's choices, plus
the player navigation prompt **Revenons à autre chose.** Its own blank INFO
links to all eleven root categories. EXIT has no links. Root links are not
injected into ordinary submenu responses.

A separate Helgen root DIAL contains exactly three navigation INFOs:

| Narrative stage | Runtime player prompt |
|---|---|
| 0–9 | Que savez-vous de ce qui s'est passé à Helgen ? |
| 10–19 | À propos de notre voyage à Helgen... |
| >=20 | Nous devons parler de ce que nous avons trouvé à Helgen. |

The first two retain their FormIDs, CTDA and RNAM; only their owning GRUP moves.
The third uses GetIsID Valen and GetStage >=20. The old generic survivor DIAL
keeps its responses and conditions inside the submenu. No discovery condition,
class condition, VMAD effect, quest, narrative response, segmentation or
existing WAV changes. Corpus and editorial fingerprint remain frozen; the six
approved runtime typography substitutions remain.

The bounded upgrade adds four records: the Helgen root DIAL, its third INFO,
the root-return DIAL and its INFO. There are 304 planned records, including
300 existing records, 123 DIAL and 162 INFO. Four identities are appended to the
canonical record manifest only during validated import. Packaging is unchanged.

## Local gates and import

The independent reader rejects unexpected records, field layouts, previous-topic
dependencies, root flags, starting topics, links, conditions, response numbering,
visible router text, narrative edits and unsafe typography. It evaluates actual
serialized conditions at seven stage boundaries across all 22 class contexts,
and retains the existing 100-state survivor evaluation. A logical graph alone
no longer grants a topology PASS.

Run the Python navigation tests and PowerShell navigation/presentation guards.
`Test-StreValenNavigationAuthor.ps1` exercises official xEdit DryRun and Prepare
on invented masters and a synthetic matrix fixture. Original hashes must remain
unchanged. This is serialization evidence, never human validation.

From a clean feature branch, use the official wrapper with
`-Action Prepare -Operation FixValenNavigation -ShowXEdit` and configured tool/Data
paths. It pins exact deployed D bytes, source, tools, masters, corpus, companions
and the record manifest. There is no CI invocation. The new saved candidate
must receive human Sort Masters, Check for Errors, save/close, reopen and second
Check for Errors before `ImportValidated`. The receipt requires all those
Booleans and exact saved/log/tool hashes. Previous attestations do not apply.

After import: local content audits, DEV build/deploy, then STOP for human smoke.
No push, full CI, automatic Skyrim launch or Hadvar continuation in this
iteration. PR #97 remains DRAFT and issue #96 remains open.

## Required human menu smoke

1. Open Valen from a normal state and observe multiple root choices.
2. Visit SELF, then **Revenons à autre chose.**; require the full root.
3. Visit RECRUIT, finish/skip normally, close dialogue and speak again.
4. Require the full root again, including the previously visited categories.
5. Before stage 10, verify Helgen's first label and open its submenu.
6. Accept Helgen normally, without revealing either survivor.
7. At stages 10–19, verify the second root label and appropriate submenu.

Also report any blank subtitle, delay/block on a router, unintended root
subtopic, double progression or regression of accepted manual skip. A fixture
PASS does not establish any of these engine behaviors.
