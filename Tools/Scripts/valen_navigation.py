"""Current Player Dialogue authoring model and independent serialized topology audit.

The original schema-1 matrix plan/fixtures remain historical evidence. This typed
upgrade derives the complete navigation from that plan and the frozen corpus;
official xEdit alone allocates records and serializes the candidate.
"""
from __future__ import annotations
import argparse
from collections import Counter
from copy import deepcopy
import hashlib
import json
from pathlib import Path
import struct

import valen_matrix as corpus
from valen_matrix_plan import build_plan, condition, quest_stage, OWNER, NARRATIVE, BRANCH, GENERIC, NPC
from valen_matrix_audit import audit_semantics, conditions, resolve_conditions, evaluate, field_layout
from valen_matrix_writer import literal
from valen_helgen import Plugin, text, require, MASTERS
from valen_presentation import frozen_corpus, runtime_text, REPLACEMENTS, CORPUS_SHA256, EDITORIAL_SHA256

BASELINE_SHA256 = '31B8DD797A68BB51542F4C24986C82B21A2154E916B42D80B4574B1D7DB03471'
HELGEN_MENU = 'STRE_DIAL_VALEN_MENU_HELGEN'
RETURN = 'STRE_DIAL_Valen_ReturnRoot'
RETURN_INFO = 'STRE_INFO_Valen_ReturnRoot'
RETURN_TEXT = 'Revenons à autre chose.'  # explicitly authorized player navigation, not NPC/corpus text
NAV_SPACE = ' '  # one non-visible ASCII space in an actual TRDT/NAM1 response


def build_navigation_plan(data):
    plan = deepcopy(build_plan(data))
    nodes = {n['id']: n for n in data['nodes'] if n['state'] == 'approved'}
    root_nodes = nodes['VALEN_ROOT']['followups']
    records = plan['records']
    by = {r['edid']: r for r in records}
    root_topics = {n: next(r['topic'] for r in records if r.get('node') == n) for n in root_nodes}
    root_topics['VALEN_MENU_HELGEN'] = HELGEN_MENU
    def topic(n):
        if n in root_topics: return root_topics[n]
        return next(r['topic'] for r in records if r.get('node') == n)
    for r in records:
        r['action'] = 'update'
        if r['signature'] == 'DLBR':
            # Eleven categories stay Top-Level; six first-report branches are
            # internal Helgen choices and are reached by explicit links only.
            if r['edid'] == BRANCH: r['start'] = HELGEN_MENU
            r['flags'] = int(r['start'] in root_topics.values())
        if r['signature'] != 'INFO': continue
        node = nodes[r['node']]
        if r['node'] == 'VALEN_MENU_HELGEN': r['topic'] = HELGEN_MENU
        ids = list(node['followups'])
        parent = node['returnTo']
        if parent and parent != 'VALEN_ROOT': ids += nodes[parent]['followups']
        r['links'] = ([] if node['kind'] == 'exit' else
                      sorted({topic(n) for n in ids if topic(n) != r['topic']} | {RETURN}))
        r['navigation'] = not r['segments']
        if r['navigation']:
            r['segments'] = [NAV_SPACE]
            r['flags'] = 0x800 | (1 if node['kind'] == 'exit' else 0)
    extra = deepcopy(by['STRE_INFO_VALEN_MENU_HELGEN_NAV_1'])
    extra.update(edid='STRE_INFO_VALEN_MENU_HELGEN_NAV_2', action='create',
                 conditions=[condition('GetIsID', NPC, 1), quest_stage(20, 'GreaterOrEqual')],
                 prompt=data['helgenMenuLabels'][2]['text'])
    records.append(extra)
    records += [dict(signature='DIAL', edid=HELGEN_MENU, action='create', owner=OWNER,
                     branch=BRANCH, text=data['helgenMenuLabels'][2]['text']),
                dict(signature='DIAL', edid=RETURN, action='create', owner=OWNER,
                     branch='STRE_DLBR_VALEN_MENU_SELF', text=RETURN_TEXT),
                dict(signature='INFO', edid=RETURN_INFO, action='create', node='ADAPTER_RETURN_ROOT',
                     topic=RETURN, responseIds=[], segments=[NAV_SPACE], navigation=True,
                     conditions=[condition('GetIsID', NPC, 1)], links=sorted(root_topics.values()),
                     prompt=None, flags=0x800, fragment=None)]
    for r in records:
        for key in ('text', 'objective20', 'prompt'):
            if r.get(key) is not None: r[key] = runtime_text(r[key])
        if r['signature'] == 'INFO': r['segments'] = [runtime_text(s) for s in r['segments']]
    plan.update(schemaVersion=2, operation='FixValenNavigation', rootTopics=root_topics,
                navigationResponse=dict(text=NAV_SPACE, flags=0x800, voiceAssets=[]),
                counts=dict(Counter(r['signature'] for r in records)),
                actions=dict(Counter(r['action'] for r in records)))
    return plan


