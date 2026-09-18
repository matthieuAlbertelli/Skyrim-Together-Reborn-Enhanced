# ADR-0021 - Natural-join final representation and standing Character Creation

- **Status:** Accepted
- **Date:** 2026-09-18
- **Decision makers:** STRE maintainer
- **Issue / discussion:** Corrective natural-join mission and explicit standing-bootstrap clarification in the local handoff
- **Supersedes:** ADR-0020 in part: local projection scope and the seated-product consequence
- **Superseded by:** [ADR-0022](ADR-0022-posture-independent-character-creation.md), in part (posture-independent entry and LAB eligibility)
- **Activation superseded by:** [ADR-0023](ADR-0023-automatic-final-character-creation-rematerialization.md) (automatic final flow, including MASTER; no debug gate)

## Context

Sharing private TESNPC/Actor allocation alone does not establish the same local
representation as natural join. A separate appearance-domain gate excluded valid
canonical finals, and default native creation initially placed the candidate at
the observer. The CK bootstrap also seated the player before Character Creation.

## Decision forces

The final represents a newly created character, not a mutation of the old native
actor. Logical campaign/network identity must remain stable. A race-specific
replacement strategy would reproduce the hot-apply complexity being removed.

## Options considered

- Retain a separate appearance respawn pipeline with race-family gates.
- Reuse natural-join materialization and local projection without recreating its
  network/ECS identity, and keep seating outside the transaction (selected).

## Decision

Final Character Creation rematerialization is race-agnostic. It recreates the
remote as a natural join representation. No live appearance synchronization
occurs during Character Creation. Players remain standing until a later
post-creation seating phase.

The single publication boundary remains the authoritative sealing of the entire
build, as clarified in ADR-0020. Earlier menu closes and ModifyRace remain local.
AppearanceBuffer, ChangeFlags and FaceTints are the only appearance source;
descriptors validate the result, never choose a race/sex transition algorithm.
Nord, Orc, Khajiit and Argonian use the same materializer and projection. There is
no humanoid allowlist or beast exclusion. Unresolvable/invalid canonical records
and unsafe lifecycle states still reject.

Share native setup, remote/player flags, explicit native spawn placement, actor
values, inventory/equipment and natural WaitingFor3D readiness. Spawn at the
current remote position/cell/worldspace from the initial native call. Preserve
its logical ECS entity/version, serverId, PlayerId, Remote.Id, ownership and
network animation/interpolation baseline; rebind only the native representation.
Retain generation/token-fenced Discovery and engine-managed old-actor retirement.
No server spawn/despawn, assignment, ownership transfer, SwitchRace, Reset3D or
manual TESNPC lifetime management belongs to this flow.

The bootstrap must actually start standing, without requiring GetSitState()==3
or moving/activating the player into furniture. Give each sealed campaign slot a
distinct local creation position before RaceMenu; Solo has a defined position.
Collective seating after all final creations is a separate future slice. It is
neither restored nor used as a respawn success criterion here.

## Consequences

The LAB remains non-MASTER, default OFF, Ctrl+F11, initial creation only. Safe
standing idle is required; mounted, seated and transitional actors reject.
Shared local projection reduces divergence but does not prove visual/native
equivalence. Preserve natural-join identity creation at its existing callers.
If a particular final renders incorrectly, compare the exact canonical snapshot
through natural join before considering capture/payload fixes; do not add a
race-specific replacement branch.

## Migration and validation

Reuse existing CK aliases as coordinate anchors and the non-furniture start
marker through load-order-independent resolution. No new quest-stage protocol is
required. Source/model tests must cover shared projection, race-independent
eligibility, identity retention and standing bootstrap. Human acceptance must
verify physical clearance, correct rendering and lifecycle behavior in fresh
campaigns. Native private-base end-of-life remains an observation question.

Implementation and validation truth is in [STATUS](../../project/STATUS.md), with
the [feature contract](../../features/alternate-start/CHARACTER_APPEARANCE_SYNC.md)
and [test plan](../../features/alternate-start/TEST_PLAN.md).
