# Alternate Start — State model

> **Status: Authoritative build implemented; campaign state proposed**

## Currently implemented state

`CharacterBuildSnapshotData` represents a player's canonical build:

```cpp
struct CharacterBuildSnapshotData
{
    uint32_t BuildVersion;
    GameId RaceId;
    String ClassId;
    Vector<CharacterBuildSelectionData> Selections;
    Inventory CanonicalInventory;
    uint64_t InventoryHash;
    Vector<GameId> CanonicalSpells;
    uint64_t SpellHash;
};
```

Network state:

```cpp
enum class CharacterBuildNetworkState : uint8_t
{
    Accepted = 1,
    Applied = 2
};
```

Current invariants:

- `BuildVersion = 5`;
- the catalog validates classes and options;
- the server derives inventory and spells;
- local plugin/FormID resolution does not use a load-order prefix;
- a build becomes `Applied` only after both hashes are validated;
- once applied, the build cannot be replaced during the session;
- no durable persistence currently exists.

## Target campaign state

```cpp
struct AlternateStartState
{
    StateVersion Version;
    AlternateStartPhase Phase;
    bool IntroductionStarted;
    bool IntroductionCompleted;
    bool DepartureAuthorized;
    std::vector<PlayerBootstrapState> Players;
};
```

```cpp
struct PlayerBootstrapState
{
    PlayerId Player;
    CharacterBindingState Binding;
    bool CharacterCreated;
    std::optional<ClassId> Class;
    bool Ready;
    bool LocalIntroductionComplete;
    ArrivalSlot Arrival;
};
```

Future invariants:

- one arrival slot per active player;
- one validated character per player and campaign;
- no class changes after departure without an explicit migration;
- `DepartureAuthorized` requires a completed introduction and satisfied ready rules;
- monotonically increasing version;
- stale events are ignored;
- Dragonborn secrets are absent from public state;
- snapshots are persisted and restorable after reconnecting.
