# Alternate Start — Test plan

> **Status: Automated audits plus M7, New Game, MQ101/post-Helgen, pre-deadline wounded-survivor, and focused campaign-bootstrap checks executed; the campaign Solo/two-player happy path passes while the remaining runtime matrices are pending**

## Completed checks

- successful Windows xmake build;
- conforming strict audit of 63 STRE-owned CK records plus the explicit
  Skyrim-master override allowlist;
- conforming audit of 41 catalog/ESP references;
- compiled `Code/tests/character_build.cpp` tests;
- in-game Mage bootstrap test;
- targeted buffs tested between two PCs;
- single-player fallback present in the service and tested through the build flow;
- New Game interception validated for first and same-process second New Game;
- MQ101 structural and generated-fragment invariant audits conform;
- post-Helgen MQ101/world-state projection validated in game after xEdit Quick
  Auto Clean while MQ102/MQ102A/MQ102B remain untouched;
- native `TPTests` pass with 126 test cases and 1753 assertions, including the
  focused join-code directory, wire validation, opcode stability, native CEF
  binding manifest/action routing, bounded Unicode lobby-pseudo validation, and
  one-shot local campaign-bootstrap gate tests;
- the seven focused Playwright campaign-bootstrap scenarios pass: entry/create,
  entry/join and code normalization, Solo intent, shared persisted connection
  address without password persistence, lobby projection/Start authority,
  disconnected form/back behavior, and required/trimmed/bounded Unicode pseudo
  handling across the five-argument native action contract;
- pre-deadline Helgen investigation bootstrap validated with independent Hadvar
  and Ralof wounded positions and dedicated wounded packages;
- bidirectional rubble-squeeze interaction validated in game with the expected
  prompt, fade, and local player transfer;
- investigation quest exclusion from generic quest synchronization builds
  successfully;
- TPTests pass with 1511 assertions in 112 test cases;
- CK packaging audit passes with 17 managed files and zero compiled PEX files
  under `Scripts/Source`;
- `git diff --check` passes for the current increment.

These checks do not constitute exhaustive validation of every combination.

## Static audits

```powershell
py -3 .\Tools\Scripts\audit_stre_plugin_records.py `
  .\GameFiles\Skyrim\STRE_AlternateStart.esp `
  --manifest .\docs\features\alternate-start\CK_RECORDS_M7_IMPLEMENTED.json `
  --output .\_audit\STRE_AlternateStart.records.m7.tsv `
  --strict `
  --reject-unexpected
```

```powershell
py -3 .\Tools\Scripts\audit_mq101_quickstart5.py `
  .\GameFiles\Skyrim\STRE_AlternateStart.esp
```

```powershell
py -3 .\Tools\Scripts\audit_mq101_generated_invariants.py `
  .\GameFiles\Skyrim\Source\Scripts\QF_MQ101_0003372B.psc
```

```powershell
py -3 .\Tools\Scripts\audit_character_build_catalog.py `
  .\GameFiles\Skyrim\STRE_AlternateStart.esp `
  .\Code\common\CharacterCreation\CharacterBuildCatalog.cpp `
  --client-source .\Code\client\Services\Generic\CharacterCreationService.cpp
```

## Local smoke test

```text
resetquest STRE_QUEST_AlternateStart
startquest STRE_QUEST_AlternateStart
setstage STRE_QUEST_AlternateStart 10
```

Verify RaceMenu, UI, preview, summary, level 1, exact inventory, equipment, exact spells, cleanup, and absence of hash rejection.

## Mage matrix

Test all nine combinations:

- Fire × Protection;
- Fire × Exploration;
- Fire × Matter;
- Frost × Protection;
- Frost × Exploration;
- Frost × Matter;
- Shock × Protection;
- Shock × Exploration;
- Shock × Matter.

Each build must produce exactly 7 canonical spells: 3 Destruction plus 4 Alteration.

## Priority multiplayer tests

- identical client/server/plugin versions on both PCs;
- independent creations with different choices;
- Accepted, then Applied states;
- no `RejectedInventoryHash` or `RejectedSpellHash`;
- remote appearance and equipment;
- Mineral Aegis: `DamageResist` increases, then returns;
- Water Breathing: effect active, then expires;
- Lighten Burden: `CarryWeight` increases, then returns;
- no application to the wrong target;
- controlled recasting and stacking.

The three spell names above are translated descriptions of currently localized French display strings; runtime IDs remain unchanged.

## Class regression tests

- Warrior: heavy equipment, weapons, smithing, pendant;
- Thief: outfits, weapons, 10 lockpicks;
- Mage: visual outfit, 7 spells;
- build change before acknowledgment;
- reject a second build after the Applied state.

## New Game bootstrap regression

Validated on 15 August 2026:

- first New Game enters the inn and reaches Character Creation;
- during the initial character-creation bootstrap, MQ101 remains at stage 0
  and the STRE Alternate Start quest reaches stage 20;
- a second New Game works after returning to the main menu without restarting Skyrim;
- an ordinary existing save loads without retriggering the bootstrap.

## Post-Helgen continuity regression

Validated on 16 August 2026:

- MQ101 reaches stage 1000 and stops;
- MQ102, MQ102A, and MQ102B remain untouched;
- the cleanup helper reaches its completion marker;
- Helgen exterior remains destroyed;
- entering `HelgenKeep01` shows already-collapsed rubble without replaying the
  vanilla proximity collapse or dragon/collapse roar;
- the accepted minor limitation is a brief rubble sound during Keep load.

## Campaign-bootstrap runtime evidence and pending matrix

Run from a fresh Skyrim process unless the scenario says otherwise. Retain the
matching client and server logs; automated tests are not evidence that these
runtime scenarios passed.

