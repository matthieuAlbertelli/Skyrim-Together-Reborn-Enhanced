# Technical risk register

> **Status: Historical snapshot as of July 27, 2026; non-canonical for active
> risks.** See [`docs/production/RISK_REGISTER.md`](../../production/RISK_REGISTER.md).

| ID | Risk | Probability | Impact | State / recommended mitigation |
|---|---|---:|---:|---|
| R-01 | The single-client preview bridge blocks real concurrent use | High | High | A second first-party consumer was validated, but a lease manager is still required |
| R-02 | Reuse of `toggleDebugUI` as a production channel | High | Medium | Dedicated typed CEF API |
| R-03 | Substantial divergence from upstream becomes difficult to rebase | Medium | High | Patch register, ADRs, and frequent merges |
| R-04 | Trading state exists only in memory during a server crash | Medium | High | Transaction journal or explicit abandonment policy |
| R-05 | Distributed inventory mutation is not strictly atomic | Medium | High | Document saga and reconciliation model; add failure tests |
| R-06 | Protocol versioning is absent for future dynamic adapters | High | High | Versioned envelope before a third-party SDK |
| R-07 | Papyrus/CK become an implicit source of truth | Medium | High | Retain local intent and canonical server state |
| R-08 | Helgen skip leaves vanilla quests inconsistent | High | High | Stage/global matrix and resumption tests |
| R-09 | External or cheated characters join a campaign | Medium | High | Cleanup and canonical build implemented; persistent binding still required |
| R-10 | Valen/scene assumes a single `Game.GetPlayer()` | High | Medium | Local aliases plus STRE coordination |
| R-11 | The inn becomes saturated with 10 players | Medium | Medium | Explicit markers, navmesh, and circulation tests |
| R-12 | Asset/voice licenses are insufficiently explicit | Medium | High | Contributor agreement and provenance record |
| R-13 | French documentation limits international contributors | Medium | Medium | Public canonical English; working French or translation |
| R-14 | Source export excludes source directories named Debug | Observed | Low | Correct the script filter |
| R-15 | Character build is lost after reconnect or server restart | High | High | Versioned persistence plus idempotent restoration |
| R-16 | Remote buffs depend on a name-based allowlist of local FormIDs | Medium | Medium | Keyword/capability classification before extending to many mods |
| R-17 | Incomplete cleanup of historical skills, perks, and statistics | High | High | Controlled reset milestone, before/after tests, and explicit policy |
| R-18 | `Script::CompileAndRun` depends on a runtime Address Library ID | Medium | High | Retain 1.6.1170 validation, documented fallback, and explicit logs |

## Priority risk-reduction decisions

- Trading is a **compensated saga**, not a distributed ACID transaction.
- The canonical build reduces cheated-import risk but is not yet an exhaustive character reset.
- Build persistence must precede the complete Campaign State.
- The mod SDK should begin with compiled first-party integrations to avoid freezing an ABI too early.
- Preview must become an arbitrated resource before it is advertised as a third-party API.
- Cooperative-spell classification must become extensible before adding many custom spells.
