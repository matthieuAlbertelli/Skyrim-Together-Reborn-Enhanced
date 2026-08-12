# Architecture Decision Records

> **Status:** canonical policy for durable STRE architecture decisions.

ADRs record decisions that change long-lived boundaries, authority, identity,
persistence, compatibility, or recovery semantics across more than one
implementation unit. Feature behavior remains in the feature's canonical
documentation, and live implementation work remains in GitHub issues.

## When to write an ADR

Use an ADR when a decision is expensive to reverse or constrains several future
changes, for example:

- server/client/CK authority boundaries;
- persistent identity, ownership, storage, migration, or reconnect semantics;
- network-wide reconciliation and failure-recovery policy;
- a shared integration or engine-safety boundary.

Do not write an ADR for a local implementation choice, an unaccepted idea, a
transient task plan, or merely to reconstruct every past change.

## Lifecycle

1. Copy [`ADR_TEMPLATE.md`](../../templates/ADR_TEMPLATE.md) and allocate the
   next unused four-digit number from the
   [decision register](../../production/DECISION_REGISTER.md).
2. Start as `Proposed` and link the decision's issue or discussion.
3. Change to `Accepted`, `Rejected`, or `Superseded` through review. `Accepted`
   means the decision is normative even if implementation is incomplete.
4. Do not silently rewrite an accepted decision. Create a later ADR and record
   `Supersedes` / `Superseded by` links.
5. Keep implementation progress out of the status field. A short implementation
   note may link to `STATUS.md` or an issue, but does not change the decision.

The [decision register](../../production/DECISION_REGISTER.md) is the only ADR
index. It records every allocated number and exposes gaps or collisions during
review.
