# Alternate Start — State Model

> **Statut : Build autoritaire implémenté / état de campagne proposé**

## État actuellement implémenté

`CharacterBuildSnapshotData` représente le build canonique d’un joueur :

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

État réseau :

```cpp
enum class CharacterBuildNetworkState : uint8_t
{
    Accepted = 1,
    Applied = 2
};
```

Invariants actuels :

- `BuildVersion = 5` ;
- classe et options validées par le catalogue ;
- inventaire et sorts dérivés côté serveur ;
- plugin/FormID local résolu sans préfixe de load order ;
- build `Applied` uniquement après validation des deux hashes ;
- une fois appliqué, le build ne peut plus être remplacé pendant la session ;
- pas de persistance durable actuelle.

## État de campagne cible

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

Invariants futurs :

- un slot d’arrivée par joueur actif ;
- un personnage validé par joueur/campagne ;
- aucune classe après départ sans migration explicite ;
- `DepartureAuthorized` implique introduction terminée et règles ready satisfaites ;
- version monotone ;
- événements anciens ignorés ;
- secrets Dragonborn absents de l’état public ;
- snapshot persisté et restaurable après reconnexion.
