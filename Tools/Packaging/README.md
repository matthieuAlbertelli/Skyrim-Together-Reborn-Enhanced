# Verified redistribution notices

This is an offline extension of `Tools/Scripts/collect-cef-notices.mjs`, not a
release approval system. The current Windows candidate removes Discord SDK,
reactive-forms and Futura, and removes the server announcement/httplib dependency.
Full technical notice assembly passes; other rights questions remain open.
Missing or changed provenance/notices must still stop packaging.

`redistribution.json` records versions, upstream sources, exact text hashes and
installed header/source fingerprints for the dependency set observed in the
historical `feb58b8e1ace4c1ed8a488dbbee6eeecd75ddf72` candidate, updated for the
`f2c9947d3ddd103247a0a9d7d2192c4e21f423c0` dependency cleanup. The current
Windows inventory has 21 native entries / 28 native notice texts. Its four final
linker records contain no OpenSSL/libssl/libcrypto; the inherited 1.1.1-w pin is
retained for non-Windows/transitive builds. This is not historical rights
clearance. Normalized LF is used only for textual source fingerprints; licence
files and binary font hashes retain their original bytes.
The neighbouring `.gitattributes` prevents checkout newline rewriting of those
texts. `reviewOnly` records delivered components outside the verified-notice
subset and their outstanding rights/source questions. `external` and
`buildOnly` distinguish separately installed prerequisites and build tools.
This is a scoped inventory, not a licence grant or exhaustive SBOM attestation.

The UI inventory covers the production build's 16 remaining notice sections and
installed versions/repository metadata corresponding to the lockfile. All seven
identifier-only sections have verified replacement text. Nine existing full
sections are retained byte-for-byte. Elf/Loadoff texts come from preserved
original Git objects identified by the publisher's exact npm `gitHead`; the
archive does not grant new rights. reactive-forms is absent from the lockfile,
production dependency graph and Webpack module statistics; its historical rights
finding is not reinterpreted as permission.

Roboto Condensed's six existing TTFs exactly match Google Fonts revision
`55aaf47ca89416e720a2449368943ff87fb6d02a`, version 2.137 (2017), copyright 2011
Google Inc., Apache-2.0. This is not the later OFL font. The four used variants
(normal/italic 400/700) and their exact upstream URLs/SHA-256 are in `fonts`.
The collector verifies source and distributed bytes, includes the original
Apache licence, and rejects missing/changed packaged fonts. Unused light files
remain source-only. Futura is absent from current source and output.

## Checks and assembly

Run from the repository root (Windows; only synthetic fixtures are written):

```powershell
node --test Tools/Scripts/test-cef-notices.mjs Tools/Scripts/test-verified-notices.mjs
```

The existing CEF-only call remains available for extracting its four embedded
notices. It is **not** sufficient for playable packaging. The Windows reusable
workflow calls the full collector when `upload-build-for-next-job` is enabled:

```powershell
node Tools/Scripts/collect-cef-notices.mjs <runtime-output> <repository> <xmake-package-cache> <build-deps> <ui-source>
node Tools/Scripts/collect-cef-notices.mjs --verify <assembled-runtime>
```

These are assembly commands for a separately authorized build/output, never for
Data, Vortex staging or the historical prepackage. The collector does not fetch,
evaluate Lua, execute third-party tools or launch applications. It reads the
three delivered-target linker records and the xmake package cache, verifies
versions/source fingerprints and all licence texts, then prepares all output
before writing. An incomplete notice or provenance drift fails explicitly.
Only the incomplete UI notice file may be completed; stale differing notice
outputs are refused. All existing complete UI texts are hash-pinned.

Expected player paths, relative to the Data-root archive:

- `SkyrimTogetherReborn/Licenses/CEF-LICENSE.txt`
- `SkyrimTogetherReborn/Licenses/Chromium-CREDITS.html`, `credits.css`, `credits.js`
- `SkyrimTogetherReborn/Licenses/ThirdParty/<exact-notice-file>`
- `SkyrimTogetherReborn/Licenses/redistribution.json`
- `SkyrimTogetherReborn/Licenses/notice-bundle.json`
- `SkyrimTogetherReborn/UI/3rdpartylicenses.txt`

