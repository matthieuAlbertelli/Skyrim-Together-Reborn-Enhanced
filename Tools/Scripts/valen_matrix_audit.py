"""Read #96 xEdit output independently. Strict records, text, graph, CTDA and VMAD.

This module never serializes an ESP. Unknown fields/records and unrelated edits
are errors; a successful audit is not a human xEdit or runtime attestation.
"""
from __future__ import annotations
import argparse
import hashlib
import json
from pathlib import Path
import struct

import valen_matrix as corpus
from valen_matrix_plan import build_plan, OWNER, NARRATIVE, TECHNICAL, ACCEPT_INFO, GENERIC, NPC
from valen_helgen import Plugin, Vmad, text, require, MASTERS

FUNCTIONS = {'GetIsID':72, 'GetStage':58, 'GetStageDone':59, 'GetVMQuestVariable':629}
ALLOWED = {
    'DLBR': set('EDID QNAM TNAM DNAM SNAM'.split()),
    'DIAL': set('EDID FULL PNAM BNAM QNAM DATA SNAM TIFC'.split()),
    'INFO': set('EDID VMAD ENAM TPIC PNAM TCLT TRDT NAM1 CTDA CIS2 RNAM'.split()),
    'QUST': set('EDID VMAD FULL DNAM NEXT INDX QSDT CNAM QOBJ FNAM NNAM ANAM ALST ALID ALFR ALED'.split()),
}


def field_layout(record):
    """Exact field multiplicity/order, including stage-to-log associations."""
    sig = record['signature']
    if sig=='DLBR':
        return 'EDID QNAM TNAM DNAM SNAM'.split()
    if sig=='DIAL':
        return 'EDID FULL PNAM BNAM QNAM DATA SNAM TIFC'.split()
    if sig=='QUST':
        if record['edid']==OWNER:
            return ['EDID','DNAM','NEXT']+['INDX','QSDT']*len(record['stages'])+['ANAM']
        return ('EDID VMAD FULL DNAM NEXT INDX INDX QSDT CNAM '+
                'INDX QSDT INDX QSDT INDX QSDT QOBJ FNAM NNAM QOBJ FNAM NNAM '+
                'ANAM ALST ALID FNAM ALFR ALED').split()
    fields=['EDID']+(['VMAD'] if record['edid']==ACCEPT_INFO else [])+['ENAM','TPIC','PNAM']
    fields+=['TCLT']*len(record['links'])+['TRDT','NAM1']*len(record['segments'])
    for condition in record['conditions']:
        fields+=['CTDA']+(['CIS2'] if condition['function']=='GetVMQuestVariable' else [])
    return fields+(['RNAM'] if record['prompt'] is not None else [])


class QuestVmad(Vmad):
    def finish(self):
        require(self.num('B') == 2, 'QF bind version')
        count, self.filename = self.num('H'), self.string()
        self.fragments = []
        for _ in range(count):
            stage, unused, index, unknown = self.num('H'), self.num('H'), self.num('I'), self.num('B')
            # xEdit defines the final byte as Unknown, not an OnBegin flag.
            # Preserve the zero used by the existing validated stage-10 binding.
            require(unused == 0 and index == 0 and unknown == 0, 'QF fragment metadata')
            self.fragments.append((stage,self.string(),self.string()))
        require(self.num('H') == 1, 'One player alias VMAD required')
        require(self.num('H') == 0 and self.num('h') == 0, 'Player alias binding index')
        self.quest = self.num('I')
        # Alias script block uses the same version/object format/scripts layout.
        alias = Vmad(self.data[self.pos:])
        require(alias.pos == len(alias.data), 'Unexpected alias VMAD tail')
        self.alias_scripts = alias.scripts
        self.pos = len(self.data)
        return self