def expected_info_fields(p, r, previous):
    fid = p.ids[r['edid']]
    # TPIC is Previous Topic, NOT the owning DIAL. Ownership is the type-7 GRUP.
    layout = [s for s in field_layout(r) if s != 'TPIC']
    values = {'EDID': [r['edid'].encode('ascii')+b'\0'],
              'ENAM': [struct.pack('<HH', r['flags'], 0)], 'PNAM': [struct.pack('<I', previous)],
              'TCLT': [struct.pack('<I', p.ids[t]) for t in r['links']],
              'TRDT': [bytes(12)+bytes([i])+bytes(11) for i in range(1,len(r['segments'])+1)],
              'NAM1': [s.encode('cp1252')+b'\0' for s in r['segments']],
              'RNAM': [] if r['prompt'] is None else [r['prompt'].encode('cp1252')+b'\0']}
    # Existing effect and condition bytes are compared independently below.
    for tag in ('VMAD','CTDA','CIS2'): values[tag] = p.values(fid,tag)
    offsets=Counter(); result=[]
    for tag in layout:
        result.append((tag,values[tag][offsets[tag]])); offsets[tag]+=1
    return result


def audit_topology(p, plan):
    records=plan['records']; by={r['edid']:r for r in records}
    require(set(by)<=set(p.ids),'Missing topology identities')
    roots=plan['rootTopics']; top=[]
    for r in records:
        fid=p.ids[r['edid']]
        if r['signature']=='DLBR':
            start=p.integer(fid,'SNAM'); require(start in p.fields,'Missing Starting Topic')
            require(p.integer(fid,'DNAM')==r['flags'],'Branch topology flags')
            require(start==p.ids[r['start']],'Wrong Starting Topic')
            if r['flags']==1: top.append(start)
        if r['signature']!='INFO': continue
        require(p.parents[fid]==p.ids[r['topic']],'INFO owning GRUP mismatch')
        require(not p.values(fid,'TPIC'),'Previous Topic is not ownership')
        response=p.values(fid,'TRDT'); lines=p.values(fid,'NAM1')
        require(len(response)==len(lines)>0,'Navigation/starting INFO has no usable Response')
        require([v[12] for v in response]==list(range(1,len(response)+1)), 'Invalid response numbering')
        flags=p.integer(fid,'ENAM')
        require(not flags&0x40, 'Invisible Continue is forbidden on these player choices')
        if r.get('navigation'):
            require(lines==[b' \0'] and flags==r['flags'] and not flags&0x204,
                    'Navigation must be invisible text, repeatable, without Force Subtitle')
        require(conditions(p,fid)==resolve_conditions(r['conditions'],p.ids),'Navigation/narrative CTDA changed')
    require(len(top)==11 and set(top)=={p.ids[t] for t in roots.values()}, 'Subtopic leaks into root / missing root')
    for sig in ('DLBR','DIAL'):
        actual={r.form_id for r in p.records if r.signature==sig and p.integer(r.form_id,'QNAM')==p.ids[OWNER]}
        require(actual=={p.ids[r['edid']] for r in records if r['signature']==sig}, 'Unplanned dialogue owner topology')
    previous={}
    for r in records:
        if r['signature']!='INFO':continue
        fid=p.ids[r['edid']]; tid=p.ids[r['topic']]
        require(p.fields[fid]==expected_info_fields(p,r,previous.get(tid,0)), 'INFO layout/links/response mismatch: '+r['edid'])
        previous[tid]=fid
    # Runtime eligibility is evaluated from actual CTDA, not an unconditional
    # assumption that every logical edge is an engine-visible topic.
    for stage in (0,9,10,19,20,30,65535):
        for projection in range(22):
            def context(c):
                if c['function']==72:return 1
                if c['function']==58:return stage
                if c['function']==59 and c['ref']==p.ids[OWNER]:return int(c['argument']==100+projection)
                raise ValueError('Unexpected root condition')
            for node,topic in roots.items():
                infos=[r for r in records if r.get('topic')==topic]
                selected=[r for r in infos if evaluate(conditions(p,p.ids[r['edid']]),context)]
                require(len(selected)==1,'Root does not expose exactly one INFO: '+node)
                if topic==HELGEN_MENU:
                    index=0 if stage<10 else 1 if stage<20 else 2
                    require(selected[0]['edid'].endswith(f'_NAV_{index}'),'Helgen stage partition')
    audit_semantics(p,plan,frozen_corpus())
    return dict(rootCategories=11,topLevelBranches=11,internalBranches=6,
                navigationInfos=sum(bool(r.get('navigation')) for r in records),
                rootStageClassContexts=154,classContexts=22,survivorContexts=100,
                engineRuntime='NOT_TESTED')


