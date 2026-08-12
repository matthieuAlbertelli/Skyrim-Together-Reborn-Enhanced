# Carte du code audité

> **Statut : Snapshot historique au 27 juillet 2026 — non canonique pour l'état
> courant.** Voir [`docs/project/STATUS.md`](../../project/STATUS.md).

## Domaine Trading

- `Code/common/Trade/TradeTypes.h`
- `Code/common/Trade/TradeSession.*`
- `Code/common/Trade/TradeInventory.*`
- `Code/common/Trade/TradeApplication.*`
- `Code/common/Trade/TradeReconciliation.*`

## Trading serveur et client

- `Code/server/Services/TradeService.*`
- `Code/client/Services/TradeService.h`
- `Code/client/Services/Generic/TradeService.cpp`
- `Code/client/Services/TradeMenuService.h`
- `Code/client/Services/Generic/TradeMenuService.cpp`

## Alternate Start / Character Build

### Catalogue partagé

- `Code/common/CharacterCreation/CharacterBuildCatalog.h`
- `Code/common/CharacterCreation/CharacterBuildCatalog.cpp`

Le catalogue courant utilise `BuildVersion = 5` et construit :

- `ItemGrant` ;
- `EquipmentGrant` ;
- `SpellGrant`.

### Client

- `Code/client/Services/CharacterCreationService.h`
- `Code/client/Services/Generic/CharacterCreationService.cpp`
- `Code/client/Games/Skyrim/Forms/MagicItem.cpp`

### Serveur

- `Code/server/Services/CharacterBuildService.h`
- `Code/server/Services/CharacterBuildService.cpp`
- `Code/server/Components/CharacterBuildComponent.h`

### Protocole

- `Code/encoding/Messages/CharacterBuildRequest.*`
- `Code/encoding/Messages/CharacterBuildResponse.*`
- `Code/encoding/Messages/CharacterBuildAppliedRequest.*`
- `Code/encoding/Messages/NotifyCharacterBuildState.*`
- `Code/encoding/Structs/CharacterBuild.*`

### UI

- `Code/skyrim_ui/src/app/data/character-loadouts.ts`
- composants et services Character Creation sous `Code/skyrim_ui/src/app/`.

### Creation Kit

- `GameFiles/Skyrim/STRE_AlternateStart.esp`
- `GameFiles/Skyrim/Source/Scripts/QF_STRE_QUEST_AlternateStart_02001AF9.psc`
- `GameFiles/Skyrim/Scripts/QF_STRE_QUEST_AlternateStart_02001AF9.pex`
- `GameFiles/STRE_AlternateStart.manifest.txt`

### Audits et tests

- `Code/tests/character_build.cpp`
- `Tools/Scripts/audit_stre_plugin_records.py`
- `Tools/Scripts/audit_character_build_catalog.py`
- `docs/features/alternate-start/CK_RECORDS_M7_IMPLEMENTED.json`

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

## Tests Trading

- `Code/tests/trade.cpp`
- `Code/tests/trade_inventory.cpp`
- `Code/tests/trade_application.cpp`
- `Code/tests/trade_reconciliation.cpp`
- `Code/tests/trade_messages.cpp`

## Documentation canonique

- `README.md`, `CHANGELOG.md` et `ROADMAP.md` servent de points d’entrée publics ;
- `docs/features/trading/` documente Trading ;
- `docs/features/item-preview/` documente la preview ;
- `docs/features/alternate-start/` documente Alternate Start et Character Build ;
- `docs/audit/CURRENT_STATE_AUDIT.md` décrit l’état courant ;
- `docs/README.md` est l’index documentaire canonique.
