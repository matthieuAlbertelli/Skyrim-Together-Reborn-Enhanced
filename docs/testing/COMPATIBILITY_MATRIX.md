# Compatibility matrix

> **Status: Source of truth for known compatibility**
> **Last updated: August 11, 2026**

## Reference platform

| Component | Observed version/support | State |
|---|---|---|
| Skyrim SE/AE runtime | `1.6.1170` Steam | primary development runtime and public-alpha target |
| Skyrim Together Reborn upstream | historical baseline in `UPSTREAM.md` | fork integrated into STRE |
| STRE | `0.2.0-alpha.1` | first public alpha; Windows CI build validated |
| Angular | 16.x | in use |
| xmake | `3.0.9` in CI (`>= 3.0.0` project) | Windows build validated |
| Creation Kit | environment compatible with `1.6.1170` | in use |
| SKSE64 | `2.2.6` for Skyrim `1.6.1170` | alpha installation target |
| Better Grabbing | `1.17` | external plugin required by default for multiplayer World Sync manipulation |
| Address Library | `11` | required external dependency; runtime base for STRE/Better Grabbing |

The versions above are the **`v0.2.0-alpha.1` release targets**. A future release must revise this table whenever a dependency or supported runtime changes.

## Features

| Configuration | Trading | World Sync drops | World Sync placed/grab | Character Build single-player | Character Build STRE |
|---|---:|---:|---:|---:|---:|
| 1 offline player | N/A | vanilla/local | Better Grabbing local | Yes | N/A |
| 2 players | Alpha | Validated | Validated on common cases | N/A | Smoke-tested |
| 4 players | To test | To test | To test | N/A | To test |
| SkyUI | Retest required | N/A | N/A | development environment | development environment |
| Anniversary content | To test | runtime 1.6.1170 | runtime 1.6.1170 | runtime 1.6.1170 | runtime 1.6.1170 |

## Validated World Sync behavior

- dynamic drop → remote materialization;
- local Havok plus authoritative settlement;
- remote pickup;
- grab/release of a dropped WorldEntity;
- lazy adoption of a movable placed reference;
- placed-reference grab/release without an observer crash;
- ownership triggering vanilla theft on grab;
- forced release when guard dialogue opens;
- swimming after correcting the STRE regression.

## World Sync scope to extend

- quest objects;
- heavily scripted references;
- complex enable-parent references;
- cell reset;
- custom item names;
- durable persistence after restart or save branching;
- 4 players.

## Mod/dependency test record

For every mod or dependency, record:

- name and version;
- Skyrim runtime;
- load order when relevant;
- single-player result;
- STRE result;
- conflict or workaround;
- logs;
- date;
- STRE SHA.
