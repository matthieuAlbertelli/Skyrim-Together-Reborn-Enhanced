# Downed State — Product Spec

> **Status:** accepted vision; not implemented in the archived code.

## State machine

```text
Healthy
→ Downed (30 seconds)
→ Out of Combat
→ Prolonged treatment after combat ends
→ Healthy
```

## Goal

Replace immediate respawn with a readable cooperative consequence. Allies have a
window in which to intervene; after 30 seconds, the player remains out of combat
until the encounter ends.

## Rules

- healing during Downed can revive the player;
- after the timeout, combat healing can no longer revive the player;
- an out-of-combat player returns only after combat ends and prolonged treatment;
- state is shared and authoritative;
- disconnect/reconnect preserves state;
- no automatic magical teleportation.

## Future capabilities

- `player.down-state/1`
- `group.revive/1`
- `combat.encounter-state/1`
- `injury.persistence/1`
