# STRE Roadmap — Road to v1.0.0

> **Status:** canonical product direction, release objectives and release gates.
> **Last updated:** 13 August 2026.

This document defines **where STRE is going and what a release must prove**. It does not track transient issue state, assignees, percentages or sprint progress.

- [`docs/project/STATUS.md`](docs/project/STATUS.md) is the source of truth for what is implemented and validated.
- The GitHub Project is the live operational tracker.
- GitHub Milestones assign actionable issues to target product versions.
- [`docs/production/GITHUB_GOVERNANCE.md`](docs/production/GITHUB_GOVERNANCE.md) defines the complete issue, Project, Milestone, PR, tag and release flow.

A release outcome is satisfied only when its user-visible behavior, failure cases and recovery behavior are demonstrable. Historical alpha accomplishments remain recorded in `STATUS.md` and [`CHANGELOG.md`](CHANGELOG.md), rather than copied into live planning lists here.

## STRE v1.0.0 product definition

STRE v1.0.0 is the first version suitable for starting and playing a real cooperative STRE campaign. Release requires all of the following outcomes:

1. Complete Alternate Start.
2. Complete character creation flow.
3. All 21 character classes in the canonical v1 roster defined by [`CLASS_ROSTER_V1.md`](docs/features/alternate-start/CLASS_ROSTER_V1.md).
4. Starting equipment/loadouts for every class.
5. Cooperative class abilities and perks.
6. Personal quest content for every class.
7. Messire Valen fully implemented.
8. The headquarters inn fully implemented.
9. Ten player rooms in the inn.
10. Persistent player-to-room assignment as the housing foundation.
11. Cooperative campaign continuity with a sealed fixed roster, full-roster
    progression, and coordinated recovery from one committed checkpoint shared
    by the server revision and every roster member's native Skyrim save.
12. No known P0 multiplayer campaign blocker.

Advanced housing customization can ship after v1.0.0. The v1.0 requirement is the housing foundation: rooms, ownership/assignment and persistence.

Where Alternate Start explicitly provides a standalone path, that path must remain usable without an STRE server. Multiplayer authority must enrich the cooperative session without making the intended solo content unusable.

## Road-to-1.0 workstreams

The exact allocation of these outcomes to intermediate `0.x` releases remains provisional until a coherent, achievable slice is ready for a Milestone. The Project owns live ordering and progress.

### Alternate Start and character creation

Deliver a clean new-game path through the custom inn, exhaustive vanilla-start/Helgen bypass, RaceMenu and STRE character creation, complete reset policy, canonical build application and a coherent departure into the campaign. Preserve the explicit local fallback for solo play.

The class program covers all 21 classes in the canonical v1 roster, all starting kits and schools, cooperative abilities/perks, balance/combination validation and a personal quest for each class. Logical catalog, CK records, Papyrus, UI and authoritative multiplayer application must remain one coherent contract.

### Messire Valen, headquarters inn and housing foundation

Complete Messire Valen's narrative, visual, audio, CK and gameplay implementation. Complete the headquarters inn as the campaign hub, including ten usable player rooms.

Introduce persistent, unambiguous player-to-room ownership/assignment with reconnect and campaign restoration behavior. Decoration and advanced housing customization are explicitly post-1.0 unless needed to validate the foundation.

### Cooperative campaign continuity

Provide the campaign-state foundations required for a real group campaign:
durable character binding, versioned server persistence, canonical snapshots,
shared introduction/departure phases, and a roster configured in the pre-campaign
lobby. Formally starting/committing the campaign seals its slots, `PlayerId`
values, and `CharacterBinding` identities before `CharacterCreation` or any other
campaign progression; `Departure` and `OpenWorld` are not seal points. Campaign
progression requires the complete roster; campaign late join, slot-owner
replacement, and continue-without-player behavior are not supported in v1.

Coordinate each committed `CampaignCheckpoint` across one canonical server
revision/snapshot and one dedicated native Skyrim save for every roster slot. The
server remains authority for shared STRE state; each `.ess` restores its player's
local Skyrim/Papyrus/quest runtime. A disconnect suspends campaign progression,
and every roster member collectively restores the same last committed checkpoint
before the campaign resumes. A failed candidate checkpoint never replaces the
previous committed recovery point.

Validate the primary two-to-four-player campaign path before broader hub/event scale. Recovery must be designed for duplicated, delayed or missing messages instead of assuming a continuous session.

### Existing system stabilization

Continue hardening the first-party verticals already described in `STATUS.md`:

