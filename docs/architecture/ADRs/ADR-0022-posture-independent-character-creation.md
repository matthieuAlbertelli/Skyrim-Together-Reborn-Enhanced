# ADR-0022 - Posture-independent Character Creation and final rematerialization

- **Status:** Accepted
- **Date:** 2026-09-18
- **Decision makers:** STRE maintainer
- **Issue / discussion:** Explicit contract correction in the local implementation handoff
- **Supersedes:** ADR-0021 in part: standing entry and standing-idle LAB eligibility
- **Superseded by:** [ADR-0023](ADR-0023-automatic-final-character-creation-rematerialization.md), in part (retained default-OFF/non-MASTER activation only; posture independence remains)

## Context

The earlier standing contract made native sit/sleep state a prerequisite for
Character Creation entry and for replacing the observer's remote representation.
Seating belongs to a later collective phase and must not block either operation.

## Decision

Character Creation placement resolves durable local PlayerId, its deterministic
rank in the sealed roster, and the existing anchor/cell. It moves the local player
and validates actor identity, cell and reasonable positional proximity before
RaceMenu. Solo retains position zero. It neither reads posture for eligibility nor
requests, waits for or forces a furniture transition. No ActorState write is
introduced. The CK stage-10/11 bootstrap also advances independently of sit/sleep
state after validating its existing marker, player and placement.

Final remote rematerialization has no sitting, standing or furniture precondition
at capture, staging or commit. It rebuilds the local native representation through
the unchanged natural-join materializer/projection and restores its position and
existing minimal gameplay state. It does not repair, normalize or restore a seat.
Other actor/lifecycle guards (including dead, disabled, combat, mounted and
assignment/binding inconsistencies) remain independent of sit/sleep state.

Seating after collective Character Creation completion is a separate future
phase. No seating implementation is part of this correction. ADR-0020/0021's
final publication boundary, non-MASTER default-OFF gate, ECS/network identity,
canonical appearance, generation fences and authority boundaries remain intact.

## Consequences and validation

All sit/sleep field values, including transition states, must have the same
eligibility result when independent guards are equal. Furniture interaction
presence must not veto the LAB. Entry and rematerialization tests enforce this
contract and absence of posture normalization. Invalid PlayerIds, actors, cells,
anchors, positions and incoherent assignment/ownership still fail closed.

Position and geometry checks do not establish native visual success. Human
acceptance must exercise seated and transitional inputs in addition to ordinary
entry, and verify no forced furniture transition. See the feature test plan and
[STATUS](../../project/STATUS.md) for implementation and validation evidence.