def conditions(p, fid):
    result = []
    for sig, raw in p.fields[fid]:
        if sig == 'CTDA':
            require(len(raw) == 32 and raw[1:4] == bytes(3) and raw[10:12] == bytes(2), 'CTDA layout')
            require(raw[20:28] == bytes(8), 'Condition must run on Subject, without a reference override')
            require(raw[28:] in (bytes(4), b'\xff'*4), 'Unexpected CTDA parameter 3')
            result.append(dict(function=struct.unpack_from('<H',raw,8)[0],
                               operator=raw[0],value=struct.unpack_from('<f',raw,4)[0],
                               ref=struct.unpack_from('<I',raw,12)[0],
                               argument=struct.unpack_from('<I',raw,16)[0]))
        elif sig == 'CIS2':
            require(result and result[-1]['function']==629 and type(result[-1]['argument']) is int,
                    'Orphan/duplicate condition variable')
            result[-1]['argument'] = text(raw)
    return result


def resolve_conditions(expected, ids):
    return [dict(function=FUNCTIONS[c['function']],operator=c['operator'],value=float(c['value']),
                 ref=ids[c['ref']],argument=c['argument'] if c['argument'] is not None else 0) for c in expected]


def evaluate(conditions_, context):
    for c in conditions_:
        value = context(c)
        compare = c['value']
        match = {0:value==compare,32:value!=compare,64:value>compare,96:value>=compare,128:value<compare,160:value<=compare}
        require(c['operator'] in match, 'Unsupported condition flags/OR')
        if not match[c['operator']]:
            return False
    return True


def audit_semantics(p, plan, data):
    """Evaluate actual serialized CTDA over all 100 combinations, not plan order."""
    infos = [r for r in plan['records'] if r['signature']=='INFO' and r['topic']==GENERIC]
    for row in corpus.survivor_rows(data):
        def context(c):
            if c['function']==72:
                return 1
            if c['function']==58:
                return 20
            if c['function']==59:
                return row['HadvarDiscovered'] if c['argument']==11 else row['RalofDiscovered']
            require(c['function']==629,'Unexpected survivor input')
            return row['HadvarState'] if c['argument']=='::HadvarState_var' else row['RalofState']
        selected = [r['node'] for r in infos if evaluate(conditions(p,p.ids[r['edid']]),context)]
        expected = [row['genericINFO']] if row['result']=='automatic' else []
        require(selected==expected,f'Actual survivor CTDA coverage/overlap: {row}: {selected}')
    for projection in range(22):
        def context(c):
            if c['function']==72:
                return 1
            require(c['function']==59 and c['ref']==p.ids[OWNER], 'Unexpected class input')
            return int(c['argument']==100+projection)
        classes = [r for r in plan['records'] if r['signature']=='INFO' and r.get('node')=='VALEN_MENU_RECRUIT']
        selected = [r for r in classes if evaluate(conditions(p,p.ids[r['edid']]),context)]
        require(len(selected)==1,'Actual class partition/fallback')
        expected = 'VALEN_CLASS_FALLBACK' if projection==0 else 'VALEN_CLASS_'+data['classes'][projection-1]['id'][6:].upper()
        require(expected in selected[0]['responseIds'],'Actual class mapping')


