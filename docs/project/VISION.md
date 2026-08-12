# STRE Product Vision

> **Status:** product decision.

## Ambition

Create the Skyrim cooperative experience that groups of players can live as a
genuine shared campaign: strangers become a company, their roles complement one
another, their decisions produce shared state, and the world remains recognizably
Skyrim.

## Value proposition

STRE provides three levels of value:

1. **Cooperative mechanics absent from STR**, such as secure trading, the downed
   state, revival, and cooperative classes.
2. **A structured campaign**, with a shared start, a narrative hub, and shared
   canonical progression.
3. **An integration platform** that lets a mod developer describe how a mod's
   single-player logic should behave in an STRE session.

## Non-negotiable principles

### Skyrim first

Players must continue to recognize Skyrim's pacing, systems, and feel. STRE adds
a cooperative layer; it does not replace the game with MMO logic.

### Cooperation over punishment

Mechanics should create opportunities to help, protect, revive, equip, or
coordinate other players. Difficulty must not rely on arbitrary penalties.

### Explicit authority

Every state that can diverge between clients must have an identified authority:
the server, campaign, owning player, or a declared policy.

### Standalone single-player mods

An STRE-compatible mod should retain a functional single-player mode. The STRE
adapter enriches its semantics; it must not make STRE mandatory when the content
can operate independently.

### Modular and maintainable

The network core, business domain, Skyrim integration, UI, and CK content must
remain separate. Features must be independently testable and documented by
contract.

### Network resilience

Systems must tolerate duplicates, delays, temporary loss, reconnections, and
snapshots. A scene visible locally is never, by itself, canonical truth.

### Pragmatic upstream compatibility

The fork should track Skyrim Together Reborn when the cost is reasonable.
Structural divergences must be isolated, documented, and easy to rebase.

## Target audience

- groups of two to four players for the primary experience;
- planned technical capacity for approximately ten players in hubs and events;
- Creation Kit and Papyrus modders;
- C++ network and Skyrim reverse-engineering developers;
- artists, writers, voice actors, sound designers, and testers.

## What STRE is not

- a large-scale persistent MMO;
- a promise of automatic compatibility with every mod;
- naive synchronization of every local variable;
- a replacement for the mod developer's responsibility;
- a campaign centered exclusively on the technically hosting player.

## Success criterion

STRE succeeds when two to ten players can begin together, understand who decides
what, experience cooperative systems without visible desynchronization, and
resume their campaign after reconnecting without manual intervention.
