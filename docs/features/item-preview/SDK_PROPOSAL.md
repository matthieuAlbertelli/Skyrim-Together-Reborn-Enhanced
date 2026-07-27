# Item Preview — Proposition de SDK

> **Statut : Proposition ; démonstrateur Character Creation réalisé sans SDK public**

## Preuve actuelle

Character Creation affiche déjà les équipements de départ via le même cœur natif que Trading. Cette preuve valide la réutilisation interne, mais utilise encore les services first-party et le bridge mono-client.

## Capability cible

`stre.item-preview/1`

## Requête logique

```json
{
  "owner": "stre.alternate-start.class-menu",
  "item": { "mod": 1, "base": 12345 },
  "region": { "left": 0.62, "top": 0.18, "width": 0.31, "height": 0.58 },
  "lightScheme": 1,
  "priority": "standard"
}
```

## Cycle de vie cible

1. `AcquirePreview` → token ;
2. `UpdatePreview` → item/région ;
3. `PreviewSuspended` si préempté ;
4. `PreviewResumed` ;
5. `ReleasePreview` ;
6. release automatique à la destruction ou timeout.

## Permissions

- capability/version compatibles ;
- item résolu localement ;
- surface UI autorisée ;
- rate limit ;
- aucun accès à un pointeur ou une texture native.

## Erreurs

- `UNSUPPORTED`
- `RESOURCE_BUSY`
- `INVALID_ITEM`
- `INVALID_REGION`
- `OWNER_NOT_ACTIVE`
- `NATIVE_MANAGER_UNAVAILABLE`
- `HOST_MENU_FAILURE`
- `DEVICE_FAILURE`

## Prochain démonstrateur

Faire fonctionner Trading et Character Creation avec de vrais leases, puis provoquer et vérifier une préemption contrôlée. Ce test doit précéder toute annonce de SDK tiers.
