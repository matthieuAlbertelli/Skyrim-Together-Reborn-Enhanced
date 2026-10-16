"""Render typed xEdit calls from the validated #96 plan; never write an ESP."""
from pathlib import Path
from collections import Counter
import json
import argparse

import valen_matrix as corpus
from valen_matrix_plan import build_plan, NARRATIVE, OWNER


def literal(value):
    value.encode('cp1252', errors='strict')
    return "'" + value.replace("'", "''").replace('\r', "'#13'").replace('\n', "'#10'") + "'"


def render(plan):
    lines = []
    emit = lines.append
    q = literal
    for record in plan['records']:
        if record['signature'] == 'QUST':
            if record['edid'] == NARRATIVE:
                emit(f"ExtendNarrative({q(record['objective20'])});")
            else:
                for stage in record['stages']:
                    emit(f'AddStage(dialogue,{stage},True);')
    for record in plan['records']:
        sig, edid, existing = record['signature'], record['edid'], str(record['action']=='update')
        if sig not in ('DLBR','DIAL'):
            continue
        emit(f'e := PlannedRecord({q(sig)},{q(edid)},{existing},nil);')
        emit("Link(e,'QNAM',dialogue);")
        if sig == 'DLBR':
            emit("Add(e,'TNAM',True); SetElementNativeValues(e,'TNAM',0);")
            emit("Add(e,'DNAM',True); SetElementNativeValues(e,'DNAM',1);")
        else:
            emit(f"Link(e,'BNAM',FindRecord('DLBR',{q(record['branch'])}));")
            emit(f"Add(e,'FULL',True); SetElementEditValues(e,'FULL',{q(record['text'])});")
            emit("SetElementNativeValues(e,'PNAM',50.0); Add(e,'DATA',True);")
            emit("SetElementNativeValues(e,'DATA\\Category',0); SetElementNativeValues(e,'DATA\\Subtype',0);")
            emit("Add(e,'SNAM',True); SetElementNativeValues(e,'SNAM',$54535543);")
    for record in plan['records']:
        if record['signature'] == 'DLBR':
            emit(f"Link(FindRecord('DLBR',{q(record['edid'])}),'SNAM',FindRecord('DIAL',{q(record['start'])}));")
    previous = {}
    for record in plan['records']:
        if record['signature'] != 'INFO':
            continue
        tid = record['topic']
        emit(f"t := FindRecord('DIAL',{q(tid)});")
        emit(f"e := PlannedRecord('INFO',{q(record['edid'])},{record['action']=='update'},t);")
        for field in ('Conditions','Link To','Responses','RNAM'):
            emit(f"ClearField(e,{q(field)});")
        emit(f"Add(e,'ENAM',True); SetElementNativeValues(e,'ENAM\\Flags',{record['flags']});")
        emit("Link(e,'TPIC',t); Add(e,'PNAM',True);")
        if tid in previous:
            emit(f"Link(e,'PNAM',MainRecordByEditorID(ChildGroup(t),{q(previous[tid])}));")
        else:
            emit("SetElementNativeValues(e,'PNAM',0);")
        previous[tid] = record['edid']
        if record['prompt'] is not None:
            emit(f"Add(e,'RNAM',True); SetElementEditValues(e,'RNAM',{q(record['prompt'])});")
        for link in record['links']:
            emit(f'AddTopicLink(e,{q(link)});')
        for index, segment in enumerate(record['segments'],1):
            emit(f'AddResponse(e,{index},{q(segment)});')
        for c in record['conditions']:
            stage = c['argument'] if c['function']=='GetStageDone' else 0
            variable = c['argument'] if c['function']=='GetVMQuestVariable' else ''
            emit(f"AddCondition(e,{q(c['function'])},{q(c['ref'])},{float(c['value'])},{c['operator']},{stage},{q(variable)});")
    for tid, count in Counter(r['topic'] for r in plan['records'] if r['signature']=='INFO').items():
        emit(f"t := FindRecord('DIAL',{q(tid)}); Add(t,'TIFC',True); SetElementNativeValues(t,'TIFC',{count});")
    template = (Path(__file__).parent/'xedit/STRE_ValenDialogueMatrix.pas').read_text('utf-8')
    assert template.count('{ GENERATED_PLAN }') == 1
    return template.replace('{ GENERATED_PLAN }','\n  '.join(lines))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--root',type=Path,required=True)
    args = parser.parse_args()
    root = args.root.resolve()
    corpus.require(root.parent == (corpus.ROOT/'_audit/modding').resolve(), 'Private session root required')
    plan = build_plan(corpus.read_json(corpus.CORPUS))
    (root/'record-plan.json').write_text(json.dumps(plan,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    (root/'author.ssepas').write_text(render(plan),encoding='utf-8-sig')
    print(json.dumps(dict(success=True,counts=plan['counts'],actions=plan['actions'])))


if __name__=='__main__':
    main()
