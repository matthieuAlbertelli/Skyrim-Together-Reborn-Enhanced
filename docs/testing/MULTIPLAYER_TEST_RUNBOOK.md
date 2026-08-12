# Multiplayer test runbook

> **Status: Active procedure for two-PC smoke tests; future automation planned**

## Preparation

- identical client and server commits;
- identical `BuildVersion`;
- identical `STRE_AlternateStart.esp`, verified by SHA-256;
- identical load order and masters;
- restarted server;
- cleared or archived logs;
- closely synchronized system clocks;
- recorded player identifiers;
- shared scenario and expected result.

## Character Build test

On each PC:

```text
resetquest STRE_QUEST_AlternateStart
startquest STRE_QUEST_AlternateStart
setstage STRE_QUEST_AlternateStart 10
```

Choose different builds and record the class, Destruction/Alteration options, revision, inventoryHash, and spellHash.

Expected result:

```text
Build accepted
Canonical spells applied
Applied acknowledgement sent
Build applied
```

None of these lines should appear:

```text
RejectedInventoryHash
RejectedSpellHash
spell resolution failed
build rejected
```

## Targeted-buff test

- retain `Fire and Forget` plus `Target Actor` in SPEL/MGEF;
- target the other player at close range;
- cast the localized Mineral Aegis, Water Breathing, or Lighten Burden spell;
- compare `DamageResist`, WaterBreathing state, or `CarryWeight` before and after;
- verify expiration;
- collect MagicService and add-target events.

## During the test

Mark T0 connection, T1 creation, T2 sealing, T3 buff, T4 anomaly, and T5 end. Retain player IDs, server IDs, revisions, and timestamps.

## Evidence collection

- `tp_client.log` from each player;
- server log;
- video or screenshot for UI or rendering issues;
- relevant save;
- mod list and load order;
- ESP SHA-256;
- exact steps;
- whether the result is reproducible.

## Useful PowerShell filter

```powershell
Select-String `
  -Path $log.FullName `
  -Pattern "CharacterBuild|CharacterCreation|MagicService|Spell grant|spellHash|inventoryHash|rejected|failed" |
Select-Object -Last 400 |
ForEach-Object { $_.Line }
```

## Future failure scenarios

- terminate a client while Pending;
- disconnect the network before acknowledgment;
- reconnect;
- restart the server;
- use a mismatched BuildVersion;
- use a different ESP;
- send a duplicate;
- remove a plugin or master.

Reconnect and restoration scenarios cannot be declared successful until build persistence is implemented.
