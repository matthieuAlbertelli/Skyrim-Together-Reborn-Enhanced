# Architecture de la plateforme Item Preview

> **Statut : Cœur réutilisé par deux consommateurs / runtime à leases proposé**

## État actuel

Le code possède des composants génériques solides :

- session native ;
- contrôleur ;
- solver ;
- mesure raster ;
- host session ;
- host bridge RAII.

Deux consommateurs first-party sont actifs :

- `TradeItemPreviewService` pour les échanges ;
- Character Creation pour les équipements de classe et de compétence.

Chaque consommateur résout un objet Skyrim réel, fournit une région Angular/CEF et utilise le même pipeline de chargement, mesure et cadrage.

## Problème restant

`ItemPreviewHostBridge` n’accepte toujours qu’un `ItemPreviewHostClient` à la fois. La réutilisation séquentielle est validée, mais le runtime ne gère ni concurrence, ni priorité, ni ownership explicite, ni timeout de ressource.

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
- priorité explicite ;
- préemption notifiée ;
- fermeture idempotente ;
- restitution de la surface et de l’input ;
- aucun consommateur ne manipule directement le host menu.

## Séparation cible

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

Trading et Character Creation deviennent des adapters consommateurs du runtime.

## API pour mods tiers

1. stabiliser les leases entre les consommateurs first-party ;
2. ajouter une capability interne `item-preview/1` ;
3. fournir un bridge UI/Mod Adapter par `GameId` canonique ;
4. publier un SDK tiers versionné.

Un mod externe ne doit jamais recevoir de pointeur Skyrim natif ni appeler `Inventory3DManager` directement.

## Tests requis

- solver pur : tailles extrêmes, bords écran, clamps ;
- acquisition/release idempotente ;
- préemption Trading ↔ Character Creation ;
- fermeture pendant reload ;
- changement rapide d’item ;
- resize de région ;
- perte du device ou absence du manager ;
- conflit avec menus natifs.
