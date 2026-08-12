# Alternate Start — Product specification

> **Status: Product target accepted; character bootstrap partially implemented**

## Pitch

Start a campaign in an inn without importing an already-advanced character. Each player creates their appearance, chooses a class and kits, then receives a clean build consistent with the campaign.

## Implemented today

1. Teleport to an inn seat through the CK quest.
2. Open RaceMenu.
3. Select Warrior, Mage, or Thief in the Angular UI.
4. Select available kits.
5. Use the 3D preview and summary screen.
6. Submit the final “Seal your destiny” choice.
7. Apply anti-import cleanup, level 1, canonical inventory, and canonical spells.
8. Use the local offline path or server validation in multiplayer.
9. Apply functional targeted Alteration buffs to a remote companion.

## Complete product goals

- skip Helgen cleanly;
- prevent unvalidated external characters from entering;
- create characters at the same narrative point;
- work in single-player without an STRE server;
- provide a readable hub for a small group;
- serve as the first reference first-party integration;
- restore state after reconnecting;
- leave the inn with coherent vanilla progression.

## v1 class roster

STRE v1.0.0 targets the 21 classes defined in [`CLASS_ROSTER_V1.md`](CLASS_ROSTER_V1.md). That canonical document defines each class's stable identity, canonical English name, French localized display name, and two major/four minor skills.

Warrior, Mage, and Thief form the first implemented vertical slice. The kits, items, quantities, spells, cooperative abilities/perks, and personal quests for all 21 classes remain governed by their dedicated design and implementation sources; they are not inferred from old documents under `history/`.

## Target experience

1. Redirect a new game to the inn.
2. Create appearance at the table.
3. Choose class and kits.
4. In STRE, create or join a campaign and bind the character.
5. Validate every build.
6. Introduce Valen.
7. Complete the ready check.
8. Depart for Skyrim together.

Steps 2 through 5 are partially present; steps 1, 6, 7, and 8 remain to be completed.

## Single-player

- the same creation flow in the inn;
- the same local catalog;
- no mandatory roster or ready check;
- no mandatory server dependency;
- state retained in the Skyrim save.

## Current STRE behavior

- server validation of race, class, and selections;
- canonical inventory and spells;
- hashes and application acknowledgment;
- Pending/Applied state broadcast;
- build state does not persist beyond the session.

## Target STRE behavior

- canonical campaign;
- roster and bound characters;
- shared phases;
- secretly assigned Dragonborn;
- server-authorized departure;
- snapshot and restoration after reconnecting.

## Out of immediate scope

- dynamic Dragonborn assignment based on accomplishments;
- late join after departure;
- intentional import of an existing character;
- complete rewrite of the vanilla campaign;
- stable third-party SDK before several first-party integrations exist.
