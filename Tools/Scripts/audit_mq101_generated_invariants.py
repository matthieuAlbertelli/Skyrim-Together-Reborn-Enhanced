#!/usr/bin/env python3
"""Invariant audit for generated MQ101 QF after the continuity commit.

Unlike the earlier delta audit, this script does NOT compare against Git HEAD.
It validates the current generated PSC as an absolute artifact:
- the #65 STRE New Game bootstrap fragment is still present;
- the 12 STRE MQ101 continuity bodies exist exactly once;
- no unexpected duplicate of those bodies exists;
- stage 70 remains intentionally body-less (covered structurally by the ESP audit);
- no UTF-8 BOM is present.

Run together with audit_mq101_quickstart5.py.
"""

from __future__ import annotations

import argparse
import re
import sys
from collections import Counter
from pathlib import Path

EXPECTED = {
    "stage 20": ["CartPathAmbientMarker.Disable()"],
    "stage 25": ["NorthGate.SetOpen(False)", "NorthGate.SetLockLevel(5)"],
    "stage 30": ["CiviliansOutsideHelgenMarker.Disable()"],
    "stage 40": [
        "Alias_Elenwen.TryToDisable()",
        "Alias_Justiciar01.TryToDisable()",
        "Alias_Justiciar02.TryToDisable()",
        "Alias_CivilianGunnar.TryToDisable()",
    ],
    "stage 100": [
        "Alias_Ulfric.GetActorReference().PlayIdle(OffSetStop)",
        "Alias_Ulfric.GetActorReference().RemoveItem(PrisonerCuffs, 1)",
        "Alias_Ralof.GetActorReference().SetOutfit(RalofOutfit)",
        "Alias_Ralof.GetActorReference().PlayIdle(OffSetStop)",
        "Alias_StormcloakPrisoner01.GetActorReference().RemoveItem(PrisonerCuffs)",
        "Alias_StormcloakPrisoner02.GetActorReference().RemoveItem(PrisonerCuffs)",
        "Alias_StormcloakPrisoner03.GetActorReference().RemoveItem(PrisonerCuffs)",
        "Alias_Ulfric.GetActorReference().UnequipItem(ArmorGag)",
        "Alias_Ulfric.GetActorReference().RemoveItem(ArmorGag, 1)",
    ],
    "stage 150": [
        "Alias_CartHorse1.TryToDisableNoWait()",
        "Alias_ImperialSoldier01.TryToMoveTo(Guard1Marker8)",
        "Alias_ImperialSoldier02.TryToMoveTo(Guard2Marker8)",
        "Alias_ImperialSoldierFort01.TryToMoveTo(FortGuard1Marker8)",
        "Alias_StormcloakPrisoner04.TryToMoveTo(StormcloakPrisoner1Marker8)",
        "Alias_StormcloakPrisoner02.TryToMoveTo(StormcloakPrisoner2Marker8)",
        "Alias_StormcloakPrisoner03.TryToMoveTo(StormcloakPrisoner3Marker8)",
        "Alias_CivilianGunnar.TryToMoveTo(HadvarMarker8)",
        "Alias_CivilianTorri.TryToMoveTo(TorriMarker8)",
        "Alias_CivilianTorolf.TryToMoveTo(TorolfMarker8)",
        "Alias_CivilianMatlara.TryToMoveTo(TulliusMarker8)",
        "Alias_CivilianIngrid.TryToMoveTo(TulliusMarker8)",
        "Alias_CivilianVilod.TryToMoveTo(TulliusMarker8)",
        "Alias_Elenwen.GetActorReference().MoveToMyEditorLocation()",
        "Alias_Elenwen.TryToEnableNoWait()",
        "Alias_Priest.TryToDisableNoWait()",
        "Alias_ImperialSoldierFort01.TryToDisableNoWait()",
        "Alias_HelgenArcher01.TryToDisableNoWait()",
        "Alias_HelgenArcher02.TryToDisableNoWait()",
    ],
    "stage 180": [
        "Alias_StormcloakPrisoner01.TryToDisable()",
        "Alias_StormcloakPrisoner02.TryToDisable()",
        "Alias_StormcloakPrisoner03.TryToDisable()",
        "Alias_StormcloakPrisoner04.TryToDisable()",
        "Alias_ImperialSoldierFort01.TryToDisable()",
        "Alias_ImperialSoldier01.TryToDisable()",
        "Alias_ImperialSoldier02.TryToDisable()",
        "Alias_Prisoner01.TryToDisable()",
    ],
    "stage 200": ["ExtHelgenAttackASREF.Disable()"],
    "stage 250": [
        "Weather.ReleaseOverride()",
        "Alias_CivilianMatlara.TryToDisableNoWait()",
        "Alias_CivilianIngrid.TryToDisableNoWait()",
        "Alias_CivilianVilod.TryToDisableNoWait()",
    ],
    "stage 500": [
        "CollapsingBridgeAnim.Enable()",
        "BridgeOriginal.Disable()",
        "BridgeDebris.Enable()",
    ],
    "stage 800": [
        "KeepIntroSceneA.Stop()",
        "KeepScene2A.Stop()",
        "KeepScene4A.Stop()",
        "KeepScene5A.Stop()",
        "KeepIntroSceneB.Stop()",
        "KeepScene2B.Stop()",
        "KeepScene4B.Stop()",
        "KeepScene5B.Stop()",
    ],
    "stage 900": [
        "Game.AddAchievement(1)",
        "Alias_Headsman.TryToDisable()",
        "SetStage(1000)",
    ],
}

