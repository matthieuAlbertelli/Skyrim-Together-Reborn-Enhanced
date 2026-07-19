# Code Guidelines

These conventions are inherited from Skyrim Together Reborn and supplemented by STRE architecture requirements. New code should follow the surrounding subsystem when an upstream file already establishes a stronger local convention.

## Language and toolchain

- C++20.
- No exception-based control flow.
- Use templates responsibly to avoid excessive compile times and binary growth.
- Prefer small, focused components and explicit ownership.

## Naming

### Variables

- Local variables use lower camel case: `someVariableName`.
- Function arguments are prefixed with `a`: `aPlayerId`.
- Constants are prefixed with `c`: `cMaxTradeLines`.
- Pointers use the `p` prefix where the surrounding codebase follows that convention.
- Static storage uses `s_`; global storage uses `g_`.

### Types and members

- Types begin with an uppercase letter: `TradeSession`.
- Members use `m_`: `m_sessionId`.
- Functions begin with an uppercase letter in existing C++ subsystems: `BuildMutationPlan()`.

## General C++ rules

- Names must communicate intent.
- Avoid `auto` for primitive values when signedness or width matters.
- Opening braces go on a new line.
- Prefer `noexcept` on boundaries that are not allowed to propagate failures.
- Validate pointer and game-object lifetime at Skyrim/native boundaries.
- Avoid hidden global state and singleton expansion unless the engine integration requires it and the decision is documented.

## STRE architecture rules

- Separate pure domain logic from transport, server authority, client orchestration and Skyrim adaptation.
- Every shared state must have an explicit authority.
- Network commands express intent; server events/snapshots express canonical outcomes.
- Handlers must tolerate duplicates and stale revisions where applicable.
- Serialization limits must be explicit and tested.
- Reconnectable systems need a canonical snapshot path.
- Creation Kit scenes and UI are projections of state, not the only source of truth.
- Do not hard-code plugin FormIDs when Editor IDs or configured properties are available.

## Formatting

Use the repository `.clang-format` configuration. Avoid drive-by formatting of unrelated upstream files because it increases merge conflicts.

## Commits

Use Conventional Commit prefixes:

- `feat:` new behavior;
- `fix:` defect or crash correction;
- `refactor:` structural change without intended behavior change;
- `test:` test-only change;
- `docs:` documentation;
- `chore:` build, tooling or release work.

See [CONTRIBUTING.md](CONTRIBUTING.md) for PR expectations.
