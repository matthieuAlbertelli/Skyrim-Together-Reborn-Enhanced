# Security Policy

## Supported versions

Security fixes are made for the latest public STRE release and, when appropriate, the current development branch. Older alpha builds and upstream STR releases are not maintained by STRE.

Before reporting a problem, confirm that it affects STRE code or packaging. Vulnerabilities that affect unmodified Skyrim Together Reborn should also be reported to the upstream project through its preferred private channel.

## Report a vulnerability privately

Do not open a public issue or Discussion for a suspected vulnerability.

Use GitHub's [private security advisory form](https://github.com/matthieuAlbertelli/Skyrim-Together-Reborn-Enhanced/security/advisories/new). Include:

- the affected STRE version, build, or commit;
- the affected client, server, installer, build, or protocol surface;
- prerequisites and exact reproduction steps;
- the expected and observed security impact;
- logs, proof-of-concept material, or a proposed mitigation when available;
- whether the issue appears to be inherited from upstream STR or a dependency.

Remove access tokens, private server addresses, personal data, copyrighted game files, and unrelated save data from the report.

Maintainers will acknowledge the report through the private advisory, investigate scope and coordinate disclosure. Please allow a reasonable remediation period before publishing details. No bounty or fixed response-time commitment is currently offered.

## Scope

Security reports include vulnerabilities in STRE-owned client/server code, network protocol handling, release packaging, build automation, native-plugin integration, and project-controlled services. General gameplay bugs belong in the [bug report form](https://github.com/matthieuAlbertelli/Skyrim-Together-Reborn-Enhanced/issues/new/choose); setup questions belong in [GitHub Discussions](https://github.com/matthieuAlbertelli/Skyrim-Together-Reborn-Enhanced/discussions/categories/q-a).
