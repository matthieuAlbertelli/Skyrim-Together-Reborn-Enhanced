# ADR-0027 - Animation replay continuity across a committed native binding

- **Status:** Accepted
- **Date:** 2026-09-19
- **Decision makers:** STRE maintainer, explicit replay continuity mission
- **References:** #9, #23
- **Extends:** ADR-0026 following runtime evidence; appearance authority and fences remain unchanged

## Evidence

Observer A's 02:12 run demonstrates IdleChairRightEnter successfully consumed on
the old actor before final candidate-commit. The same ECS animation queue survives
the binding replacement; its remaining movement/turn actions reach the new actor,
but the consumed chair entry does not. STATUS owns exact evidence and limits.

## Decision

Treat final candidate-commit as replacement of the native representation of the
same entity. The materialization layer only announces the committed old/new FormID
and token, session, generation, server ID and versioned entity to AnimationSystem.
This notification occurs after binding publication and the candidate-commit log;
it neither calls the engine nor changes admission, transaction, retirement or
Discovery fences. All replay responsibility stays in the animation layer.

Share STR's ActionReplayCache and AnimationEventLists between client and server
in the encoding library. Keep the existing 32-action limit, ignore list, exit
classification, instant counterparts and ActionReplayChain representation. No
protocol/schema change, chair/seat/FormID special case or new native primitive.

Each RemoteAnimationComponent keeps a value-owned cache of consumed actions.
Pending actions are outside this cache until consumed. At the announced binding
replacement, refine the consumed cache using the same server rules and **prepend**
the resulting chain to the live queue. Preserve the queue's source order, equal
ticks and differential baseline; do not append reconstruction after newer actions,
reinsert pending actions, sort by tick, or reset the logical entity as a new spawn.
Injected historical actions are not recorded a second time on consumption.

A received exit in the pending suffix invalidates older cached history, even if
the exit has not run yet. Keep all live pending actions in their original order.
An exit arriving while injected history waits for a graph similarly cancels only
the synthetic prefix. Consumed exits use the existing cache reset/drop rules.
This prevents transient resurrection of an invalidated entry. Pending entries
continue through the normal action path once; only consumed history is refined.

The existing AnimationSystem tick/readiness/reset/ForceAction/state-variable
application path executes reconstruction on the new actor. No direct ForceAction
outside that path, remote Activate, remote MoveTo, forced graph event, or added
ActorState assignment. Reconstruction is idempotent per session/generation and
binding; compare fresh actor identity at update and discard history on unexpected
binding changes. Do not retain borrowed engine pointers.

Disconnect, a new connection and observed recovery lock invalidate history and
remove only its injected prefix. Invalidation disables historical capture until
normal Setup creates a fresh animation component; ordinary STR pending actions
are not cleared by this feature. New-session history cannot reuse an old context.

The functional replay has the same MASTER/non-MASTER scope as official final
rematerialization. This does not promote the local non-MASTER seating prototype.
Diagnostics have their own bounds: at most 32 source entries, 32 injected entries,
32 execution returns and one graph-wait line per reconstruction, independent of
native diagnostic spam. Native success is not visual acceptance.

## Validation and limits

Pure tests cover refinement parity, pending/exits, causal ordering, idempotence,
bounded history, invalidation and alias rejection. Structural checks enforce the
single post-commit notification and existing native action path. Build/test results
belong in STATUS; the reciprocal roster-2 procedure belongs in TEST_PLAN.

Human acceptance requires correct final appearance and local/remote visible sitting
in both directions, in both completion orders. No commit/push before acceptance.
Native lifetime, broader animation serialization and seating beyond this inherited
action pipeline are not investigated here.