def audit(before, candidate, *, fixture=False):
    plan=build_navigation_plan(frozen_corpus()); old,p=Plugin(before),Plugin(candidate)
    require(fixture or hashlib.sha256(before).hexdigest().upper()==BASELINE_SHA256,'Wrong deployed D baseline')
    require([text(v) for v in p.values(0,'MAST')]==MASTERS,'Master order')
    expected={r['edid']:r for r in plan['records']}; created={n for n,r in expected.items() if r['action']=='create'}
    require(set(p.ids)-set(old.ids)==created and set(old.ids)<=set(p.ids),'Unexpected added/removed identity')
    require(len(p.records)==len(old.records)+4,'Expected exactly four adapter records')
    oldrecs={r.form_id:r for r in old.records}
    require(set(old.fields)<=set(p.fields),'Existing record removed')
    added=set(p.fields)-set(old.fields)
    require(added=={p.ids[n] for n in created},'Unexpected anonymous record')
    require(p.one(0,'HEDR')[:4]==old.one(0,'HEDR')[:4],'HEDR version')
    def count_entries(start,end):
        count=0; pos=start
        while pos<end:
            size=struct.unpack_from('<I',candidate,pos+4)[0]
            if candidate[pos:pos+4]==b'GRUP':
                count+=1+count_entries(pos+24,pos+size); pos+=size
            else:count+=int(pos!=0); pos+=24+size
        return count
    require(struct.unpack_from('<I',p.one(0,'HEDR'),4)[0]==count_entries(0,len(candidate)), 'HEDR entry count')
    require(struct.unpack_from('<I',p.one(0,'HEDR'),8)[0]>max(fid&0xFFFFFF for fid in p.fields if fid>>24==3), 'HEDR next object')
    require([(s,v) for s,v in p.fields[0] if s!='HEDR']==
            [(s,v) for s,v in old.fields[0] if s!='HEDR'],'Header changed')
    planned_fids={old.ids[n] for n in expected if n in old.ids}
    for r in p.records:
        if r.form_id in oldrecs:
            a=oldrecs[r.form_id]
            require((r.signature,r.flags)==(a.signature,a.flags),'Record signature/flags changed')
            require(candidate[r.offset+20:r.offset+24]==before[a.offset+20:a.offset+24],'Form version changed')
            if r.form_id and r.form_id not in planned_fids:
                require(p.fields[r.form_id]==old.fields[r.form_id] and
                        p.parents[r.form_id]==old.parents[r.form_id], 'Unrelated record changed')
        else:
            require(r.form_id>>24==3 and r.flags==0 and
                    candidate[r.offset+20:r.offset+24]==struct.pack('<HH',44,0),'New record format/ownership')
    changed=[]
    for name,fid in old.ids.items():
        require(p.ids[name]==fid,'Existing FormID changed')
        spec=expected.get(name)
        if not spec or spec['signature']=='QUST':
            require(p.fields[fid]==old.fields[fid],'Unrelated record changed: '+name)
        elif spec['signature']=='INFO':
            for tag in ('CTDA','CIS2','VMAD'):
                require(p.values(fid,tag)==old.values(fid,tag),'Existing conditions/effect changed: '+name)
            if not spec['navigation']:
                for tag in ('NAM1','TRDT','ENAM','RNAM'):
                    require(p.values(fid,tag)==old.values(fid,tag),'Spoken response changed: '+name)
        elif spec['signature']=='DLBR':
            allowed={'DNAM','SNAM'}
            require([(s,v) for s,v in p.fields[fid] if s not in allowed]==[(s,v) for s,v in old.fields[fid] if s not in allowed],'Branch ownership changed')
        elif spec['signature']=='DIAL':
            require([(s,v) for s,v in p.fields[fid] if s!='TIFC']==[(s,v) for s,v in old.fields[fid] if s!='TIFC'],'Existing topic changed')
        if not spec or spec['signature']!='INFO':require(p.parents[fid]==old.parents[fid],'Unrelated parent changed')
        if p.fields[fid]!=old.fields[fid] or p.parents[fid]!=old.parents[fid]:changed.append(name)
    for r in plan['records']:
        fid=p.ids[r['edid']]
        actual=next(x for x in p.records if x.form_id==fid)
        require(actual.signature==r['signature'] and fid>>24==3,'Planned signature/ownership')
        if r['signature']=='DIAL':
            require(p.integer(fid,'QNAM')==p.ids[OWNER] and p.integer(fid,'BNAM')==p.ids[r['branch']],'DIAL owner/branch')
            require(text(p.one(fid,'FULL'))==r['text'],'Player prompt changed')
            require(p.integer(fid,'TIFC')==sum(x.get('topic')==r['edid'] for x in plan['records']),'INFO count')
            require([s for s,v in p.fields[fid]]==field_layout(r),'DIAL field layout')
            if r['action']=='create':
                require(p.one(fid,'PNAM')==struct.pack('<f',50) and p.one(fid,'DATA')==bytes(4)
                        and p.one(fid,'SNAM')==b'CUST','New DIAL configuration')
        if r['signature']=='INFO':
            for line in p.values(fid,'NAM1')+p.values(fid,'RNAM'):
                require(not set(text(line))&REPLACEMENTS.keys(),'Unsafe typography')
    result=audit_topology(p,plan)
    return dict(success=True,operation='FixValenNavigation',sha256=hashlib.sha256(candidate).hexdigest().upper(),
                records=len(p.records),created=sorted(created),updated=sorted(changed),companionsChanged=0,
                corpusSha256=CORPUS_SHA256,editorialSha256=EDITORIAL_SHA256,**result)


