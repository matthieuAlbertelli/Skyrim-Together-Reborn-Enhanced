# ADR-0025 - Existing CK markers as local seating approaches

- **Status:** Accepted
- **Date:** 2026-09-19
- **Decision makers:** STRE maintainer, explicit marker-reuse instruction
- **References:** #9, #23
- **Supersedes:** ADR-0024 only for the local approach primitive; individual Applied remains unchanged

## Context

The Seat01 prototype established the usefulness of explicit pre-positioning
before native activation. Seat-specific C++ offsets and obstacle models would
duplicate layout authored in CK. The maintainer explicitly rejected adding ten
new SeatApproach references and chose the existing Marker01..10 instead.

## Decision

Reuse STRE_REFR_PlayerCreationMarker01..10, paired with the existing
STRE_FURN_PlayerSeat01..10 by lexical durable PlayerId rank. The same markers
remain the initial Character Creation positions. Their placement and heading
are authored manually in CK; changing either affects both uses.

After its own Applied and local finalization, each non-MASTER client may move
only its native PlayerCharacter to the exact assigned marker position. Observe
arrival in the correct cell, apply the marker's full upright orientation, wait
for a later engine UpdateEvent, then call the existing SeatXX Activate wrapper.
Keep one MoveTo and one Activate per attempt/token, bounded preparation, native
identity, occupancy, campaign/session and recovery fences. No collective Applied
barrier, remote MoveTo, new packet, forced animation or ActorState writes.

There are no furniture-derived offsets, table/obstacle models or fallback seat
assignments in C++. Resolve existing plugin-local IDs through the loaded plugin;
validate marker type, cell and transform. Do not create new CK references, alter
PSC/PEX or revive a posture prerequisite for creation/rematerialization.

This decision does not promote the prototype into MASTER or validate remote
seating projection. Existing remote projection behavior is unchanged.

## Consequences and validation

Layout authors must keep all ten markers enabled, persistent, distinct and
upright, at valid entries for their assigned chairs. A numeric spacing threshold
cannot prove collision clearance; physical placement and visible native sitting
need human validation, first Solo and the local B player in a full roster of two.
Tests prove rank/pair resolution, exact transform use, update ordering and fences.
The read-only asset audit proves record/binding integrity, not safe pathing or pose.

Contracts and CK procedure: [CK_IMPLEMENTATION](../../features/alternate-start/CK_IMPLEMENTATION.md).
Executed evidence and remaining validation: [STATUS](../../project/STATUS.md).
