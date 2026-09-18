# ADR-0020 - Character Creation final local materialization

- **Status:** Accepted
- **Date:** 2026-09-18
- **Decision makers:** STRE maintainer, explicit approval in the Slice 3 revised mission
- **Issue / discussion:** Local implementation handoff; no GitHub issue created by this mission
- **Supersedes:** The Character Creation hot-appearance strategy in the feature contract, not a prior ADR
- **Superseded by:** [ADR-0021](ADR-0021-natural-join-final-representation-standing-creation.md), in part (natural-join projection and standing creation)
- **Activation superseded by:** [ADR-0023](ADR-0023-automatic-final-character-creation-rematerialization.md) (automatic final flow, including MASTER; no debug gate)

## Context

Remote race/sex hot updates have not established a reliable rendering contract.
Appearance allocation provenance can disappear during campaign recovery while
the native remote remains bound. It is not a durable materialization registry.

## Decision

Initial STRE Character Creation uses no remote appearance preview while RaceMenu
is open. It publishes a canonical final AppearanceBuffer/ChangeFlags/FaceTints
snapshot, then observers systematically replace their local native representation,
including unchanged appearance and cosmetic-only, sex-only, race-only and combined
changes. Race/sex/weight descriptors validate the result, never select an algorithm.

The replacement preserves serverId, PlayerId, network ownership, sealed campaign
roster and the logical ECS entity. It creates a new local Actor and private TESNPC
through the shared materializer. No server spawn/despawn, RequestServerAssignment,
ownership transfer, SwitchRace or Reset3D belongs to this flow. Native retirement
uses existing engine-managed deletion; no manual TESNPC free or reference counting.

The first LAB must be non-MASTER and explicitly enabled, default OFF. It requires
session/entity/version/server/generation fencing, candidate-local tints, validated
candidate geometry, isolated Discovery routing and current minimal-state replay.
The appearance provenance marker is not the durable identity source. Unknown
eligibility or contradictory binding must reject before candidate creation.

Legacy hot pipelines can remain compiled, but must be unreachable from the new
Character Creation flow. They are not a fallback if LAB activation fails. Later
manual showracemenu and post-creation appearance editing are outside this decision.
Solo remains local. ADR-0002, ADR-0012 and ADR-0018 authority rules remain intact.

## Consequences

Observers retain the preceding appearance until final publication. Native creation,
Discovery and retirement become one explicit transaction instead of race/sex hot
rebuild branches. Candidate failure must preserve the old representation when valid.
Base registry persistence alone is neither completion nor leak evidence. Seating
can remain unvalidated for the first LAB, but the seated product is then incomplete.

## Final cycle boundary (maintainer clarification, 2026-09-18)

ONE final for the entire initial creation, published AFTER authoritative Character
Build sealing. All earlier RaceMenu closes, including ModifyRace/reopen cycles,
remain local. The source event is the local pending build's matching server
NotifyCharacterBuildState::Applied, not the menu close. Canonical inventory and
equipment exist at this boundary. Duplicate Applied notifications do not republish.

The existing appearance message carries the nonzero sealed build revision in
schema 2. The server validates owner, Applied revision and race, freezes the first
accepted final and permits only identical retransmissions for that revision.
This requires matching client/server builds; it is not a new network lifecycle.

## Validation and implementation notes

See [the feature contract](../../features/alternate-start/CHARACTER_APPEARANCE_SYNC.md),
[test plan](../../features/alternate-start/TEST_PLAN.md), and
[STATUS](../../project/STATUS.md). Decision acceptance is not implementation or
runtime validation. No implementation completion is asserted by this ADR.
