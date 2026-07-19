# Carte du code audité

> **Statut : Implémenté**

## Domaine Trading

- `Code/common/Trade/TradeTypes.h`
- `Code/common/Trade/TradeSession.*`
- `Code/common/Trade/TradeInventory.*`
- `Code/common/Trade/TradeApplication.*`
- `Code/common/Trade/TradeReconciliation.*`

## Serveur

- `Code/server/Services/TradeService.*`
- enregistrement : `Code/server/World.cpp`

## Client

- `Code/client/Services/TradeService.h`
- `Code/client/Services/Generic/TradeService.cpp`
- `Code/client/Services/TradeMenuService.h`
- `Code/client/Services/Generic/TradeMenuService.cpp`
- enregistrement : `Code/client/World.cpp`

## Preview

- `Code/client/Services/TradeItemPreviewService.h`
- `Code/client/Services/Generic/TradeItemPreviewService.cpp`
- `Code/client/Services/ItemPreview/*`
- `Code/client/Games/Skyrim/Interface/Menus/TradePreviewHostMenu.*`

## CEF / UI

- `Code/client/Services/Generic/OverlayClient.cpp`
- `Code/client/Services/UiSurfaceService.h`
- `Code/client/Services/Generic/UiSurfaceService.cpp`
- `Code/skyrim_ui/src/app/components/trade-menu/*`
- `Code/skyrim_ui/src/app/services/trade-ui.service.ts`
- `Code/skyrim_ui/src/app/models/trade.ts`

## Protocole

- `Code/encoding/Messages/*Trade*`
- `Code/encoding/Structs/Trade*`
- factories client/serveur et opcodes associés.

## Tests

- `Code/tests/trade.cpp`
- `Code/tests/trade_inventory.cpp`
- `Code/tests/trade_application.cpp`
- `Code/tests/trade_reconciliation.cpp`
- `Code/tests/trade_messages.cpp`

## Documentation canonique après consolidation

- `README.md` et `ROADMAP.md` servent de points d’entrée publics ;
- `docs/features/trading/` documente la verticale Trading ;
- `docs/features/item-preview/` et `docs/architecture/ITEM_PREVIEW_PLATFORM.md` documentent la preview ;
- `docs/architecture/ADRs/ADR-0013-preview-refactor.md` conserve la décision historique de refactor ;
- `docs/README.md` est l’index documentaire canonique.

Les anciens résumés mono-fichier ont été supprimés afin d’éviter des sources concurrentes.
