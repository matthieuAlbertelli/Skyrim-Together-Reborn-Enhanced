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

STRE video assets require separate provenance and redistribution approval.
No video is licensed for distribution by this code notice; the asset record is
docs/features/main-menu/README.md.
