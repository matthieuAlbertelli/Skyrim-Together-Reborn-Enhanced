## Related issue

Closes #<!-- issue number -->

For a trivial documentation-only change without an issue, explain why one is unnecessary.

## Problem

What user-visible or technical problem does this change address?

## Solution

Describe the approach, important trade-offs, and expected outcome.

## Modified areas

- [ ] Native client / Skyrim integration
- [ ] Server
- [ ] Shared domain or protocol
- [ ] Angular / CEF UI
- [ ] Creation Kit / Papyrus
- [ ] Content, art, audio, or narrative
- [ ] Build / CI / packaging
- [ ] Documentation only

## Architecture and compatibility

- **Shared-state authority impact:** <!-- Who owns any affected shared mutable state? Use "None" when not applicable. -->
- **Protocol / persistence / compatibility impact:** <!-- Include versioning or migration needs. -->
- **Upstream STR impact:** <!-- Note conflicts, dependencies, or portability concerns. -->
- **ADR impact:** <!-- Link an ADR, name the ADR that needs updating, or explain why none is needed. -->

## Validation

- **Automated tests/checks:** <!-- Commands and results, or why not applicable. -->
- **Manual in-game validation:** <!-- Players, host/client roles, scenario, and result. -->
- **Logs/diagnostics:** <!-- New or inspected logs, identifiers, revisions, or rejection reasons. -->
- **UI / Creation Kit evidence:** <!-- Screenshots or video when applicable. -->

Documentation-only PRs may mark irrelevant validation fields as `Not applicable` with one short explanation.

## Documentation impact

List updated canonical documents, or explain why no documentation change is required. Do not create a second source of truth for status, roadmap, protocol, or tests.

## Checklist

- [ ] The change is scoped to one coherent outcome.
- [ ] Failure and recovery behavior are covered where applicable.
- [ ] New shared state has explicit authority and network-triggered Skyrim mutations use an engine-safe path.
- [ ] Protocol inputs and serialized collections are bounded where applicable.
- [ ] Relevant tests and documentation are updated.
- [ ] No secrets, private server details, or unlicensed assets are included.
