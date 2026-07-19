# Alternate Start — State Model

> **Statut : Proposition**

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

## Invariants

- un slot d’arrivée par joueur actif ;
- un personnage validé par joueur/campagne ;
- aucune classe après départ sans migration explicite ;
- `DepartureAuthorized` implique introduction terminée et règles ready satisfaites ;
- version monotone ;
- événements anciens ignorés ;
- les secrets Dragonborn restent hors de cet état public.
