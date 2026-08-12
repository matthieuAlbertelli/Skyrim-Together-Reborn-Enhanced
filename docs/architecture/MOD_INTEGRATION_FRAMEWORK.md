# STRE Mod Integration Framework

> **Status:** proposed architecture, with the first first-party pattern validated
> by Character Build.
> **Patterns:** Microkernel / Plugin Architecture, Ports and Adapters, Adapter,
> Observer/Event Bus, Command, Strategy, Anti-Corruption Layer.

## Goal

Let a single-player mod developer declare:

- which mod features have cooperative meaning;
- which state STRE must monitor;
- which actions are intents;
- who owns authority;
- which data is replicated;
- how to apply a canonical result;
- how to restore a player after reconnection.

The framework does not make every mod compatible automatically. It provides a
standard **interface-engineering** model for writing the adaptation.

## Vocabulary

### Capability

A named, versioned cooperative feature, for example:

- `campaign.start/1`
- `player.class-selection/1`
- `group.ready-check/1`
- `item-preview/1`

### Intent

A request produced by a player or local mod, such as `SelectClass(Paladin)`,
`SetReady(true)`, or `RequestDeparture`.

### Command

The validatable, routable form of an intent sent to the authority.

### Canonical State

Shared reference state that is versioned and serializable.

### Event

An accepted, ordered result, such as `PlayerClassChanged` or
`CampaignPhaseChanged`.

### Local Effect

A consequence applied in Skyrim: equip a set, play a scene, or open a door.

### Snapshot

The complete state required to join or resynchronize.

### Authority Policy

A strategy specifying who decides: server, campaign, owning player, vote, or
leader.

### Replication Policy

A strategy specifying who receives data: everyone, the group, the owner, or a
privileged role.

## Conceptual contract

```cpp
class IStreModAdapter
{
public:
    virtual AdapterDescriptor Describe() const noexcept = 0;
    virtual void RegisterCapabilities(CapabilityRegistry&) noexcept = 0;
    virtual void RegisterIntentHandlers(IntentRegistry&) noexcept = 0;
    virtual void RegisterStateSchemas(StateSchemaRegistry&) noexcept = 0;
    virtual AdapterSnapshot CaptureLocalSnapshot() noexcept = 0;
    virtual ApplyResult ApplyCanonicalSnapshot(const AdapterSnapshot&) noexcept = 0;
    virtual ApplyResult ApplyEvent(const AdapterEvent&) noexcept = 0;
};
```

This contract remains directional. The Character Build vertical slice validated
logical selections, canonical snapshots, hashing, and local application, but not
an adapter registry or public ABI. The exact ABI must not be frozen before a
second first-party integration and persistence/reconnection.

## Proposed runtime

### Adapter Registry

- stable identifier;
- adapter version;
- minimum STRE version;
- required Skyrim mod;
- dependencies;
- capabilities;
- active/incompatible status.

### Intent Router

- validates shape, size, and permission;
- associates player and campaign context;
- applies rate limits;
- sends an authoritative command.

### Canonical State Store

- state by campaign and capability;
- monotonic version;
- optional hash;
- serialization;
- snapshots;
- migrations.

### Event Dispatcher

- ordering by capability;
- idempotence;
- replay from snapshot/version;
- structured errors.

### Policy Engine

Initial policies:

- `ServerAuthoritative`
- `PlayerOwnedServerValidated`
- `SessionManagerAuthorized`
- `GroupVote`
- `DragonbornAuthorized`

### Compatibility Gate

Before activation:

- verify plugin and version;
- verify adapter and schema;
- compare hashes and configurations;
- reject cleanly or enter single-player mode according to policy.

## Recommended phases

### Phase 1 — First-party compile-time — in progress

C++ services compiled into STRE with dedicated messages. Character Build and
Alternate Start are the first delivered example: a shared catalog, canonical
inventory and spells, hashes, and local fallback. They do not yet provide the
generic registry described above.

### Phase 2 — Data-driven

YAML/JSON manifests, versioned schemas, and a more generic Papyrus bridge.

### Phase 3 — Experimental third-party SDK

Documented API, examples, manifest validation, and version compatibility.

### Phase 4 — Possible native ABI/plugin

Only after several adapters and stabilized requirements. Do not promise a binary
C++ ABI too early.

## Illustrative manifest

```yaml
adapter:
  id: stre.alternate-start
  version: 1.0.0
  requires:
    stre: '>=0.3.0'
    skyrim_plugin: STRE_AlternateStart.esp

capabilities:
  - id: campaign.start
    version: 1
    authority: server
  - id: player.class-selection
    version: 1
    authority: player-owned-server-validated

state:
  schema: alternate-start-state-v1
  snapshot: full
  events: incremental
```

## Security constraints

- no arbitrary execution received from the network;
- identified adapters and verified versions;
- bounded sizes;
- per-capability permissions;
- audience-filtered secret data;
- logs do not expose narrative secrets to unauthorized clients;
- the server does not trust Papyrus stages reported by a client.

## Current first-party evidence

Two clients can already create different builds, receive canonical inventory and
spells, and apply targeted buffs. This evidence covers build authority, but not
campaign state.

## MVP framework success definition

Two clients with Alternate Start:

1. load the same adapter;
2. join a campaign;
3. publish their ready state;
4. receive a canonical phase;
5. one client disconnects;
6. that client returns and applies a snapshot;
7. the scene does not play twice.
