# Audited source map

> **Status: Historical snapshot as of July 27, 2026; non-canonical for current
> state.** See [`docs/project/STATUS.md`](../../project/STATUS.md).

## Trading domain

- `Code/common/Trade/TradeTypes.h`
- `Code/common/Trade/TradeSession.*`
- `Code/common/Trade/TradeInventory.*`
- `Code/common/Trade/TradeApplication.*`
- `Code/common/Trade/TradeReconciliation.*`

## Trading server and client

- `Code/server/Services/TradeService.*`
- `Code/client/Services/TradeService.h`
- `Code/client/Services/Generic/TradeService.cpp`
- `Code/client/Services/TradeMenuService.h`
- `Code/client/Services/Generic/TradeMenuService.cpp`

## Alternate Start / Character Build

### Shared catalog

- `Code/common/CharacterCreation/CharacterBuildCatalog.h`
- `Code/common/CharacterCreation/CharacterBuildCatalog.cpp`

The catalog at the snapshot date used `BuildVersion = 5` and constructed:

- `ItemGrant`;
- `EquipmentGrant`;
- `SpellGrant`.

### Client

- `Code/client/Services/CharacterCreationService.h`
- `Code/client/Services/Generic/CharacterCreationService.cpp`
- `Code/client/Games/Skyrim/Forms/MagicItem.cpp`

### Server

- `Code/server/Services/CharacterBuildService.h`
- `Code/server/Services/CharacterBuildService.cpp`
- `Code/server/Components/CharacterBuildComponent.h`

### Protocol

- `Code/encoding/Messages/CharacterBuildRequest.*`
- `Code/encoding/Messages/CharacterBuildResponse.*`
- `Code/encoding/Messages/CharacterBuildAppliedRequest.*`
- `Code/encoding/Messages/NotifyCharacterBuildState.*`
- `Code/encoding/Structs/CharacterBuild.*`

### UI

- `Code/skyrim_ui/src/app/data/character-loadouts.ts`
- Character Creation components and services under `Code/skyrim_ui/src/app/`.

### Creation Kit

- `GameFiles/Skyrim/STRE_AlternateStart.esp`
- `GameFiles/Skyrim/Source/Scripts/QF_STRE_QUEST_AlternateStart_02001AF9.psc`
- `GameFiles/Skyrim/Scripts/QF_STRE_QUEST_AlternateStart_02001AF9.pex`
- `GameFiles/STRE_AlternateStart.manifest.txt`

### Audits and tests

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

## Trading tests

- `Code/tests/trade.cpp`
- `Code/tests/trade_inventory.cpp`
- `Code/tests/trade_application.cpp`
- `Code/tests/trade_reconciliation.cpp`
- `Code/tests/trade_messages.cpp`

## Canonical documentation at the snapshot date

- `README.md`, `CHANGELOG.md`, and `ROADMAP.md` were public entry points;
- `docs/features/trading/` documented Trading;
- `docs/features/item-preview/` documented preview;
- `docs/features/alternate-start/` documented Alternate Start and Character Build;
- the dated current-state audit is retained as [`CURRENT_STATE_AUDIT_20260727.md`](CURRENT_STATE_AUDIT_20260727.md);
- `docs/README.md` was the canonical documentation index.
