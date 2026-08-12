# Asset pipeline

> **Status: Proposal**

## Directory structure

```text
assets/source/<discipline>/<asset>/
assets/export/skyrim/<asset>/
GameFiles/Skyrim/meshes/STRE/
GameFiles/Skyrim/textures/STRE/
```

## Naming

Use the `STRE_` prefix for records and `stre/` directories for paths where possible. Do not ship files named `final2`, `new`, or `test`.

## Provenance record

Every asset records its author, date, license, source material, tools, version, restrictions, and required credits.

## Validation

- inspect the source;
- provide an automated export or written procedure;
- test in CK;
- test in game;
- test packaging;
- review the license.
