# Carte des dépendances

> **Statut : relations structurelles courantes**

Ce document décrit **les dépendances entre sous-systèmes**, pas leur avancement.

## World Sync

```text
Skyrim reference / inventory event
  → client InventoryService / WorldEntity lifecycle
  → WorldEntity protocol
  → server InventoryService authority
  → remote binding/materialization
  → local Havok
  → authoritative settlement
```

Manipulation :

```text
Better Grabbing (external SKSE plugin)
  → Skyrim grab/release behavior/events
  → STRE WorldEntity manipulation lifecycle
  → server authority
  → remote hide/release
```

Dependency gate:

```text
Authentication
  → generic NativePlugins inventory
  → ModPolicy:sRequiredNativePlugins
  → BetterGrabbing.dll required by default
```

## Trading

```text
Trade domain
  → protocol
  → server TradeService
  → client TradeService
  → Angular UI
  → Item Preview
```

Inventory metadata limits feed the trade eligibility policy; unsupported instance metadata must fail closed.

## Item Preview

```text
NativeSession + Controller + Solver + RasterMeasurer + Host
  → Trading
  → Character Creation
```

Future lease arbitration sits between consumers and the native host.

## Alternate Start / Character Build

```text
CK quest + RaceMenu
  → Character Creation UI
  → logical selections
  → CharacterBuildCatalog
  → server CharacterBuildService OR local fallback
  → canonical inventory/spells
  → local application + acknowledgement
```

CK record/catalog coupling:

```text
STRE_AlternateStart.esp
  ↔ CK_RECORDS_M7_IMPLEMENTED.json
  ↔ CharacterBuildCatalog
  ↔ character-loadouts.ts
  ↔ tests/audits
```

## Campaign

```text
Character binding/persistence
  → Campaign State
  → roster/ready
  → shared introduction/departure
  → late join/reconnect
```

## Mod Integration

```text
first-party feature contracts
  → generalized capability model
  → adapter registry/version negotiation
  → Papyrus/C++ SDK
```

Do not invert this dependency by freezing a generic SDK before enough first-party contracts are proven.
