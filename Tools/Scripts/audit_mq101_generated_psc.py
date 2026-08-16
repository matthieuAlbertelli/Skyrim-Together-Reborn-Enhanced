#!/usr/bin/env python3
"""Post-CK semantic audit for generated QF_MQ101_0003372B.psc.

The script compares the current generated QF against the Git HEAD baseline.
It is designed to be run together with audit_mq101_quickstart5.py:

  1) ESP audit proves the MQ101 stage/log-entry/condition structure.
  2) This audit proves the generated Papyrus delta contains exactly the
     expected 12 new STRE fragment bodies and that pre-existing fragment
     bodies (including the #65 New Game bootstrap) did not change.

It does not modify any file.
"""

from __future__ import annotations

import argparse
import re
import subprocess
import sys
from collections import Counter
from pathlib import Path

REPO_RELATIVE = "GameFiles/Skyrim/Source/Scripts/QF_MQ101_0003372B.psc"

EXPECTED = {
    "stage 20": [
        "CartPathAmbientMarker.Disable()",
    ],
    "stage 25": [
        "NorthGate.SetOpen(False)",
        "NorthGate.SetLockLevel(5)",
    ],
    "stage 30": [
        "CiviliansOutsideHelgenMarker.Disable()",
    ],
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
    "stage 200": [
        "ExtHelgenAttackASREF.Disable()",
    ],
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

FRAGMENT_RE = re.compile(
    r";BEGIN FRAGMENT\s+(Fragment_\d+)\s*"
    r"Function\s+\1\s*\(\)\s*"
    r"(.*?)"
    r"EndFunction\s*"
    r";END FRAGMENT",
    re.IGNORECASE | re.DOTALL,
)

CODE_RE = re.compile(
    r";BEGIN CODE\s*(.*?)\s*;END CODE",
    re.IGNORECASE | re.DOTALL,
)


def run_git(repo: Path, *args: str) -> bytes:
    proc = subprocess.run(
        ["git", *args],
        cwd=repo,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
    )
    if proc.returncode:
        raise RuntimeError(proc.stderr.decode("utf-8", "replace").strip())
    return proc.stdout


def find_repo(start: Path) -> Path:
    proc = subprocess.run(
        ["git", "rev-parse", "--show-toplevel"],
        cwd=start,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
    )
    if proc.returncode:
        raise RuntimeError("Impossible de trouver la racine Git.")
    return Path(proc.stdout.strip())


def decode_psc(data: bytes, label: str) -> str:
    if data.startswith(b"\xef\xbb\xbf"):
        raise RuntimeError(f"{label}: BOM UTF-8 détecté — refusé.")
    try:
        return data.decode("utf-8")
    except UnicodeDecodeError:
        # Bethesda-generated PSCs are commonly ASCII-compatible. latin-1 is
        # deliberately only a decoding fallback; semantic checks remain ASCII.
        return data.decode("latin-1")


def normalize_statement(line: str) -> str:
    # Expected fragments contain no semicolons inside string literals.
    line = line.split(";", 1)[0].strip()
    return re.sub(r"\s+", "", line).lower()


def fragment_code(block: str) -> tuple[str, ...]:
    match = CODE_RE.search(block)
    if not match:
        return tuple()

    out: list[str] = []
    for line in match.group(1).splitlines():
        norm = normalize_statement(line)
        if norm:
            out.append(norm)
    return tuple(out)


def parse_fragments(text: str) -> dict[str, tuple[str, ...]]:
    result: dict[str, tuple[str, ...]] = {}
    for match in FRAGMENT_RE.finditer(text):
        name = match.group(1)
        if name in result:
            raise RuntimeError(f"Fragment dupliqué dans le PSC: {name}")
        result[name] = fragment_code(match.group(2))
    if not result:
        raise RuntimeError("Aucun fragment Papyrus détecté.")
    return result


def norm_expected(lines: list[str]) -> tuple[str, ...]:
    return tuple(normalize_statement(x) for x in lines)


def print_body(body: tuple[str, ...]) -> str:
    return "\n".join(f"         {x}" for x in body) if body else "         <vide>"


def main() -> int:
    ap = argparse.ArgumentParser(
        description="Audit sémantique du QF MQ101 généré par le CK."
    )
    ap.add_argument(
        "psc",
        nargs="?",
        type=Path,
        default=Path(REPO_RELATIVE),
        help=f"PSC courant (défaut: {REPO_RELATIVE})",
    )
    args = ap.parse_args()

    psc = args.psc.resolve()
    if not psc.is_file():
        print(f"[FAIL] PSC introuvable: {psc}", file=sys.stderr)
        return 2

    try:
        repo = find_repo(psc.parent)
        current_bytes = psc.read_bytes()
        current = decode_psc(current_bytes, "PSC courant")

        baseline_bytes = run_git(repo, "show", f"HEAD:{REPO_RELATIVE}")
        baseline = decode_psc(baseline_bytes, "PSC HEAD")

        cur = parse_fragments(current)
        base = parse_fragments(baseline)

        errors = 0

        def ok(msg: str) -> None:
            print(f"[OK]   {msg}")

        def fail(msg: str) -> None:
            nonlocal errors
            errors += 1
            print(f"[FAIL] {msg}")

        ok("aucun BOM UTF-8 dans le PSC courant")
        print(f"[INFO] baseline HEAD : {len(base)} fragments")
        print(f"[INFO] PSC courant   : {len(cur)} fragments")

        missing_old = sorted(set(base) - set(cur))
        if missing_old:
            fail(f"fragments historiques supprimés: {', '.join(missing_old)}")
        else:
            ok("aucun fragment historique supprimé")

        changed_old = [
            name for name in sorted(set(base) & set(cur))
            if base[name] != cur[name]
        ]
        if changed_old:
            fail(
                "corps de fragments historiques modifiés: "
                + ", ".join(changed_old)
            )
            for name in changed_old:
                print(f"       {name} HEAD:")
                print(print_body(base[name]))
                print(f"       {name} courant:")
                print(print_body(cur[name]))
        else:
            ok("tous les corps de fragments historiques sont inchangés")

        if "Fragment_332" in base and "Fragment_332" in cur:
            if base["Fragment_332"] == cur["Fragment_332"]:
                ok("Fragment_332 (#65 New Game bootstrap) inchangé")
            else:
                fail("Fragment_332 (#65 New Game bootstrap) a changé")
        else:
            fail("Fragment_332 du bootstrap #65 introuvable")

        added_names = sorted(
            set(cur) - set(base),
            key=lambda n: int(n.split("_", 1)[1]),
        )
        print(
            "[INFO] nouveaux fragments: "
            + (", ".join(added_names) if added_names else "<aucun>")
        )

        if len(added_names) == 12:
            ok("exactement 12 nouveaux fragments avec Result Script")
        else:
            fail(
                f"{len(added_names)} nouveaux fragments, attendu 12 "
                "(stage 70 est volontairement vide)"
            )

        added_bodies = {name: cur[name] for name in added_names}
        body_to_names: dict[tuple[str, ...], list[str]] = {}
        for name, body in added_bodies.items():
            body_to_names.setdefault(body, []).append(name)

        expected_bodies = {
            stage: norm_expected(lines) for stage, lines in EXPECTED.items()
        }

        matched_names: set[str] = set()
        for stage, expected in expected_bodies.items():
            names = body_to_names.get(expected, [])
            if len(names) == 1:
                matched_names.add(names[0])
                ok(f"{stage}: fragment {names[0]} exactement conforme")
            elif not names:
                fail(f"{stage}: aucun nouveau fragment ne correspond")
                print("       attendu:")
                print(print_body(expected))
            else:
                fail(
                    f"{stage}: fragment attendu dupliqué: {', '.join(names)}"
                )

        unexpected_added = [n for n in added_names if n not in matched_names]
        if unexpected_added:
            fail(
                "nouveaux fragments inattendus ou non conformes: "
                + ", ".join(unexpected_added)
            )
            for name in unexpected_added:
                print(f"       {name}:")
                print(print_body(added_bodies[name]))
        else:
            ok("aucun nouveau fragment Papyrus inattendu")

        expected_counter = Counter(expected_bodies.values())
        current_counter = Counter(added_bodies.values())
        if current_counter == expected_counter:
            ok("delta Papyrus = exactement les 12 corps STRE attendus")
        else:
            fail("le multiensemble des nouveaux corps diffère du plan attendu")

        print()
        if errors:
            print(
                f"RÉSULTAT: ÉCHEC — {errors} anomalie(s). "
                "Ne lance pas Skyrim."
            )
            return 1

        print(
            "RÉSULTAT: CONFORME — le QF généré contient exactement le delta "
            "Papyrus STRE attendu et le bootstrap #65 est intact."
        )
        print(
            "À combiner avec audit_mq101_quickstart5.py pour valider "
            "l'attachement structurel des branches dans l'ESP."
        )
        return 0

    except (OSError, RuntimeError) as exc:
        print(f"[FAIL] {exc}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
