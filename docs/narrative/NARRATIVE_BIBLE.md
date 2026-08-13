# Narrative bible

> **Status: Foundational decisions accepted; development remains**

## Premise

Several strangers receive a personal invitation from Valen. Each believes they were summoned alone. When they arrive at an inn isolated from Skyrim's normal flow, they discover that Valen gathered other individuals based on rumors, accomplishments, and fragmented testimony.

Valen believes one of them may be the Dovahkiin. He does not know which one, and neither does the group. The story concerns the company's formation as much as the Dragonborn's emergence.

## Themes

- uncertain destiny;
- trust between strangers;
- collective strength;
- truth assembled from rumors;
- tension between chosen and imposed roles;
- shared consequences.

## Tone

Consistent with Skyrim: serious but human. Valen may be awkward or excessively convinced without becoming comic. The scene must leave room for player characters and must not reduce them to extras.

## Continuity rules

- Valen does not personally know any player at the start.
- Every invitation is individual.
- He apologizes for not announcing the other guests.
- The Session Manager is not necessarily the Dragonborn.
- Dragonborn is a narrative function, not a host privilege.
- Companions remain essential.
- The introduction must not contradict vanilla facts needed later.

## Introduction structure

### Beat 1 — Arrival

The players discover one another, observe the inn, and understand that they have been brought together.

### Beat 2 — Valen's explanation

Valen presents his sources, theory, and urgency without claiming certainty.

### Beat 3 — Friction

Players may doubt, mock, ask for proof, or accept. The cooperative system must tolerate different local reactions while retaining canonical progression.

### Beat 4 — Formation

The group chooses roles and classes, then prepares. Valen presents this complementarity as a necessity, not an abstract menu.

### Beat 5 — Departure

The company leaves the inn. The Dragonborn reveal is deferred.

## Campaign roster and interruption

The multiplayer campaign roster exists before the campaign starts. Once sealed,
all members participate in canonical progression; no new player or replacement
character enters the campaign in v1.

A required-member disconnect interrupts progression, including the introduction
scene. The scene resumes only through collective restoration of the last committed
campaign checkpoint, not through a catch-up summary for an arriving or returning
player. Network and runtime details are governed by
[ADR-0018](../architecture/ADRs/ADR-0018-fixed-roster-coordinated-checkpoint-recovery.md)
and the [Campaign State model](../architecture/CAMPAIGN_STATE.md).

## Group dialogue

The campaign phase is shared, but rendering may remain local. Responses should become group votes only when they actually change canonical state.