def audit(before, candidate, plan=None):
    data = corpus.read_json(corpus.CORPUS)
    canonical = build_plan(data)
    require(plan is None or plan == canonical, 'Plan differs from approved bounded operation')
    plan = canonical
    old, p = Plugin(before), Plugin(candidate)
    expected = {r['edid']: r for r in plan['records']}
    for plugin in (old,p):
        require([text(v) for v in plugin.values(0,'MAST')] == MASTERS,'Master order')
    require(p.records[0].flags == old.records[0].flags and not p.records[0].flags & 0x200,'TES4 flags/ESL')
    require(candidate[20:24] == before[20:24],'TES4 version')
    require(p.one(0,'HEDR')[:4] == old.one(0,'HEDR')[:4],'HEDR format version')
    header_fields = lambda plugin: [(s,v) for s,v in plugin.fields[0] if s!='HEDR']
    require(header_fields(p)==header_fields(old),'Unexpected header field change')
    changed = {old.ids[name] for name,r in expected.items() if r['action']=='update'}
    old_records = {r.form_id:r for r in old.records}
    for r in old.records[1:]:
        match = next((c for c in p.records if c.form_id==r.form_id),None)
        require(match and match.signature==r.signature and match.flags==r.flags,'Existing identity/signature/flags changed')
        require(candidate[match.offset+20:match.offset+24]==before[r.offset+20:r.offset+24],
                'Existing form version changed')
        if r.form_id not in changed:
            require(p.fields[r.form_id]==old.fields[r.form_id],f'Unrelated record changed: {r.form_id:08X}')
    added = set(p.fields)-set(old.fields)
    require(len(added)==plan['actions']['create'],'Unexpected added record count')
    require(set(p.ids)-set(old.ids)=={r['edid'] for r in plan['records'] if r['action']=='create'},'Unexpected new EditorIDs')
    require(len(p.records)==len(old.records)+len(added),'Unexpected removed record')
    order = {}
    for name,r in expected.items():
        fid = p.ids[name]
        record = next(x for x in p.records if x.form_id==fid)
        require(record.signature==r['signature'] and fid>>24==3,'Planned record signature/ownership')
        require(record.flags==(old_records[fid].flags if fid in old_records else 0),'Unexpected record flags')
        require(not {s for s,_ in p.fields[fid]}-ALLOWED[r['signature']],f'Unexpected field: {name}')
        if r['action']=='update':
            require(fid==old.ids[name],'Existing FormID changed')
        else:
            require(candidate[record.offset+20:record.offset+24]==struct.pack('<HH',44,0),
                    'New record must use Skyrim SE form version 44')
        if r['signature']=='DLBR':
            require(p.integer(fid,'QNAM')==p.ids[OWNER] and p.integer(fid,'TNAM')==0 and p.integer(fid,'DNAM')==1
                    and p.integer(fid,'SNAM')==p.ids[r['start']],'Branch ownership/start')
        elif r['signature']=='DIAL':
            require(text(p.one(fid,'FULL'))==r['text'],'Player text changed')
            require(p.integer(fid,'QNAM')==p.ids[OWNER] and p.integer(fid,'BNAM')==p.ids[r['branch']],'Topic ownership')
            require(p.one(fid,'DATA')==bytes(4) and p.one(fid,'SNAM')==b'CUST','Topic category/type')
            require(p.one(fid,'PNAM')==struct.pack('<f',50),'Topic priority')
            require(p.integer(fid,'TIFC')==sum(x.get('topic')==name for x in plan['records']),'INFO count')
        elif r['signature']=='INFO':
            parent = p.ids[r['topic']]
            require(p.parents[fid]==parent and p.integer(fid,'TPIC')==parent,'INFO parent group')
            require(p.integer(fid,'PNAM')==order.get(parent,0),'INFO previous ordering')
            order[parent]=fid
            require(p.integer(fid,'ENAM')==r['flags'],'INFO flags/Goodbye/SayOnce')
            require([text(v) for v in p.values(fid,'NAM1')]==r['segments'],'French response text/segmentation')
            require([v[12] for v in p.values(fid,'TRDT')]==list(range(1,len(r['segments'])+1)),'Response numbers')
            for response in p.values(fid,'TRDT'):
                require(len(response)==24 and response[:12]==bytes(12) and response[13:]==bytes(11),'Unexpected response metadata')
            require([int.from_bytes(v,'little') for v in p.values(fid,'TCLT')]==[p.ids[t] for t in r['links']],'Graph link mismatch')
            require(([text(v) for v in p.values(fid,'RNAM')])==([] if r['prompt'] is None else [r['prompt']]),'Prompt override')
            require(conditions(p,fid)==resolve_conditions(r['conditions'],p.ids),'CTDA mismatch: '+name)
            if name==ACCEPT_INFO:
                require(p.one(fid,'VMAD')==old.one(fid,'VMAD'),'Acceptance VMAD changed')
            else:
                require(not p.values(fid,'VMAD'),'Unexpected dialogue effect/VMAD')
        elif r['signature']=='QUST':
            require(p.one(fid,'DNAM')==old.one(fid,'DNAM'),'Quest flags/type changed')
            require(p.values(fid,'FULL')==old.values(fid,'FULL'),'Quest title changed')
            stages = p.values(fid,'INDX')
            require([int.from_bytes(v[:2],'little') for v in stages]==r['stages'] and
                    all(len(v)==4 and v[2:]==bytes(2) for v in stages),'Quest stages')
            require(all(v==b'\0' for v in p.values(fid,'QSDT')),'Quest cannot complete/fail')
            require(p.values(fid,'CNAM')==old.values(fid,'CNAM'),'Journal log changed')
            if name==OWNER:
                require(not p.values(fid,'VMAD') and not p.values(fid,'ALST') and not p.values(fid,'QOBJ'),'Local owner must remain journal/script free')
                require(p.integer(fid,'ANAM')==0,'Owner aliases')
            else:
                require([int.from_bytes(v,'little') for v in p.values(fid,'QOBJ')]==[10,20],'Objective indices')
                require(p.values(fid,'NNAM')==old.values(fid,'NNAM')+[r['objective20'].encode('cp1252')+b'\0'],'Objective text')
                require(p.values(fid,'ALST')==[bytes(4)] and p.values(fid,'ALFR')==[struct.pack('<I',0x14)] and
                        p.values(fid,'ALID')==[b'STRE_Valen_LocalPlayer\0'] and p.integer(fid,'ANAM')==1,'Local player alias')
                require(p.values(fid,'FNAM')==[bytes(4)]*3,'Objective/alias flags')
                vm = QuestVmad(p.one(fid,'VMAD')).finish()
                qf = f'QF_STRE_QUEST_ValenHelgen_{fid:08X}'
                require(vm.scripts==[(qf,0,{})] and vm.filename==qf,'QF script identity')
                require(vm.fragments==[(10,qf,'Fragment_0'),(20,qf,'Fragment_1')],'QF fragment bindings')
                require(vm.quest==fid and vm.alias_scripts==[('STRE_ValenHelgenObserver',0,
                        {'Investigation':p.ids[TECHNICAL],'HelgenLocation':0x18A4A})],'Observer alias properties')
        require([s for s,_ in p.fields[fid]]==field_layout(r),'Field order/multiplicity: '+name)
        if r['signature']=='QUST':
            require(p.one(fid,'NEXT')==b'' and all(v==b'' for v in p.values(fid,'ALED')),
                    'Unexpected quest/alias separator payload')
    require(p.integer(p.ids[NPC],'VTCK')==0x9843A,'Valen voice type must remain MaleWarlock')
    audit_semantics(p,plan,data)
    actual_changed = [r['edid'] for r in plan['records'] if r['action']=='update' and
                      p.fields[p.ids[r['edid']]] != old.fields[old.ids[r['edid']]]]
    keys = [dict(signature=r['signature'],editorId=r['edid'],action=r['action'],
                 payloadChanged=r['action']=='create' or r['edid'] in actual_changed,
                 formKey=f"{p.ids[r['edid']] & 0xFFFFFF:06X}:STRE_AlternateStart.esp") for r in plan['records']]
    return dict(success=True,scope='Independent static candidate audit; HUMAN_XEDIT_REQUIRED',
                counts=plan['counts'],actions=plan['actions'],coverage=plan['coverage'],records=keys,
                actualChanges=dict(created=len(added),updated=len(actual_changed),
                                   unchangedPlannedUpdates=plan['actions']['update']-len(actual_changed)),
                QF=f'QF_STRE_QUEST_ValenHelgen_{p.ids[NARRATIVE]:08X}',
                sha256=hashlib.sha256(candidate).hexdigest().upper())


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--before',type=Path,required=True)
    parser.add_argument('--candidate',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args()
    report=audit(args.before.read_bytes(),args.candidate.read_bytes())
    args.output.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    print(json.dumps({k:v for k,v in report.items() if k!='records'}))


if __name__=='__main__':
    main()
