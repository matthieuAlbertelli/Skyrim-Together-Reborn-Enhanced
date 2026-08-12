# Item Preview — Test Plan

> **Status:** smoke-tested in Trading and Character Creation; automated tests
> remain incomplete.

## Already observed in game

- real objects loaded in Trading;
- real objects loaded in Character Creation;
- automatic framing;
- rapid selection changes;
- updated Angular regions;
- native reload where necessary.

## Pure solver

- very small and very large models;
- offset centers;
- edges;
- minimum and maximum scale;
- invalid bounds;
- convergence after refinements.

## Runtime

- idempotent begin/end;
- reordered host show/hide;
- item change during measurement;
- stale region revision;
- cancelled pending reload;
- missing manager;
- concurrent bind;
- future lease preemption;
- Trading → Character Creation → Trading transition.

## Rendering

- 1080p, 1440p, and 4K;
- 16:9 and 21:9;
- UI scaling;
- weapons, armor, books, potions, and small objects;
- objects with atypical bounds;
- STRE outfits;
- conflict with inventory and crafting menus;
- explicit behavior for spells without a useful 3D model.
