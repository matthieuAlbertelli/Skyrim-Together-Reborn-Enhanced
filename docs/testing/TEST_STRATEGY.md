# Test strategy

> **Status: Updated with M7 Character Build tests**

## Test pyramid

### Pure tests

- domain state machines;
- validation and canonicalization;
- reward catalogs;
- mutation and reconciliation plans;
- deterministic hashes;
- preview solver;
- future Campaign State transitions.

### Serialization tests

- round-trip every message;
- maximum sizes;
- truncated payloads;
- unknown enums;
- incompatible versions;
- Character Build snapshot with inventory and spells;
- acknowledgment with inventory and spell hashes.

### Service tests

- request validation;
- missing local plugins or FormIDs;
- Pending build replaced before application;
- Applied build cannot be replaced;
- revision or hash mismatch;
- retransmission;
- disconnect and future restoration.

### Client/server tests

Two automated processes or a harness:

- message order and loss;
- duplicate delivery;
- version mismatch;
- different catalogs or plugins;
- reconnect;
- latency.

### In-game tests

- Skyrim 1.6.1170;
- CK plugin;
- UI and preview;
- targeted magic;
- save and load;
- 1 player, then 2 players, then 4 and 10.

## Trading

Existing tests cover session, application, inventory planning, protocol, and reconciliation. Remaining work includes client/server integration, commit failure, disconnect at every step, stress testing, and UI end-to-end coverage.

## Character Build

`Code/tests/character_build.cpp` covers:

- class and option validation;
- nine Mage combinations;
- spell count and uniqueness;
- simplified Thief kit and 10 lockpicks;
- normalized spell hash;
- snapshot and acknowledgment serialization.

Static scripts:

- `audit_stre_plugin_records.py`;
- `audit_character_build_catalog.py`.

Add:

- dedicated server-service rejection tests;
- client/server hash harness;
- persistence and reconnect tests;
- extensible buff-classification test;
- in-game matrix for every class and option.

## Complete Alternate Start

Future matrix: new game, Helgen skip, save/load, Valen, full-roster activation,
post-seal join/replacement/wrong-binding rejection, disconnect recovery,
interrupted checkpoint, collective restore, class conflict, scene completion, and
departure.

## Preview

- solver with synthetic data;
- resizing;
- rapid changes;
- acquire and release;
- host loss;
- edge clipping;
- surface conflicts;
- Trading and Character Creation concurrency.

## Test data

Every multiplayer scenario must produce a timestamp, player IDs, build/campaign/session IDs, revisions, BuildVersion, hashes, client/server logs, load order, and SHA of the deployed ESP.