def render():
    plan=build_navigation_plan(frozen_corpus()); lines=[]; emit=lines.append; q=literal
    # Allocate all new topics/INFO first so child GRUPs exist before moving the
    # existing two stage routers; no FormID is supplied by this Python adapter.
    for r in plan['records']:
        if r['signature']=='DIAL' and r['action']=='create':
            emit(f"t := NewTopic({q(r['edid'])},{q(r['branch'])},{q(r['text'])});")
    for r in plan['records']:
        if r['signature']=='INFO' and r['action']=='create':
            emit(f"e := NewInfo({q(r['edid'])},{q(r['topic'])});")
    for r in plan['records']:
        if r['signature']=='DLBR':
            emit(f"e := FindByEditorID({q(r['edid'])}); SetElementNativeValues(e,'DNAM',{r['flags']});")
            emit(f"Link(e,'SNAM',FindByEditorID({q(r['start'])}));")
    previous={}
    for r in plan['records']:
        if r['signature']!='INFO':continue
        emit(f"e := FindByEditorID({q(r['edid'])}); t := FindByEditorID({q(r['topic'])});")
        emit("MoveInfo(e,t); ClearField(e,'TPIC'); ClearField(e,'Link To');")
        emit(f"SetElementNativeValues(e,'ENAM\\Flags',{r['flags']});")
        emit("Add(e,'PNAM',True);")
        if r['topic'] in previous:emit(f"Link(e,'PNAM',FindByEditorID({q(previous[r['topic']])}));")
        else:emit("SetElementNativeValues(e,'PNAM',0);")
        previous[r['topic']]=r['edid']
        for target in r['links']:emit(f"AddTopicLink(e,{q(target)});")
        if r['navigation']:
            emit("ClearField(e,'Responses'); AddResponse(e,1,' ');")
        if r['action']=='create':
            for c in r['conditions']:
                emit(f"AddCondition(e,{q(c['function'])},{q(c['ref'])},{float(c['value'])},{c['operator']});")
            if r['prompt'] is not None:emit(f"Add(e,'RNAM',True); SetElementEditValues(e,'RNAM',{q(r['prompt'])});")
    for topic,count in Counter(r['topic'] for r in plan['records'] if r['signature']=='INFO').items():
        emit(f"SetElementNativeValues(FindByEditorID({q(topic)}),'TIFC',{count});")
    template=(Path(__file__).parent/'xedit/STRE_ValenNavigation.pas').read_text('utf-8')
    return template.replace('{ GENERATED_NAVIGATION }','\n  '.join(lines))


