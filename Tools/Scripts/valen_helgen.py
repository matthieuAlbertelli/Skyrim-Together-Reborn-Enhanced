"""Typed #94 candidate audit and companion generation. Never writes an ESP.

The official xEdit script owns record allocation/serialization. This independent
reader rejects extra records, edits to existing records and malformed contracts.
Generated artifacts stay beside that private candidate until human import.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import struct
import wave
from pathlib import Path

from audit_mq101_quickstart5 import parse_records, record_payload, iter_subrecords

ROOT = Path(__file__).resolve().parents[2]
PLUGIN = "STRE_AlternateStart.esp"
NARRATIVE = "STRE_QUEST_ValenHelgen"
OWNER = "STRE_QUEST_ValenDialogue"
BRANCH = "STRE_DLBR_ValenHelgen"
NEWS = "STRE_DIAL_ValenHelgen_News"
ACCEPT = "STRE_DIAL_ValenHelgen_Accept"
NEWS_INFO = "STRE_INFO_ValenHelgen_News"
ACCEPT_INFO = "STRE_INFO_ValenHelgen_Accept"
EXPECTED = {NARRATIVE: "QUST", OWNER: "QUST", BRANCH: "DLBR", NEWS: "DIAL",
            ACCEPT: "DIAL", NEWS_INFO: "INFO", ACCEPT_INFO: "INFO"}
FIELDS = {
    NARRATIVE: 'EDID VMAD FULL DNAM NEXT INDX INDX QSDT CNAM QOBJ FNAM NNAM ANAM',
    OWNER: 'EDID DNAM NEXT ANAM',
    BRANCH: 'EDID QNAM TNAM DNAM SNAM',
    NEWS: 'EDID FULL PNAM BNAM QNAM DATA SNAM TIFC',
    ACCEPT: 'EDID FULL PNAM BNAM QNAM DATA SNAM TIFC',
    NEWS_INFO: 'EDID ENAM TPIC PNAM TCLT TRDT NAM1 TRDT NAM1 CTDA CTDA',
    ACCEPT_INFO: 'EDID VMAD ENAM TPIC PNAM TRDT NAM1 CTDA CTDA',
}
MASTERS = ["Skyrim.esm", "Update.esm", "Dragonborn.esm"]
TITLE = "Les ruines de Helgen"
OBJECTIVE = "Se rendre à Helgen pour enquêter sur l'attaque du dragon."
PROMPT = "Nous irons peut-être voir ce qui s'y est passé."
LINES = {
    NEWS_INFO: ["Des nouvelles nous sont parvenues de Helgen. La ville aurait été attaquée par un dragon.",
                "Les récits sont confus, mais quelque chose de grave s'y est produit."],
    ACCEPT_INFO: ["Faites-le. Mais restez prudents. Si ces récits sont vrais, Helgen pourrait n'être que le commencement."],
}


def require(ok, message):
    if not ok:
        raise ValueError(message)


def text(raw):
    # Skyrim's nonlocalized French strings are Windows-1252; never repair mojibake.
    return raw.rstrip(b"\0").decode("cp1252")


class Plugin:
    def __init__(self, data):
        self.data = data
        self.records = parse_records(data)
        self.fields = {r.form_id: list(iter_subrecords(record_payload(data, r))) for r in self.records}
        self.ids = {}
        for r in self.records:
            edid = self.one(r.form_id, "EDID", b"").rstrip(b"\0").decode("ascii")
            if edid:
                require(edid not in self.ids, "Duplicate EditorID: " + edid)
                self.ids[edid] = r.form_id
        require(len(self.fields) == len(self.records), "Duplicate record FormID")
        self.parents = {}

        def walk(start, end, topic=None):
            pos = start
            while pos < end:
                size = struct.unpack_from("<I", data, pos + 4)[0]
                if data[pos:pos + 4] == b"GRUP":
                    label, kind = struct.unpack_from("<II", data, pos + 8)
                    walk(pos + 24, pos + size, label if kind == 7 else topic)
                    pos += size
                else:
                    self.parents[struct.unpack_from("<I", data, pos + 12)[0]] = topic
                    pos += 24 + size
        walk(0, len(data))

    def values(self, fid, sig):
        return [v for s, v in self.fields[fid] if s == sig]

    def one(self, fid, sig, default=None):
        vals = self.values(fid, sig)
        require(len(vals) <= 1, f"Duplicate {sig} in {fid:08X}")
        if not vals:
            require(default is not None, f"Missing {sig} in {fid:08X}")
            return default
        return vals[0]

    def integer(self, fid, sig):
        return int.from_bytes(self.one(fid, sig), "little")


class Vmad:
    def __init__(self, data):
        self.data, self.pos = data, 0
        require(self.num("H") == 5 and self.num("H") == 2, "VMAD format")
        self.scripts = []
        for _ in range(self.num("H")):
            name, flags, props = self.string(), self.num("B"), {}
            for _ in range(self.num("H")):
                key, kind, flag = self.string(), self.num("B"), self.num("B")
                require(kind == 1 and flag == 1, "Only configured object property allowed")
                unused, alias, fid = self.num("H"), self.num("h"), self.num("I")
                require(unused == 0 and alias == -1 and key not in props, "Invalid object binding")
                props[key] = fid
            self.scripts.append((name, flags, props))

    def take(self, size):
        raw = self.data[self.pos:self.pos + size]
        require(len(raw) == size, "Truncated VMAD")
        self.pos += size
        return raw

    def num(self, fmt):
        return struct.unpack("<" + fmt, self.take(struct.calcsize("<" + fmt)))[0]

    def string(self):
        return self.take(self.num("H")).decode("ascii")

    def fragment(self, name, quest=False):
        require(self.num("B") == 2, "Fragment bind version")
        require(self.num("H" if quest else "B") == 1, "Exactly one stage/begin fragment required")
        require(self.string() == name, "Fragment filename")
        if quest:
            require(self.num("H") == 10, "QF must run at stage 10")
            self.take(2)
            require(self.num("I") == 0, "QF log index")
        self.take(1)
        require(self.string() == name and self.string() == "Fragment_0", "Fragment callback")
        if quest:
            require(self.num("H") == 0, "No aliases in narrative VMAD")
        require(self.pos == len(self.data), "Unexpected VMAD tail")


def audit(before: bytes, candidate: bytes):
    old, p = Plugin(before), Plugin(candidate)
    require([text(v) for v in p.values(0, "MAST")] == MASTERS, "Master order")
    require([text(v) for v in old.values(0, "MAST")] == MASTERS, "Baseline master order")
    require(p.records[0].flags == old.records[0].flags and not p.records[0].flags & 0x200, "TES4 flags/ESL")
    require(candidate[20:24] == before[20:24], "TES4 form version")
    require(p.one(0, "HEDR")[:4] == old.one(0, "HEDR")[:4], "HEDR format version")
    for r in old.records[1:]:
        match = next((c for c in p.records if c.form_id == r.form_id), None)
        require(match is not None and match.signature == r.signature and match.flags == r.flags,
                f"Existing record removed/changed: {r.form_id:08X}")
        require(p.fields[r.form_id] == old.fields[r.form_id], f"Existing payload changed: {r.form_id:08X}")
    new = [r for r in p.records if r.form_id not in old.fields]
    require(len(new) == len(EXPECTED), "Expected exactly seven added records")
    require(set(p.ids) - set(old.ids) == set(EXPECTED), "Unexpected new EditorIDs")
    for r in new:
        name = p.one(r.form_id, "EDID").rstrip(b"\0").decode("ascii")
        require(r.signature == EXPECTED[name] and r.form_id >> 24 == len(MASTERS), "Record ownership/signature")
        require(r.flags == 0, "Unexpected added record flags")
        require([s for s, _ in p.fields[r.form_id]] == FIELDS[name].split(),
                "Unexpected field/order in " + name)
    n, d, b, news, accept, ni, ai = [p.ids[x] for x in EXPECTED]
    valen = p.ids["STRE_NPC_MessireValen"]
    require(text(p.one(n, "FULL")) == TITLE, "Narrative title")
    for fid, flags, kind in [(n, 0x100, 8), (d, 0x8101, 0)]:
        data = p.one(fid, "DNAM")
        require(len(data) == 12 and int.from_bytes(data[:2], "little") == flags and data[2] == 50
                and int.from_bytes(data[8:], "little") == kind, "Quest flags/priority/type")
    require(not p.values(d, "INDX") and not p.values(d, "QOBJ") and not p.values(d, "FULL"), "Dialogue owner must not enter journal")
    stages = p.values(n, "INDX")
    require(len(stages) == 2 and [int.from_bytes(s[:2], "little") for s in stages] == [0, 10]
            and all(len(s) == 4 and s[2] == 0 for s in stages), "Stage contract")
    require(p.values(n, "QSDT") == [b"\0"] and len(p.values(n, "CNAM")) == 1, "One uncompleted stage log")
    require(p.integer(n, "QOBJ") == 10 and text(p.one(n, "NNAM")) == OBJECTIVE, "Objective 10")
    require(p.integer(n, "FNAM") == 0, "Objective flags")
    require(not p.values(n, "QSTA"), "No target/arrival implementation")
    require(p.integer(b, "QNAM") == d and p.integer(b, "TNAM") == 0
            and p.integer(b, "DNAM") == 1 and p.integer(b, "SNAM") == news, "Branch ownership/start")
    for fid, info, name in [(news, ni, NEWS_INFO), (accept, ai, ACCEPT_INFO)]:
        require(p.integer(fid, "QNAM") == d and p.integer(fid, "BNAM") == b, "Topic ownership")
        require(p.one(fid, "DATA") == bytes(4) and p.one(fid, "SNAM") == b"CUST", "Custom player topic")
        require(p.integer(fid, "TIFC") == 1, "Topic INFO count")
        require(p.one(fid, "PNAM") == struct.pack('<f', 50), "Topic priority")
        require(p.parents[info] == fid and p.integer(info, "TPIC") == fid and p.integer(info, "PNAM") == 0, "INFO parent/order")
        require(p.integer(info, "ENAM") == (0xA00 if info == ni else 0xA01), "Subtitle/NoLIP/Goodbye flags")
        require([text(v) for v in p.values(info, "NAM1")] == LINES[name], "French response text")
        require([v[12] for v in p.values(info, "TRDT")] == list(range(1, len(LINES[name]) + 1)), "Response ordering")
        conditions = p.values(info, "CTDA")
        require(len(conditions) == 2 and all(len(c) == 32 for c in conditions), "Two CTDA required")
        for c, op, value, func, target in zip(conditions, [0, 0x80], [1., 10.], [72, 58], [valen, n]):
            require(c[0] == op and struct.unpack_from("<f", c, 4)[0] == value
                    and int.from_bytes(c[8:10], "little") == func
                    and int.from_bytes(c[12:16], "little") == target
                    and c[20:28] == bytes(8), "Valen/preacceptance condition")
    require(p.values(ni, "TCLT") == [struct.pack("<I", accept)] and not p.values(ai, "TCLT"), "Topic continuation")
    require(text(p.one(accept, "FULL")) == PROMPT, "Player acceptance text")
    require(text(p.one(news, "FULL")) == 'Quelles nouvelles avez-vous de Helgen ?', "News topic text")
    qf, tif = f"QF_STRE_QUEST_ValenHelgen_{n:08X}", f"TIF_{ai:08X}"
    for fid, name, props, quest in [(n, qf, {}, True), (ai, tif, {"ValenHelgen": n}, False)]:
        vm = Vmad(p.one(fid, "VMAD"))
        require(len(vm.scripts) == 1 and vm.scripts[0][0] == name and vm.scripts[0][2] == props, "Script/property binding")
        vm.fragment(name, quest)
    return {"success": True, "result": {"EditorId": NARRATIVE, "FormKey": f"{n & 0xFFFFFF:06X}:{PLUGIN}",
            "Records": [{"signature": EXPECTED[name], "editorId": name,
                         "expectedLocalFormId": f"0x{p.ids[name] & 0xFFFFFF:08X}"} for name in EXPECTED],
            "QF": qf, "TIF": tif, "DialogueFormId": d, "NewsInfo": ni, "AcceptInfo": ai,
            "Sha256": hashlib.sha256(candidate).hexdigest().upper()}}


def generate(report, data_root: Path, voice: str):
    require(data_root.name == "Data" and data_root.parent.parent.name == "modding", "Generation must stay private")
    require(voice.isascii() and voice.isalnum(), "Invalid voice EditorID")
    result = report["result"]
    source = data_root / "Source/Scripts"
    source.mkdir(parents=True, exist_ok=True)
    (data_root / "Scripts").mkdir(exist_ok=True)
    for kind in ["QF", "TIF"]:
        name = result[kind]
        template = (ROOT / f"Tools/Scripts/valen/{kind}.psc.in").read_text(encoding="utf-8")
        (source / f"{name}.psc").write_text(template.replace("__SCRIPT__", name), encoding="utf-8", newline="\n")
    seq = data_root / "SEQ"
    seq.mkdir(exist_ok=True)
    (seq / "STRE_AlternateStart.seq").write_bytes(struct.pack("<I", result["DialogueFormId"]))
    # Temporary silent PCM timing assets: no actor voice, recording or speech synthesis.
    audio = data_root / "Sound/Voice" / PLUGIN / voice
    audio.mkdir(parents=True, exist_ok=True)
    for topic, key, durations in [(NEWS, "NewsInfo", [9, 7]), (ACCEPT, "AcceptInfo", [10])]:
        for number, seconds in enumerate(durations, 1):
            filename = f"{OWNER[:10]}_{topic[:15]}_{result[key] & 0xFFFFFF:08x}_{number}.wav".lower()
            with wave.open(str(audio / filename), "wb") as wav:
                wav.setparams((1, 2, 44100, 0, "NONE", "not compressed"))
                wav.writeframes(bytes(44100 * 2 * seconds))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--before", required=True, type=Path)
    parser.add_argument("--candidate", required=True, type=Path)
    parser.add_argument("--generate", action="store_true")
    parser.add_argument("--voice")
    parser.add_argument("--manifest", type=Path)
    args = parser.parse_args()
    report = audit(args.before.read_bytes(), args.candidate.read_bytes())
    if args.generate:
        generate(report, args.candidate.parent, args.voice or "")
    if args.manifest:
        manifest = json.loads((ROOT / "docs/features/alternate-start/CK_RECORDS_M7_IMPLEMENTED.json").read_text(encoding="utf-8-sig"))
        manifest["records"].extend(report["result"]["Records"])
        args.manifest.write_text(json.dumps(manifest, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(report, ensure_ascii=False))


if __name__ == "__main__":
    main()
