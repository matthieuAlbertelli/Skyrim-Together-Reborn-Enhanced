# Item Preview — API interne actuelle

> **Statut : Implémenté, non stable pour tiers**

## Entrée consommateur

`TradeItemPreviewService` expose :

- `SelectItem(Trade::ItemId)` ;
- `Clear()` ;
- `SetPreviewRegion(left, top, width, height)` ;
- capture/soumission de mesures raster ;
- cycle de vie de session native et host menu.

## Cœur générique

### `ItemPreviewController`

Conserve sélection, région, révisions, fitting, reload et état actif.

### `ItemPreviewNativeSession`

Encapsule `Inventory3DManager::Begin3D`, load/restart/clear et `End3D`.

### `ItemPreviewHostSession`

Automate atomique de show/hide qui absorbe les messages concurrents.

### `ItemPreviewHostBridge`

Singleton thread-safe auquel un seul `ItemPreviewHostClient` peut être lié. `ItemPreviewHostBinding` gère le bind/unbind en RAII.

### `ItemPreviewFitSolver`

Fonction pure calculant position et échelle à partir des bounds raster.

### `ItemPreviewRasterMeasurer`

Capture D3D11 avant/après et mesure le modèle dans la région cible.

## Ce que cette API permet déjà

- extraire la preview du service de trading ;
- réutiliser le contrôleur depuis un autre service C++ interne ;
- tester le solver indépendamment ;
- isoler le host menu du consommateur.

## Ce qu’elle ne garantit pas

- coexistence de plusieurs consommateurs ;
- ABI stable ;
- appel depuis Papyrus ou un mod externe ;
- ownership et priorité ;
- compatibilité inter-version ;
- sécurité d’un payload venant du réseau.

La communication publique doit donc employer : **« fondation d’API réutilisable »**, pas encore **« SDK de mods tiers »**.
