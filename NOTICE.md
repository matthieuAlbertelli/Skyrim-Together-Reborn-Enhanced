NOTICE

This project is based on Skyrim Together Reborn
(TiltedEvolution).

Original project:
https://github.com/tiltedphoques/TiltedEvolution

This repository contains independent modifications developed
for Skyrim Together Reborn Enhanced (STRE).

The original authors retain copyright over their respective work.

All new contributions are distributed under the GPL v3,
consistent with the upstream project.

STRE Main Menu presentation adapts the before-menu PostDisplay ordering and
menu-music predicate hook points from Main Menu Video by powerofthree:
https://github.com/powerof3/MainMenuVideo
Upstream commit: ec692f0745972ba3b381e2b1df5c4c56218ee8e0.
License: GPL-3.0-or-later (upstream vcpkg manifest). The license text is retained
in GameFiles/Skyrim/STRE/Licenses/MainMenuVideo-GPL-3.0.txt.

The adapted boundary is Code/client/Games/Skyrim/MainMenuRuntime.cpp, marked
with attribution and STRE changes. STRE reuses its own renderer/input and adds
runtime checks and process-local failure handling; it does not redistribute the
upstream plugin, decoder implementation, OpenCV or any upstream media.
See docs/features/main-menu/TECHNICAL_DESIGN.md for the exact derivation.

STRE Main Menu background and supplied branding visuals: Matthieu Albertelli /
STRE, contributed under the repository's GPL-3.0-or-later terms. Their canonical
provenance and export record is docs/features/main-menu/README.md.
The local intro is excluded from distribution: its Lawrence James music awaits
permission and is not licensed by this code/visual-contribution notice.


The Windows player package also carries CEF 141.0.11 / Chromium
141.0.7390.123 runtime binaries. Their existing embedded notices are extracted
without launching CEF into SkyrimTogetherReborn/Licenses/CEF-LICENSE.txt and
Chromium-CREDITS.html (plus its CSS/JS). The bytes come from that package's
resources.pak, using the resource IDs of the pinned cef_pack_resources.h.
The source CEF revision is 7e73ac45f2979537f677e785383a04998f90d51d.
Angular's production build retains UI/3rdpartylicenses.txt.
These notices do not license external game/mod assets or private media.

Verified native and UI notice texts are versioned under Tools/Packaging/licenses.
Tools/Packaging/redistribution.json identifies their exact versions, provenance
and remaining rights questions. The collector checks these texts before playable
packaging; it refuses incomplete UI notices. See Tools/Packaging/README.md for
the corresponding-source procedure and unresolved licence boundaries.
Including a notice does not settle all rights to distribute the combined work.
