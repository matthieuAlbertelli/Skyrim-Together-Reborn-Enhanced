# ADR-0026 - Remote seating through the existing STR action stream

- **Status:** Accepted
- **Date:** 2026-09-19
- **Decision makers:** STRE maintainer, explicit remote seating diagnostic mission
- **Supersedes:** ADR-0024 only for observer-side furniture activation
- **References:** #9, #23
- **Replay continuity extension:** [ADR-0027](ADR-0027-animation-replay-native-binding-continuity.md), after the old-binding consumption hypothesis was demonstrated at runtime; the diagnostic decision below is retained as its historical boundary.

## Evidence and decision

On Skyrim 1.6.1170, TESFurniture's native activation method compares the activator
pointer with the native PlayerCharacter singleton and returns false for a remote
Actor. The observed final remote binding is correct; invoking that primitive on
it is architecturally invalid. STATUS records the evidence and its limits.

CreationSeating must never call Activate on a remote actor, in any build.
Keep the real activation and the existing non-MASTER MarkerXX approach only for
the native local PlayerCharacter after its individual Applied/finalization.
Durable PlayerId rank, session/recovery/identity fences and local behavior remain.
There is no all-Applied barrier, remote MoveTo, invented furniture state, new
ActorState write, or ad hoc forced animation.

Observe the existing STR action stream before choosing a correction: native
capture, movement/action messages, observer animation queue, existing ForceAction,
and the actor bound when it executes. Observer diagnostics are non-MASTER,
bounded and read-only. The existing network animation implementation is retained;
its existing state/variable application is not duplicated or altered.

Final appearance and its official rematerialization are invariants. Do not modify
RemoteRespawnLab, publication, materialization, Discovery or commit fences for
this investigation. Query CommittedActor only from the existing seating update;
native callbacks use published value context, never saved engine pointers.

An action consumed by the old actor could be absent from the new actor's graph
despite preservation of the ECS queue. This remains a hypothesis until a real
received/dequeued/returned action trace identifies the actor and commit ordering.
No extra buffering or replay is authorized by that hypothesis alone. If proven,
prefer a bounded reuse of the existing action representation on the committed
binding, without fabricating furniture state or changing the appearance lifecycle.

## Validation boundary

Neutralization and instrumentation are not a remote sitting fix. Acceptance still
requires both observers to see the other's final appearance and visible sitting,
while both local players sit through Skyrim. No B-client logs are required by
this mission: use observer A and server logs, and state explicitly when source
emission versus relay cannot be distinguished with the available evidence.

Executed checks belong in STATUS; the observer procedure and decision table are
in TEST_PLAN. No commit/push before runtime acceptance.
