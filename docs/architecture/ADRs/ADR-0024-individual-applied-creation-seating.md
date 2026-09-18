# ADR-0024 - Individual seating after completed Character Creation

- **Status:** Accepted
- **Date:** 2026-09-18
- **Decision makers:** STRE maintainer, explicit individual Applied seating mission
- **Supersedes:** ADR-0021/0022/0023 only where seating was deferred until collective completion
- **References:** #9, #23

## Decision

Each player's authoritative CharacterBuild Applied authorizes that player's
local seating projection immediately after local finalization. Offline Solo
uses successful local finalization. There is no all-Applied or collective
seating barrier. A character at MarkerXX is still creating; a character seated
at SeatXX has completed creation. Native pending states can temporarily delay
that visual indicator, so posture is not authority for build completion.

Keep the existing lexical durable PlayerId rank for MarkerXX and SeatXX.
Transport PlayerId is never used to compute the rank. The existing build-state
notification carries an optional versioned, bounded campaign/durable identity
tail populated from server admission. The server's revision/inventory/spell hash
validation remains the authority; clients never announce seating as Applied.

Observers wait for the committed final native representation and current ECS
binding, then project seating from the game update. This is a read-only fence
on the appearance transaction, not a change to its materializer or lifetime.
No serverId, ECS entity, PlayerId, ownership or roster mutation is introduced.

Use the existing native activation wrapper after checking furniture occupancy
including reservations. Observe native furniture identity and GetSitState only
inside this post-completion projection. Creation entry and rematerialization
remain posture-independent. Never eject an occupant, teleport to a chair,
normalize ActorState or create per-player quest stages.

Duplicate notifications cannot rearm an activation. Native identity changes
reset only that projection's token-specific attempt. Unknown availability stays
pending. Recovery locks suspend projection; disconnect/fresh creation clears
session intentions, requiring fresh canonical evidence after reconnection.

Valen, readiness, departure and any collective next phase remain unimplemented.

## Validation boundaries

Tests cover domain decisions, wire roundtrip/bounds and source integration.
Build success does not execute native furniture activation. Registry availability,
animation entry, occupancy, remote interpolation and the visual indicator need
runtime acceptance on 1.6.1170. STATUS records executions; TEST_PLAN owns scenarios.