The final packaging step verifies both the file hashes and completeness against
the inventory before artifact upload. No cache, node_modules, local receipt or
private path is copied into the player package. The receipt explicitly has
`releaseReady: false`; even a technically complete notice bundle cannot resolve
remaining Tilted/Microsoft/asset permissions, corresponding-source obligations
or the human qualification gates. Removed components' historical findings remain
in the dated review evidence.
Keep the original CEF credits, including their third-party terms, intact.

## Corresponding-source procedure (prepared, not executed)

1. Freeze the future accepted release commit, version, Git tree and each gitlink
   SHA. Record build configuration, compiler/linker versions, xmake recipe
   revisions, exact dependency source archives and checksums, patches, generated
   source inputs and UI lockfile. A tag or a moving upstream branch alone does
   not identify all linked code. Preserve the historical compiled SHA separately.
2. Map licence boundaries before redistribution: STRE GPL-3.0-or-later; the
   root LICENSE's historical launcher/CitizenFX LGPL reference; the five Tilted
   components; Chromium/FFmpeg and other reciprocal portions. Current inherited
   CitizenFX headers are in `Code/immersive_launcher/loader/{TlsMemory.cpp,
   MemoryLayout.cpp,ExeLoader.h,ExeLoader.cpp}` and refer to upstream LICENSE and
   MENTIONS. Obtain those exact terms and history before treating the old
   `code/launcher` label as a resolved boundary. SteamCrypto.h retains atom0s /
   Steamless GPL attribution. MainMenuRuntime.cpp's MainMenuVideo adaptation is
   identified by NOTICE.md and upstream commit
   `ec692f0745972ba3b381e2b1df5c4c56218ee8e0`.
3. In a new authorized source-staging directory, export the exact STRE commit
   and each submodule at its gitlink commit. `git archive` does not export
   submodule contents; export them separately under their original paths.
   Include relevant build/install scripts, interface definitions, patches,
   preferred editable sources and generators. Exclude secrets/private voices,
   personal configs, saves, campaigns and assets whose redistribution is still
   unresolved; do not silently omit sources necessary for a claimed source offer.
4. Collect matching upstream source snapshots for incorporated reciprocal
   components and exact changes/build settings. For CEF use revision
   `7e73ac45f2979537f677e785383a04998f90d51d`, its matching Chromium
   `141.0.7390.123` DEPS and the actual FFmpeg revision/build flags, not just a
   generic Chromium link. CEF's 544 extracted credit sections identify notice
   subjects, not a completed corresponding-source delivery. Resolve legacy
   launcher LGPL terms and any relinking requirement: provide eligible object
   files/relink instructions or another licence-compliant mechanism where
   required, and do not prohibit modifications/reverse engineering needed for
   debugging such modifications. Static/dynamic linkage must be assessed per
   component; changing linkage alone does not settle OpenSSL compatibility.
5. Build an indexed source delivery with immutable component IDs, original
   copyright/NOTICE/licence texts, archive hashes and a binary-to-source mapping.
   For downloadable GPLv3 binaries, plan equivalent access to corresponding
   sources from the same place under section 6(d), with clear directions and no
   extra charge; evaluate the exact LGPL options separately. Do not rely solely
   on GitHub's autogenerated archive, a mutable branch or someone else's URL
   whose retention cannot be ensured. No new source archive is created or
   published in this mission.
6. Before a future release, verify extraction and source completeness offline,
   then reproduce a build from those inputs without private prerequisites under
   separate authorization. Record any nondeterminism instead of promising
   bit-identical binaries. Resolve all outstanding rights first; a maintainer's
   product-risk acceptance cannot grant third-party rights. Preserve the
   accepted source/binary mapping durably alongside the release.

Full current release gates remain in
[STATUS](../../docs/project/STATUS.md) and the
[preparation plan](../../docs/production/releases/0.5.0-alpha.1-preparation.md).
