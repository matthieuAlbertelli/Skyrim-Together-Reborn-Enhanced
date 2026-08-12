# Upstream Strategy

> **Status:** active policy.

The current upstream baseline is recorded **only** in
[`UPSTREAM.md`](../../UPSTREAM.md). This document defines integration policy and
does not duplicate current SHAs or versions.

## Rules

- record the exact upstream commit for every release;
- integrate upstream regularly instead of through large catch-up merges;
- isolate STRE changes in dedicated services and directories where possible;
- avoid broad style changes in upstream files;
- keep structural divergences behind documented contracts;
- test hook-sensitive and protocol-breaking areas before and after every
  integration.

## Patch classification

- `isolated` — new file or service with low conflict risk;
- `factory-registration` — opcode or service registry;
- `hook-sensitive` — reverse engineering, menus, engine wrappers, magic, physics;
- `ui-invasive` — shared Angular/CEF components;
- `protocol-breaking` — incompatible factories, opcodes, schemas, or versions;
- `ck-catalog-coupled` — linked ESP, records, catalog, and UI;
- `build/release` — CI, packaging, and toolchain.

`hook-sensitive`, `protocol-breaking`, and `ck-catalog-coupled` patches require
dedicated review during an upstream update.

## STRE guardrails

- do not reintroduce an engine wrapper whose ABI signature is unproven;
- prefer already reverse-engineered and validated STR primitives;
- keep network-triggered engine mutations on the game-thread path;
- preserve STRE identity and authority contracts during merges;
- update tests and ADRs if upstream invalidates a structural assumption.
