"""Bounded presentation-only authoring and independent ESP audit for #96.

Python never serializes an ESP. Historical matrix plans/receipts keep their
editorial bytes; this projection is applied only by the presentation author.
"""
from collections import Counter
import argparse
import hashlib
import json
from pathlib import Path
import struct
import wave

import valen_matrix as corpus
from valen_matrix_plan import build_plan, NPC, NARRATIVE, OWNER
from valen_matrix_audit import audit_semantics
from valen_matrix_writer import literal
from valen_helgen import Plugin, text, require, MASTERS

REPLACEMENTS = {'\u2019': "'", '\u2018': "'", '\u201c': '"', '\u201d': '"', '\u2014': '-', '\u2026': '...'}
CORPUS_SHA256 = '88EB2A8690EA3E7BDD7FBE9DF40DA7228B6A4946FB1280107BEB70FDF655E030'
EDITORIAL_SHA256 = '9B29AFABA5E75CF9436817AA77619B3408DB9530D5884F896E05C94EA122D4F5'
BASELINE_SHA256 = '2ECEA346231CF6F6453205948FEF4E6C5CE5882C19C1F9141D9873A41DEE355E'
TRIAL_INFO = 'STRE_INFO_VALEN_SELF_010_R'
NO_LIP = 0x800
FORCE_SUBTITLE = 0x200


def sha(data):
    return hashlib.sha256(data).hexdigest().upper()


def runtime_text(value):
    """Only the six approved substitutions; no Unicode/ASCII normalization."""
    return value.translate(str.maketrans(REPLACEMENTS))


def frozen_corpus():
    raw = corpus.CORPUS.read_bytes()
    # Pin Git's LF bytes across Windows/Linux checkouts. The session additionally
    # pins the raw checkout hash before/after every authoring phase.
    require(sha(raw.replace(b'\r\n', b'\n')) == CORPUS_SHA256, 'Canonical corpus bytes changed')
    data = corpus.read_json(corpus.CORPUS)
    require(corpus.editorial_hash(data) == EDITORIAL_SHA256 == data['source']['editorialSha256'],
            'Canonical editorial fingerprint changed')
    corpus.require_authoring_ready(data)
    return data


def text_fields(plugin, plan):
    """Yield only user-visible fields owned by the reviewed matrix."""
    for record in plan['records']:
        sig, name = record['signature'], record['edid']
        tags = {'DIAL': ('FULL',), 'INFO': ('RNAM', 'NAM1'), 'QUST': ('NNAM',)}.get(sig, ())
        for tag in tags:
            for index, raw in enumerate(plugin.values(plugin.ids[name], tag)):
                require(raw.endswith(b'\0') and b'\0' not in raw[:-1], 'Malformed text field')
                yield name, tag, index, text(raw)


def voice_filename(quest, topic, form_id, response):
    """xEdit Edit Scripts/Export dialogues.pas, InfoFileName (SSE)."""
    qlen, dlen = len(quest), len(topic)
    if qlen + dlen > 25:
        if qlen > 10:
            qlen, dlen = 10, 15
        else:
            dlen = 25 - qlen
    return f'{quest[:qlen]}_{topic[:dlen]}_{form_id & 0xFFFFFF:08x}_{response}.wav'.lower()


def inspect_trial(plugin, data_root):
    """Static resolution is deliberately not reported as an engine load."""
    fid = plugin.ids[TRIAL_INFO]
    # INFO ownership is its type-7 GRUP. TPIC means Previous Topic in SSE.
    topic = plugin.parents[fid]
    owner = plugin.integer(topic, 'QNAM')
    topic_name, owner_name = text(plugin.one(topic, 'EDID')), text(plugin.one(owner, 'EDID'))
    require(owner_name == OWNER, 'Trial dialogue owner changed')
    require(plugin.integer(plugin.ids[NPC], 'VTCK') == 0x9843A, 'Valen voice type changed')
    assets = []
    for response in plugin.values(fid, 'TRDT'):
        number = response[12]
        path = Path('Sound/Voice/STRE_AlternateStart.esp/MaleWarlock') / voice_filename(owner_name, topic_name, fid, number)
        full = data_root / path
        with wave.open(str(full), 'rb') as wav:
            require((wav.getnchannels(), wav.getsampwidth(), wav.getframerate(), wav.getcomptype()) ==
                    (1, 2, 44100, 'NONE'), 'Trial WAV PCM format')
            frames = wav.getnframes()
            while samples := wav.readframes(65536):
                require(not any(samples), 'Trial WAV is not silent')
        assets.append(dict(response=number, path=path.as_posix(), sha256=sha(full.read_bytes()),
                           seconds=frames / 44100, frames=frames))
    flags = plugin.integer(fid, 'ENAM')
    return dict(info=TRIAL_INFO, formKey=f'{fid & 0xFFFFFF:06X}:STRE_AlternateStart.esp',
                topic=topic_name, voiceType='MaleWarlock', voiceTypeFormId='0009843A',
                flags=flags, forceSubtitle=bool(flags & FORCE_SUBTITLE), noLipFile=bool(flags & NO_LIP),
                assets=assets, engineLoad='NOT_OBSERVED', runtimeSkip='NOT_TESTED')


