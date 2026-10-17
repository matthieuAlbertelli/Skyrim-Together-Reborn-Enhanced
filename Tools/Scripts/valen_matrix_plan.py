"""Bounded #96 record plan. No ESP serialization or FormID allocation here."""
from __future__ import annotations

import argparse
from collections import Counter
import hashlib
import json
from pathlib import Path

import valen_matrix as corpus
from valen_helgen import OWNER, NARRATIVE, BRANCH, NEWS, ACCEPT, NEWS_INFO, ACCEPT_INFO

TECHNICAL = 'STRE_QUEST_HelgenInvestigation'
NPC = 'STRE_NPC_MessireValen'
GENERIC = 'STRE_DIAL_Valen_Helgen'
LEGACY = {'VALEN_HELGEN_NEWS': (NEWS, NEWS_INFO), 'VALEN_HELGEN_ACCEPT': (ACCEPT, ACCEPT_INFO)}
OPS = {'Equals': 0, 'NotEquals': 32, 'GreaterThan': 64, 'GreaterOrEqual': 96, 'LessThan': 128, 'LessOrEqual': 160}


def condition(function, ref, value, op='Equals', argument=None):
    return dict(function=function, ref=ref, value=value, operator=OPS[op], argument=argument)


def quest_stage(value, op='Equals', quest=NARRATIVE):
    return condition('GetStage', quest, value, op)


def done(stage, value, quest=NARRATIVE):
    return condition('GetStageDone', quest, value, argument=stage)


def physical(name, values):
    # Supported sets are contiguous; two AND comparisons avoid fragile OR groups.
    assert values == list(range(min(values), max(values) + 1))
    variable = f'::{name}State_var'
    if len(values) == 1:
        return [condition('GetVMQuestVariable', TECHNICAL, values[0], argument=variable)]
    return [condition('GetVMQuestVariable', TECHNICAL, min(values), 'GreaterOrEqual', variable),
            condition('GetVMQuestVariable', TECHNICAL, max(values), 'LessOrEqual', variable)]


def survivor_condition(rule):
    return [quest_stage(20, 'GreaterOrEqual'), done(11, int(rule['hadvarDiscovered'])),
            done(12, int(rule['ralofDiscovered']))] + physical('Hadvar', rule['hadvarStates']) + physical('Ralof', rule['ralofStates'])