- **Trading:** integration/recovery coverage, active-session reconnect policy, stack/gold behavior, instance-metadata-safe transfer and player-facing recovery.
- **World Sync:** scripted/quest-owned reference validation, custom-name metadata, durable world persistence and deliberate extension to additional world object classes.
- **Item Preview:** lease/owner arbitration, lifecycle/concurrency coverage and a stable internal request contract before any third-party API promise.
- **Release engineering:** clean-machine prerequisites, broader native/UI CI, one canonical automated test entry point and a release-grade supported-version matrix.

These scopes remain visible product/engineering direction. Their current implementation state belongs only in `STATUS.md`; their actionable units and ordering belong in GitHub.

### Mod-integration platform

The experimental adapter SDK remains a deliberate later track: versioned envelopes, manifests and negotiation, permissions/sandboxing, Papyrus/C++ examples and compatibility/deprecation rules. Its public contract is not frozen until additional first-party integrations validate the abstraction.

Downed/recovery states, persistent consequences, group votes/shared decisions and other cooperative systems may be scheduled when they support the campaign without displacing the v1 gate.

## Permanent STR stabilization track

STRE permanently audits inherited Skyrim Together Reborn behavior that can compromise a campaign. Track observable symptoms first, reproduce them across player perspectives, then decide whether work belongs in STRE, upstream STR, or both.

The architectural families below are investigation lenses, not diagnoses or preselected solutions. Evidence may reveal that several symptoms share ownership, actor-lifecycle, authoritative-world-state, physical-reconciliation or presentation-replication causes.

| Symptom | Observable requirement and release concern | Possible family to audit |
|---|---|---|
| Mount ownership | Two players can mount the same horse and break its behavior. If a horse is already occupied by a player, another player must not be able to mount it. | Ownership; authoritative interaction state |
| Mounted animation synchronization | A horse ridden by one player can appear to animate incorrectly to another player. Reproduction scope and gameplay impact must be investigated before final severity is assigned. | Presentation replication; physical reconciliation |
| Dungeon world-state divergence | Door, lever or secret-passage mechanisms can present different states to different players and block progression. A reproduced case that makes campaign progression impossible meets the P0 definition. | Authoritative world state; interaction ownership |
| Actor spawn synchronization | Wilderness monsters can exist for one player while remaining absent for others. Relevant players must observe a coherent actor lifecycle. | Actor lifecycle; authoritative world state |
| Corpse transform/interaction divergence | Players can see the same corpse at different locations, and some cannot loot it. The shared corpse must converge to an interactable state without losing legitimate loot access. | Actor lifecycle; physical reconciliation; interaction authority |

Each symptom becomes an issue with exact roles, saves/logs and reproduction evidence. Parent/sub-issue grouping follows the audit; do not implement one-off network patches merely because symptoms look similar.

## Release sequence and gates

### `0.x` alpha releases

Intermediate alphas integrate demonstrable vertical slices and reduce risk toward the v1 product definition. A `0.x` Milestone is created only when its scope can be justified from dependency order and available validation capacity; version assignments must not be fabricated for planning neatness.

### v1.0 release candidate

A release candidate may be produced when the complete v1 product definition is feature-complete and the repository has evidence for:

- end-to-end Alternate Start, character creation, class, personal-quest, Valen, inn and room-assignment flows;
- solo validation for every explicitly standalone Alternate Start path;
- multiplayer validation across host, client and observer perspectives for campaign-critical flows;
- persistence and coordinated-checkpoint evidence, including partial candidate
  failure and server/client restart from the last committed checkpoint;
- missing-player activation, post-seal late-join, replacement, wrong-slot, wrong
  binding, and wrong-save rejection;
- disconnect barriers and collective restore of the full roster before campaign
  progression resumes;
- triage of known multiplayer divergence, crash, corruption and progression reports;
- the supported-version matrix, installation guide and reproducible player package;
- relevant automated tests, diagnostics and canonical documentation.

### v1.0.0 release

The `v1.0.0` release is cut only when the release-candidate evidence satisfies every product outcome above and no known `priority: P0` issue remains open. The accepted source receives the immutable `stre-v1.0.0` tag; the GitHub Release distributes the player artifact and release notes. The `stre-v` namespace distinguishes STRE releases from inherited upstream tags; mechanics are defined in [`RELEASE_PROCESS.md`](docs/production/RELEASE_PROCESS.md).

The WBS describes structural decomposition without progress state: see [Work Breakdown Structure](docs/production/WORK_BREAKDOWN_STRUCTURE.md).