Runtime prerequisite: disable `Alternate Start - Live Another Life` whenever
`STRE_AlternateStart.esp` is active. The two alternate-start implementations
were observed to be incompatible during this validation. This slice does not
add compatibility behavior.

### Executed happy-path evidence — 23 August 2026

The following observations, and only these observations, are recorded as passed:

- the Solo bootstrap works;
- Create Campaign works on the first PC;
- a second PC joins that campaign with its four-character code;
- both players progress through Character Creation and arrive in the STRE inn;
- the persisted last server address is reused;
- the transient creator/joiner pseudos appear correctly in the lobby.

### A — Solo regression — validated scope

The Solo bootstrap was exercised successfully. No additional negative Solo
scenario is claimed by this evidence.

### B — two-PC Create/Join happy path — validated scope

PC A created a campaign and PC B joined it with the displayed four-character
code. Both selected pseudos were displayed correctly, the saved server address
was reused, and both players completed Character Creation and arrived in the
STRE inn.

On a future repetition, retain server evidence for the create/join/start
operation and result, internal campaign/revision context, PartyService
alignment/leadership decision,
and the sealed `CharacterCreation` snapshot. Retain client evidence for
the bootstrap transition, lobby projection, canonical authorization, and a
single CharacterCreation release. Logs must not contain passwords.

### C–L — pending manual scenarios

- C: A creates, B then C join, and Start seals exactly those three members.
- D: an unknown valid-shaped code is rejected without roster/party mutation or
  CharacterCreation release.
- E: a valid code entered in lowercase is normalized and joins the same lobby.
- E2: empty, control-containing, or over-24-code-point pseudos are rejected
  before connection or party/campaign mutation; surrounding whitespace is
  trimmed and ordinary Unicode pseudos are preserved.
- F: after A+B seal, C cannot reuse that code and the sealed roster is unchanged.
- G: B disconnects before Start; A cannot seal/progress until B legitimately
  reconnects/resumes, after which Start succeeds.
- H: B cannot force Start through a manipulated UI or direct request.
- I: with two simultaneous lobbies, B joins A's code and D joins C's code with no
  campaign or transient-party cross-wiring.
- J: wrong password, unreachable server, and protocol mismatch keep the mandatory
  bootstrap active and never release Character Creation.
- K: a second New Game in the same process reopens a clean bootstrap.
- L: loading an ordinary existing save does not open the bootstrap.

For each pending scenario, retain the operation/result, internal campaign and revision
where available, transient/durable player context allowed by logging policy, and
PartyService decision. Confirm the roster is unchanged and no password is logged.
None of C–L is claimed as manually passed by the happy-path run above.

## Pre-deadline Helgen investigation regression

Validated on 20 August 2026:

- project post-Helgen continuity first and confirm MQ101 reaches stage 1000;
- start `STRE_QUEST_HelgenInvestigation` and advance its diagnostic stage 10;
- verify Hadvar, Ralof, and both wounded-marker aliases are filled;
- verify Hadvar and Ralof reach their independent intended locations;
- verify Hadvar uses the Wounded02-based furniture pose and Ralof the
  Wounded03-based furniture pose;
- verify neither actor immediately resumes an incompatible vanilla travel
  package;
- verify `Se faufiler` appears only around the intended rubble opening;
- traverse the rubble in both directions and verify fade, destination, and
  immediate movement after arrival;
- verify the dead bandit and pickaxe do not obstruct the interaction;
- verify the strict record audit reports 63 expected records with no unexpected
  master override;
- verify `audit_ck_packaging.py`, client build, TPTests, and
  `git diff --check`.

Still required for this slice:

- save/load while the investigation is active;
- repeat/reapply stage-10 projection and confirm safe behavior;
- cell reset to determine whether the current vanilla corpse ActorBase respawns;
- multiplayer traversal with two players using the squeeze independently.

## Tests still blocked by missing features

- four-day deadline and safe-boundary world-phase transition;
- post-deadline bandit occupation and prisoner placement;
- rescue/liberation and `Freed`/`CapturedInKeep` survivor transitions;
- neutral MQ102/MQ103 vanilla main-quest handoff;
- Valen and scene;
- exit and vanilla resumption;
- save/load at every campaign phase;
- sealed roster, coordinated checkpoints, disconnect recovery, and collective
  build/campaign restoration;
- 4 and 10 players.

## Future campaign continuity matrix

Roster and activation:

- extra player after roster seal is rejected;
- replacement player and wrong `CharacterBinding` are rejected;
- activation with one missing roster member is rejected;
- full roster activates only with every expected slot/binding.

Disconnect boundaries:

- disconnect during open world;
- disconnect during combat;
- disconnect during dialogue or a scene;
- disconnect around a quest-stage change;
- disconnect during checkpoint creation;
- no persistent campaign mutation or new checkpoint is accepted while in
  `RECOVERY_LOCK`.

Checkpoint failure and restore:

- one player save failure;
- client crash during a candidate checkpoint;
- server interruption before and after the checkpoint commit boundary;
- a failed candidate leaves the previous committed checkpoint valid;
- all clients load the exact checkpoint and per-slot save selected by the server;
- wrong, stale, missing, or mismatched save metadata is rejected;
- restored server revision matches the selected checkpoint;
- resume occurs only after all roster members acknowledge the same restore;
- retry and duplicate/delayed acknowledgements are idempotent;
- no duplicate build grants occur after collective restore.

## Log collection

For every test, retain the date, runtime, load order, BuildVersion, client/server versions, logical choices, and `CharacterBuild`/`CharacterCreation`/`MagicService` lines. `_audit` reports, TSV files, and logs remain local and are not versioned.