def manifest_for(before_manifest, candidate):
    result=deepcopy(before_manifest); p=Plugin(candidate)
    require(result['pluginName']=='STRE_AlternateStart.esp','Wrong records manifest')
    existing={r['editorId'] for r in result['records']}
    for r in build_navigation_plan(frozen_corpus())['records']:
        if r['action']!='create':continue
        require(r['edid'] not in existing,'Adapter record already in baseline manifest')
        entry=dict(signature=r['signature'],editorId=r['edid'],
                   expectedLocalFormId=f"0x{p.ids[r['edid']] & 0xFFFFFF:08X}")
        if r['signature']=='DIAL':entry['displayName']=r['text']
        result['records'].append(entry)
    return result


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--root',type=Path,required=True)
    parser.add_argument('--variant',choices=('A',),default='A')
    parser.add_argument('--audit',action='store_true'); parser.add_argument('--saved',action='store_true')
    parser.add_argument('--fixture',action='store_true')
    args=parser.parse_args(); root=args.root.resolve()
    require(root.parent==(corpus.ROOT/'_audit/modding').resolve(),'Private session required')
    before=(root/'before.esp').read_bytes()
    if args.audit:
        candidate=(root/('Data/STRE_AlternateStart.esp' if args.saved else 'authored.esp')).read_bytes()
        report=audit(before,candidate,fixture=args.fixture)
        (root/('saved-navigation-audit.json' if args.saved else 'navigation-audit.json')).write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
        if not args.fixture:
            manifest=manifest_for(json.loads((root/'records-before.json').read_text('utf-8-sig')),candidate)
            path=root/'candidate-manifest.json'
            if args.saved:
                require(json.loads(path.read_text('utf-8'))==manifest,'Candidate manifest drift after review')
            else:path.write_text(json.dumps(manifest,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    else:
        require(args.fixture or hashlib.sha256(before).hexdigest().upper()==BASELINE_SHA256,'Wrong deployed D baseline')
        plan=build_navigation_plan(frozen_corpus())
        (root/'navigation-plan.json').write_text(json.dumps(plan,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
        (root/'author.ssepas').write_text(render(),encoding='utf-8-sig')
        report=dict(success=True,operation=plan['operation'],counts=plan['counts'],actions=plan['actions'])
    print(json.dumps({k:v for k,v in report.items() if k!='updated'}))


if __name__=='__main__':main()
