# ADR-0023 - Automatic final Character Creation rematerialization

- **Status:** Accepted
- **Date:** 2026-09-18
- **Decision makers:** STRE maintainer
- **Issue / discussion:** Character Creation #9; default-activation implementation handoff, 2026-09-18
- **Supersedes:** ADR-0020/0021/0022 in part: default-OFF, explicit debug activation and non-MASTER-only functional execution
- **Superseded by:** None

## Context

The maintainer confirmed one successful A-observes-B final rematerialization and
its visual result. The previous LAB required Ctrl+F11 on publisher and observer
and compiled out the transaction in MASTER. A debug control must no longer be
part of the official initial Character Creation user flow. That product decision
does not extend the scope of the recorded runtime validation.

## Decision forces and options

Retaining an optional toggle would make the final appearance depend on hidden
debug state. An always-enabled diagnostic toggle would misrepresent the contract.
Remove the functional toggle and use the existing final-build boundary directly.

## Decision

Initial Character Creation publishes no live remote appearance updates. Only the
local pending build's matching authoritative NotifyCharacterBuildState::Applied
publishes one canonical final snapshot after the entire build is sealed. All
observers automatically use local natural-join rematerialization, in MASTER as
well as non-MASTER. No menu or shortcut activation is required.

Remove Ctrl+F11 and its functional menu item. F11/Shift+F11 passive lifetime
probes and additional retirement-window diagnostics remain non-MASTER. Existing
internal RemoteRespawnLab names may remain without implying optional behavior.

Retain connection, exact supported runtime, sealed revision, canonical snapshot,
current binding, native safety, capacity, readiness, recovery and generation
fences. Preserve the logical ECS entity/version, serverId, PlayerId and ownership;
only the native Actor/private TESNPC representation changes. No network lifecycle,
natural-join materializer, roster, race classification or Discovery policy change
is part of this promotion. No SwitchRace, Reset3D or hot-apply fallback is allowed.
Failed replacement keeps the old binding when still valid and logs its reason.

Solo remains local with no appearance transport or remote transaction. Intermediate
RaceMenu closes/reopens and later manual showracemenu do not publish. Post-creation
appearance editing and collective seating remain separate future work. Posture
must not be a prerequisite or be normalized by this operation (ADR-0022).

## Consequences and migration

The user no longer needs a debug control to see the final remote appearance.
All build variants include the same functional transaction. Matching schema-2
client/server artifacts remain required; this decision adds no protocol change.
Existing process-lifetime tombstones, bounded waits and deferred retirement remain
unchanged. Restart clients between independent initial-creation acceptance runs.

Automatic activation is not proof of general production readiness. Repeated cycles,
recovery/reconnect, the complete race/sex matrix and native retirement completion
still require separate evidence. Registry presence alone proves neither a leak nor
destruction. There is no new seating contract or approval to alter native lifetime.

## Validation

Require T1-T12 in the feature test plan, TPTests, relevant structural checks and
client/server/launcher builds. Verify that MASTER retains the functional path and
that unchanged Papyrus artifacts remain coherent. Do not infer a new native run
from compilation. Implementation and actual validation belong in
[STATUS](../../project/STATUS.md), with the current behavior in the
[appearance contract](../../features/alternate-start/CHARACTER_APPEARANCE_SYNC.md).