def changes(before, variant):
    require(variant in ('A', 'B'), 'Only A/B are authorized; C requires a human B failure')
    plan = build_plan(frozen_corpus())
    plugin = Plugin(before)
    result = []
    for name, tag, index, value in text_fields(plugin, plan):
        projected = runtime_text(value)
        require(len(projected.encode('cp1252')) <= 255, 'Projected string exceeds SSE field limit')
        if projected != value:
            result.append(dict(edid=name, field=tag, index=index, before=value, after=projected))
    require(plugin.integer(plugin.ids[TRIAL_INFO], 'ENAM') == FORCE_SUBTITLE | NO_LIP,
            'Trial requires the unchanged A flags')
    if variant == 'B':
        result.append(dict(edid=TRIAL_INFO, field='ENAM', index=0,
                           before=FORCE_SUBTITLE | NO_LIP, after=FORCE_SUBTITLE))
    return result


def audit(before, candidate, variant):
    expected_changes = changes(before, variant)
    old, new = Plugin(before), Plugin(candidate)
    require(old.ids == new.ids and old.parents == new.parents, 'Record identities/parents changed')
    require(len(old.records) == len(new.records), 'Record count changed')
    require([text(v) for v in new.values(0, 'MAST')] == MASTERS, 'Master order')
    edits = {(old.ids[c['edid']], c['field'], c['index']): c for c in expected_changes}
    changed_records = []
    for a, b in zip(old.records, new.records):
        require((a.form_id, a.signature, a.flags) == (b.form_id, b.signature, b.flags),
                'Record order/type/flags changed')
        require(before[a.offset+20:a.offset+24] == candidate[b.offset+20:b.offset+24], 'Form version changed')
        counts = Counter()
        expected = []
        for tag, raw in old.fields[a.form_id]:
            key = (a.form_id, tag, counts[tag])
            counts[tag] += 1
            if key in edits:
                value = edits[key]['after']
                raw = struct.pack('<H', value) + raw[2:] if tag == 'ENAM' else value.encode('cp1252') + b'\0'
            expected.append((tag, raw))
        require(new.fields[b.form_id] == expected, f'Unexpected serialized delta: {a.form_id:08X}')
        if old.fields[a.form_id] != expected:
            changed_records.append(f'{a.form_id:08X}')
    data = frozen_corpus()
    plan = build_plan(data)
    fields = list(text_fields(new, plan))
    for name, tag, index, value in fields:
        require(not set(value) & REPLACEMENTS.keys(), f'Unsafe runtime typography: {name}/{tag}/{index}')
    # Check actual serialized CTDA, including all class and survivor contexts.
    audit_semantics(new, plan, data)
    return dict(success=True, variant=variant, sha256=sha(candidate), records=len(new.records),
                changedRecords=changed_records, textChanges=sum(c['field'] != 'ENAM' for c in expected_changes),
                checkedFields=dict(Counter(tag for _, tag, _, _ in fields)),
                corpusSha256=CORPUS_SHA256, editorialSha256=EDITORIAL_SHA256,
                flagChanges=1 if variant == 'B' else 0, companionsChanged=0,
                classContexts=22, survivorContexts=100, runtimePass=False)


def render(before, variant):
    operations = []
    for c in changes(before, variant):
        operations.append(f"e := FindByEditorID({literal(c['edid'])});")
        tag, index, value = c['field'], c['index'], c['after']
        if tag == 'ENAM':
            operations.append(f"SetElementNativeValues(e,'ENAM\\Flags',{value});")
        elif tag == 'NAM1':
            operations.append(f"SetElementEditValues(ElementByIndex(ElementByPath(e,'Responses'),{index}),'NAM1',{literal(value)});")
        elif tag == 'NNAM':
            operations.append(f"SetElementEditValues(ElementByIndex(ElementByPath(e,'Objectives'),{index}),'NNAM',{literal(value)});")
        else:
            operations.append(f"SetElementEditValues(e,{literal(tag)},{literal(value)});")
    template = (Path(__file__).parent / 'xedit/STRE_ValenPresentation.pas').read_text('utf-8')
    return template.replace('{ GENERATED_PRESENTATION }', '\n  '.join(operations))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--root', type=Path, required=True)
    parser.add_argument('--variant', choices=('A', 'B'), required=True)
    parser.add_argument('--audit', action='store_true')
    parser.add_argument('--saved', action='store_true', help='Audit the human-saved private Data candidate')
    args = parser.parse_args()
    root = args.root.resolve()
    require(root.parent == (corpus.ROOT / '_audit/modding').resolve(), 'Private session required')
    before = (root / 'before.esp').read_bytes()
    require(sha(before) == BASELINE_SHA256, 'Not the human-tested c48be63 baseline')
    if args.audit:
        candidate = root / ('Data/STRE_AlternateStart.esp' if args.saved else 'authored.esp')
        report = audit(before, candidate.read_bytes(), args.variant)
        output = 'saved-presentation-audit.json' if args.saved else 'presentation-audit.json'
        (root / output).write_text(json.dumps(report, indent=2)+'\n', encoding='utf-8')
    else:
        report = dict(success=True, variant=args.variant, changes=changes(before, args.variant),
                      baselineSha256=sha(before), trial=inspect_trial(Plugin(before), root / 'Data'))
        (root / 'presentation-plan.json').write_text(json.dumps(report, ensure_ascii=False, indent=2)+'\n', encoding='utf-8')
        (root / 'author.ssepas').write_text(render(before, args.variant), encoding='utf-8-sig')
    print(json.dumps({k:v for k,v in report.items() if k not in ('changes', 'changedRecords')}))


if __name__ == '__main__':
    main()
