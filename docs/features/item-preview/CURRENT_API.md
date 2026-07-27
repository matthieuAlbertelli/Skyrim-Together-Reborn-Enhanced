# Item Preview — API interne actuelle

> **Statut : Implémenté pour Trading et Character Creation, non stable pour tiers**

## Consommateurs

### Trading

`TradeItemPreviewService` résout un `Trade::ItemId`, construit l’entrée native et pilote la preview.

### Character Creation

`CharacterCreationService` associe des `previewKey` à des formulaires vanilla ou `STRE_AlternateStart.esp`, puis transmet la sélection et la région au pipeline de preview.

## Cœur générique

### `ItemPreviewController`

Conserve sélection, région, révisions, fitting, reload et état actif.

### `ItemPreviewNativeSession`

Encapsule `Inventory3DManager::Begin3D`, load/restart/clear et `End3D`.

### `ItemPreviewHostSession`

Automate atomique de show/hide qui absorbe les messages concurrents.

### `ItemPreviewHostBridge`

Singleton thread-safe auquel un seul `ItemPreviewHostClient` peut être lié. `ItemPreviewHostBinding` gère bind/unbind en RAII.

### `ItemPreviewFitSolver`

Fonction pure calculant position et échelle à partir des bounds raster.

### `ItemPreviewRasterMeasurer`

Capture D3D11 avant/après et mesure le modèle dans la région cible.

## Ce que cette API permet déjà

- partager le pipeline entre deux fonctionnalités first-party ;
- afficher des objets Skyrim réels dans une région Angular ;
- recalculer le cadrage après resize ou changement rapide ;
- tester le solver indépendamment ;
- isoler le host menu du consommateur.

## Ce qu’elle ne garantit pas

- coexistence concurrente de plusieurs consommateurs ;
- leases, ownership ou priorité ;
- ABI stable ;
- appel depuis Papyrus ou un mod externe ;
- compatibilité inter-version ;
- sécurité d’un payload réseau tiers ;
- preview 3D appropriée pour tous les `MagicItem`.

La communication publique doit employer **« fondation d’API interne réutilisable »**, pas **« SDK de mods tiers »**.
