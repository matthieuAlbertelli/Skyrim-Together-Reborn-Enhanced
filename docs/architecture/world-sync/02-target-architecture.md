# World Sync — Architecture cible

- **Statut : Proposed**
- **Date : 2026-07-30**

## Principe

Construire un noyau unique de synchronisation d’entités monde. Les objets lâchés constituent la première implémentation verticale ; les cadavres et conteneurs réutiliseront les mêmes mécanismes.

## Couches

```text
Domain
  identité, état, révisions, invariants, policies

Application
  commandes et cas d’usage

Infrastructure
  stockage, journal, outbox, réseau

Adapters
  Skyrim, services STRE existants, sauvegarde hôte
```

Le domaine ne dépend ni des FormID locaux, ni de Skyrim, ni d’SQLite.

## Modèle minimal futur

```cpp
using WorldEntityId = uint64_t;
using Revision = uint64_t;

enum class WorldEntityKind : uint8_t
{
    DroppedItem,
    Corpse,
    Container,
    DynamicReference
};

struct WorldEntity
{
    WorldEntityId Id;
    WorldEntityKind Kind;
    GameId BaseForm;
    CellIdComponent Location;
    Revision RevisionNumber;
    EntityLifecycleState Lifecycle;
};
```

Les FormID locaux appartiennent à un registre client séparé :

```text
WorldEntityId ↔ Local FormID ↔ BindingGeneration
```

## Pipeline d’une commande

```text
Decode
→ authenticate
→ deduplicate
→ validate
→ check expected revision
→ apply domain transition
→ persist state + journal + outbox atomically
→ commit
→ replicate result
```

## Persistance

Architecture retenue :

```text
état courant normalisé
+ journal transactionnel append-only
+ checkpoints liés aux sauvegardes .ess de l’hôte
```

Il ne s’agit pas d’un Event Sourcing intégral. L’état courant reste directement lisible et le journal sert à la reprise, à l’audit et aux mutations postérieures au dernier checkpoint.

## Sauvegarde hôte

```text
SaveWillBegin
→ terminer les mutations déjà admises
→ réconcilier les références hôte matérialisables
→ préparer checkpoint STRE
→ Skyrim écrit le .ess
→ SaveCompleted(save identity)
→ valider checkpoint STRE
```

En cas d’échec, le dernier checkpoint validé reste la référence.
