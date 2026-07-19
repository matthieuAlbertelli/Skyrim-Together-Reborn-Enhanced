# Item Preview — Proposition de SDK

> **Statut : Proposition**

## Capability

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

Les coordonnées publiques doivent être normalisées ou clairement définies en pixels CSS + device scale.

## Cycle de vie

1. `AcquirePreview` → token ;
2. `UpdatePreview` → item/région ;
3. `PreviewSuspended` si préempté ;
4. `PreviewResumed` si ressource récupérée ;
5. `ReleasePreview` ;
6. release automatique à la destruction ou timeout.

## Permissions

- capability présente et version compatible ;
- item résolu localement ;
- surface UI autorisée ;
- rate limit sur changements d’item/région ;
- pas d’accès aux pointeurs ou textures natives.

## Erreurs

- `UNSUPPORTED`
- `RESOURCE_BUSY`
- `INVALID_ITEM`
- `INVALID_REGION`
- `OWNER_NOT_ACTIVE`
- `NATIVE_MANAGER_UNAVAILABLE`
- `HOST_MENU_FAILURE`
- `DEVICE_FAILURE`

## Démonstrateur requis

Une fonctionnalité non Trading — par exemple le choix de classe — affiche l’équipement de départ via la même API sans inclure de header spécifique au trading.