BOOTSTRAP_REQUIRED = [
    'GameHour.SetValue(7)',
    'Player.MoveTo(STRENewGameStartMarker)',
    'STREAlternateStartQuest.Start()',
]

FRAGMENT_RE = re.compile(
    r";BEGIN FRAGMENT\s+(Fragment_\d+)\s*"
    r"Function\s+\1\s*\(\)\s*"
    r"(.*?)"
    r"EndFunction\s*"
    r";END FRAGMENT",
    re.IGNORECASE | re.DOTALL,
)
CODE_RE = re.compile(r";BEGIN CODE\s*(.*?)\s*;END CODE", re.IGNORECASE | re.DOTALL)


def normalize_statement(line: str) -> str:
    line = line.split(";", 1)[0].strip()
    return re.sub(r"\s+", "", line).lower()


def fragment_code(block: str) -> tuple[str, ...]:
    m = CODE_RE.search(block)
    if not m:
        return tuple()
    return tuple(
        norm
        for raw in m.group(1).splitlines()
        if (norm := normalize_statement(raw))
    )


def parse_fragments(text: str) -> dict[str, tuple[str, ...]]:
    result = {}
    for m in FRAGMENT_RE.finditer(text):
        result[m.group(1)] = fragment_code(m.group(2))
    if not result:
        raise RuntimeError("Aucun fragment Papyrus détecté.")
    return result


def norm_lines(lines: list[str]) -> tuple[str, ...]:
    return tuple(normalize_statement(x) for x in lines)


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("psc", type=Path)
    args = ap.parse_args()

    if not args.psc.is_file():
        print(f"[FAIL] PSC introuvable: {args.psc}", file=sys.stderr)
        return 2

    data = args.psc.read_bytes()
    if data.startswith(b"\xef\xbb\xbf"):
        print("[FAIL] BOM UTF-8 détecté", file=sys.stderr)
        return 2

    try:
        text = data.decode("utf-8")
    except UnicodeDecodeError:
        text = data.decode("latin-1")

    fragments = parse_fragments(text)
    errors = 0

    def ok(msg):
        print(f"[OK]   {msg}")

    def fail(msg):
        nonlocal errors
        errors += 1
        print(f"[FAIL] {msg}")

    ok("aucun BOM UTF-8")
    print(f"[INFO] fragments courants : {len(fragments)}")

    # #65 bootstrap invariant.
    bootstrap_candidates = []
    req = [normalize_statement(x) for x in BOOTSTRAP_REQUIRED]
    for name, body in fragments.items():
        if all(r in body for r in req):
            bootstrap_candidates.append(name)

    if len(bootstrap_candidates) == 1:
        ok(f"bootstrap #65 présent dans {bootstrap_candidates[0]}")
    else:
        fail(f"bootstrap #65 attendu exactement une fois, trouvé {bootstrap_candidates}")

    # Continuity bodies must each exist exactly once.
    current = Counter(fragments.values())
    matched_names = set()

    for stage, lines in EXPECTED.items():
        expected = norm_lines(lines)
        count = current[expected]
        if count == 1:
            names = [n for n, b in fragments.items() if b == expected]
            matched_names.update(names)
            ok(f"{stage}: {names[0]} exactement conforme")
        elif count == 0:
            fail(f"{stage}: corps STRE absent")
        else:
            names = [n for n, b in fragments.items() if b == expected]
            fail(f"{stage}: corps STRE dupliqué dans {', '.join(names)}")

    if len(matched_names) == 12:
        ok("exactement 12 fragments de continuité STRE conformes")
    else:
        fail(f"{len(matched_names)} fragments STRE conformes, attendu 12")

    print()
    if errors:
        print(f"RÉSULTAT: ÉCHEC — {errors} anomalie(s).")
        return 1

    print("RÉSULTAT: CONFORME — invariants MQ101 générés préservés.")
    print("Le stage 70 vide reste validé par audit_mq101_quickstart5.py.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
