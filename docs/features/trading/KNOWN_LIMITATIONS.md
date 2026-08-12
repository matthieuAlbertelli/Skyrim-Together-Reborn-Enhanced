# Trading — Known Limitations

> **Status:** observed and tracked.

## Functional

- no gold;
- no quest items;
- no equipped items;
- no complex or enchanted instances;
- no splitting of heterogeneous instances;
- one session per player;
- no recovery after server restart.

## Architecture

- `IsMvpTransferable` is duplicated on client and server;
- the native UI multiplexes through `toggleDebugUI`;
- client inventory remains a fragile engine boundary;
- a saga plus reconciliation replaces true atomicity;
- tests are mostly unit-level rather than end-to-end network tests.

## UX

- error messages remain technical;
- invitation timeout is not represented explicitly;
- controller navigation remains to be validated;
- accessibility and localization remain incomplete;
- ultra-wide behavior remains to be tested.
