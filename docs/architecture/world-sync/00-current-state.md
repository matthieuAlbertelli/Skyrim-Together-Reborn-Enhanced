# World Sync — État actuel

- **Statut : Jalon 0 — audit et instrumentation**
- **Date : 2026-07-30**
- **Branche : `feature/world-sync-foundation`**

## Objectif

Documenter le flux réel des objets lâchés avant d’introduire une identité réseau persistante. Ce jalon n’altère pas le gameplay ni le protocole.

## Flux actuel d’un drop

```text
Skyrim local
  Actor::DropObject hook
    → InventoryChangeEvent(FormId acteur, Inventory::Entry, Drop=true)
      → client InventoryService::OnInventoryChangeEvent
        → RequestInventoryChanges(ServerId acteur, Item, Drop=true)
          → server InventoryService::OnInventoryChanges
            → mutation InventoryComponent de l’acteur
            → NotifyInventoryChanges(Drop=true) aux autres joueurs en portée
              → client distant InventoryService::OnNotifyInventoryChanges
                → Actor::DropOrPickUpObject
                  → Actor::DropObject
                    → Skyrim crée une référence temporaire locale
```

## Rupture d’identité

Le serveur synchronise une **mutation d’inventaire d’acteur**, pas une entité monde.

Chaque client distant appelle Skyrim localement pour créer le drop. Il en résulte des références temporaires indépendantes :

```text
Même BaseForm et même quantité
≠ même FormID local
≠ même entité ECS serveur
≠ identité réseau commune
```

Le joueur initiateur est exclu du broadcast parce que son Skyrim a déjà créé sa référence locale pendant l’action native.

## `ObjectService` actuel

`ObjectService` attribue des entités serveur aux références statiques recensées dans une cellule. Le code indique explicitement que ce modèle repose sur des objets statiques et n’est pas conçu pour les références temporaires.

Quand le dernier joueur quitte une cellule, les entités `ObjectComponent + CellIdComponent` sont détruites côté serveur. Cette règle devra être révisée avant toute persistance de références dynamiques.

## Messages trompeurs ou non centraux

`RequestObjectInventoryChanges` et `NotifyObjectInventoryChanges` existent dans le protocole, mais le flux de drop joueur observé utilise actuellement :

- `RequestInventoryChanges` ;
- `NotifyInventoryChanges`.

Le futur système ne doit donc pas être conçu à partir du nom des messages existants, mais à partir des cas d’usage et invariants du domaine.

## Instrumentation ajoutée par ce jalon

Préfixes :

```text
[STRE][WorldSync][LegacyInventory]
[STRE][WorldSync][LegacyDrop]
```

Transitions observables :

```text
request_send
request_receive
notify_broadcast
notify_apply
```

Ces traces permettent de rapprocher le client initiateur, le serveur et le client distant pour un même acteur, BaseForm et compte.