def build_plan(data):
    corpus.require_authoring_ready(data)
    nodes = {n['id']: n for n in data['nodes'] if n['state'] == 'approved'}
    classes = {c['id']: c['projection'] for c in data['classes']}
    rules = data['survivors']['rules']
    root = nodes['VALEN_ROOT']['followups']
    explicit = ['VALEN_HELGEN_ARRIVAL_010', 'VALEN_SURVIVOR_NONE', 'VALEN_SURVIVOR_HADVAR',
                'VALEN_SURVIVOR_RALOF', 'VALEN_SURVIVOR_BOTH', 'VALEN_SURVIVOR_BOTH_SAFE']

    def topic(id_):
        if id_ in LEGACY:
            return LEGACY[id_][0]
        if id_ == 'VALEN_MENU_HELGEN' or nodes[id_].get('route') == 'generic-survivor-topic':
            return GENERIC
        return 'STRE_DIAL_' + id_

    def top(id_):
        while nodes[id_]['parent'] not in (None, 'VALEN_ROOT'):
            id_ = nodes[id_]['parent']
        return id_

    def branch(id_):
        if id_ in explicit:
            return 'STRE_DLBR_' + id_
        top_id = top(id_)
        return BRANCH if top_id == 'VALEN_MENU_HELGEN' else 'STRE_DLBR_' + top_id

    def compile_conditions(conditions):
        result = []
        for c in conditions:
            kind, op, val = c['kind'], c['operator'], c['value']
            if kind == 'QuestStage':
                result.append(quest_stage(val, op))
            elif kind in ('HadvarDiscovered', 'RalofDiscovered'):
                result.append(done(11 if kind == 'HadvarDiscovered' else 12, int(val)))
            elif kind == 'ClassId':
                if op == 'Unknown':
                    # Unknown/fresh saves never acquire a known-class stage.
                    result.extend(done(100 + i, 0, OWNER) for i in range(1, 22))
                else:
                    assert op == 'Equals'
                    result.append(done(100 + classes[val], 1, OWNER))
            elif kind == 'RosterSize':
                assert (op, val) in (('Equals', 1), ('GreaterThan', 1))
                result.append(done(50, int(op == 'GreaterThan'), OWNER))
            else:
                raise ValueError('Unsupported condition: ' + kind)
        return result

    def links(node):
        if node['kind'] == 'exit':
            return []
        ids = list(node['followups'])
        parent = node['returnTo']
        if parent and parent != 'VALEN_ROOT':
            ids += nodes[parent]['followups']
        ids += root + explicit
        return sorted({topic(i) for i in ids if topic(i) != topic(node['id'])})

    records = [dict(signature='QUST', edid=NARRATIVE, action='update', stages=[0,10,11,12,20],
                    objective20=data['objective20'], alias=0),
               dict(signature='QUST', edid=OWNER, action='update', stages=[0,50] + list(range(100,122)))]
    for id_ in root + explicit:
        records.append(dict(signature='DLBR', edid=branch(id_), action='update' if branch(id_) == BRANCH else 'create',
                            owner=OWNER, start=topic(id_), flags=1))
    topics = {}
    infos = []
    speaker = condition('GetIsID', NPC, 1)
    for id_, node in nodes.items():
        if id_ == 'VALEN_ROOT':
            continue
        tid = topic(id_)
        if tid not in topics:
            topics[tid] = dict(signature='DIAL', edid=tid, action='update' if id_ in LEGACY else 'create',
                               owner=OWNER, branch=branch(id_), text=node['playerText'])
        variants = []
        if id_ == 'VALEN_MENU_HELGEN':
            for i, label in enumerate(data['helgenMenuLabels'][:2]):
                variants.append((f'NAV_{i}', [], [quest_stage(label['minimumStage'], 'GreaterOrEqual'),
                                 quest_stage(label['maximumStage'], 'LessOrEqual')], label['text']))
            topics[tid]['text'] = data['survivors']['genericPlayerText']
        elif node.get('route') == 'generic-survivor-topic' or id_ in data['survivors']['firstReports'].values():
            for rule in rules:
                if rule['node'] == id_:
                    variants.append((rule['id'].replace('-', '_').upper(), node['responses'], survivor_condition(rule), None))
        elif id_ == 'VALEN_SURVIVOR_BOTH_SAFE':
            variants.append(('R', node['responses'], [quest_stage(20, 'GreaterOrEqual'), done(11,1), done(12,1)]
                            + physical('Hadvar',[3,4]) + physical('Ralof',[3,4]), None))
        elif len(node['responses']) > 1:
            for response in node['responses']:
                variants.append((response['id'], ([node['leadIn']] if 'leadIn' in node else []) + [response],
                                 compile_conditions(node['conditions'] + response['conditions']), None))
        else:
            conditions = compile_conditions(node['conditions'])
            if node['parent'] in data['survivors']['firstReports'].values():
                parent_rules = [r for r in rules if r['node'] == node['parent']]
                assert len(parent_rules) == 1
                conditions = survivor_condition(parent_rules[0])
            elif node['parent'] == 'VALEN_SURVIVOR_BOTH_SAFE':
                conditions += [done(11,1), done(12,1)] + physical('Hadvar',[3,4]) + physical('Ralof',[3,4])
            variants.append(('R', node['responses'], conditions, None))
        for suffix, responses, conditions, prompt in variants:
            edid = LEGACY[id_][1] if id_ in LEGACY else 'STRE_INFO_' + id_ + '_' + suffix
            # Existing #95 response segmentation and assets remain byte-faithful.
            segments = ([r['text'] for r in responses] if id_ in LEGACY else
                        [s for r in responses for s in r['segments']])
            if id_ == 'VALEN_HELGEN_NEWS':
                segments = ['Des nouvelles nous sont parvenues de Helgen. La ville aurait été attaquée par un dragon.',
                            "Les récits sont confus, mais quelque chose de grave s'y est produit."]
            infos.append(dict(signature='INFO', edid=edid, action='update' if id_ in LEGACY else 'create',
                              node=id_, topic=tid, responseIds=[r['id'] for r in responses], segments=segments,
                              conditions=[speaker] + conditions, links=links(node), prompt=prompt,
                              flags=0xA00 | (1 if node['kind']=='exit' else 0) |
                                    (4 if node['repeatability']=='OncePerLocalSave' else 0),
                              fragment='legacy-acceptance' if id_=='VALEN_HELGEN_ACCEPT' else None))
    # Preserve the approved specificity order in the actual INFO/PNAM chain,
    # independently of the editorial node order. Conditions remain disjoint.
    ranks = {'STRE_INFO_'+r['node']+'_'+r['id'].replace('-','_').upper(): i for i,r in enumerate(rules)}
    generic = sorted((r for r in infos if r['topic']==GENERIC),
                     key=lambda r: -2 if r['edid'].endswith('_NAV_0') else
                                   -1 if r['edid'].endswith('_NAV_1') else ranks[r['edid']])
    ordered = iter(generic)
    infos = [next(ordered) if r['topic']==GENERIC else r for r in infos]
    records += list(topics.values()) + infos
    ids = [r['edid'] for r in records]
    corpus.require(len(ids) == len(set(ids)), 'Duplicate planned EditorID')
    corpus.require(all(i.startswith('STRE_') for i in ids), 'Unprefixed planned record')
    corpus.require(all(r['start'] in topics for r in records if r['signature']=='DLBR'), 'Missing branch start')
    corpus.require(all(set(r['links']) <= topics.keys() for r in infos), 'Unresolved graph link')
    return dict(schemaVersion=1, operation='AddValenDialogueMatrixV1', plugin='STRE_AlternateStart.esp',
                corpusSha256=data['source']['editorialSha256'], allocation='official xEdit only',
                masters=['Skyrim.esm','Update.esm','Dragonborn.esm'],
                coverage=corpus.validate(data)['survivorResults'], classes=data['classes'], audio=data['audio'],
                records=records, counts=dict(Counter(r['signature'] for r in records)),
                actions=dict(Counter(r['action'] for r in records)))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    plan = build_plan(corpus.read_json(corpus.CORPUS))
    args.output.write_text(json.dumps(plan,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    print(json.dumps(dict(success=True, counts=plan['counts'], actions=plan['actions'],
                         sha256=hashlib.sha256(args.output.read_bytes()).hexdigest().upper())))


if __name__ == '__main__':
    main()
