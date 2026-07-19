# Architecture cible de la plateforme Item Preview

> **Statut : Refactor partiellement implémenté / évolution proposée**

## État actuel

Le code possède déjà des composants génériques solides :

- session native ;
- contrôleur ;
- solver ;
- mesure raster ;
- host session ;
- host bridge RAII.

Le consommateur `TradeItemPreviewService` traduit un `Trade::ItemId` vers un `TESBoundObject`, crée une `InventoryEntry` puis pilote le contrôleur.

## Problème restant

`ItemPreviewHostBridge` n’accepte qu’un `ItemPreviewHostClient`. Il ne gère ni plusieurs consommateurs, ni priorité, ni ownership explicite, ni timeout de ressource.

## API cible

```cpp
struct ItemPreviewRequest
{
    PreviewOwnerId Owner;
    GameId Item;
    PreviewRegion Region;
    LightScheme Light;
    PreviewPriority Priority;
};

class ItemPreviewLease
{
public:
    void Update(const ItemPreviewRequest&) noexcept;
    void Release() noexcept;
};

class ItemPreviewRuntime
{
public:
    PreviewAcquireResult Acquire(const ItemPreviewRequest&) noexcept;
};
```

## Arbitrage

- une seule session native active ;
- priorité explicite : menu système > interaction critique > fonctionnalité standard > preview passive ;
- préemption notifiée au propriétaire ;
- fermeture idempotente ;
- restitution de la surface et de l’input ;
- aucun consommateur ne manipule directement le host menu.

## Séparation recommandée

```text
ItemPreviewRuntime
├─ LeaseManager
├─ ItemResolver
├─ Controller
├─ NativeSession
├─ HostMenuAdapter
├─ RasterMeasurementAdapter
└─ Telemetry
```

`TradeItemPreviewService` devient un adapter consommateur, non le propriétaire du host.

## API pour mods tiers

Étape 1 : capability STRE interne `item-preview/1`.  
Étape 2 : bridge UI/Mod Adapter demandant une preview par GameId canonique.  
Étape 3 : SDK tiers avec permissions et versionnement.

Un mod externe ne doit pas recevoir des pointeurs Skyrim natifs ni appeler `Inventory3DManager` directement.

## Tests requis

- solver pur : tailles extrêmes, bords écran, clamps ;
- acquisition/release idempotente ;
- préemption ;
- fermeture pendant reload ;
- changement rapide d’item ;
- resize de région ;
- perte du device ou absence du manager ;
- conflit avec menus natifs.
