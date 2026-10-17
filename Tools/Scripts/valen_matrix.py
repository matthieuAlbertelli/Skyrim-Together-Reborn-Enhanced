"""Validate the complete #96 editorial corpus and enumerate survivor conditions.

This preparatory tool never writes an ESP, Papyrus, or an asset. --for-authoring
fails closed if the frozen corpus has any uncovered valid states or overlaps.
The small schema interpreter implements only the keywords used by our checked-in
schema; an unknown keyword is an error, never a silently ignored constraint.
"""
from __future__ import annotations

import argparse
from collections import Counter
import csv
import hashlib
import io
import itertools
import json
from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[2]
DIRECTORY = ROOT / 'docs/narrative/valen'
CORPUS = DIRECTORY / 'VALEN_DIALOGUE_MATRIX_V1.json'
SCHEMA = DIRECTORY / 'VALEN_DIALOGUE_MATRIX_V1.schema.json'
REVIEW = DIRECTORY / 'VALEN_DIALOGUE_MATRIX_V1.generated.md'
TABLE = DIRECTORY / 'VALEN_SURVIVOR_MATRIX_V1.generated.csv'
TABLE_REVIEW = DIRECTORY / 'VALEN_SURVIVOR_MATRIX_V1.generated.md'


def require(value, message):
    if not value:
        raise ValueError(message)


def read_json(path):
    def unique(pairs):
        result = {}
        for key, value in pairs:
            require(key not in result, f'duplicate JSON key: {key}')
            result[key] = value
        return result
    return json.loads(Path(path).read_text(encoding='utf-8'), object_pairs_hook=unique)


KEYWORDS = {'$schema', '$id', 'title', 'description', '$defs', '$ref', 'type',
            'enum', 'const', 'properties', 'required', 'additionalProperties',
            'items', 'minItems', 'uniqueItems', 'pattern', 'minLength',
            'minimum', 'maximum'}
TYPES = {'object': dict, 'array': list, 'string': str, 'integer': int,
         'number': (int, float), 'boolean': bool, 'null': type(None)}


def validate_schema_definition(schema, root=None):
    root = schema if root is None else root
    require(not set(schema) - KEYWORDS, f'unsupported schema keywords: {set(schema) - KEYWORDS}')
    if '$ref' in schema:
        require(set(schema) == {'$ref'}, 'schema reference must be standalone')
        reference = schema['$ref']
        require(reference.startswith('#/$defs/'), 'external schema reference forbidden')
        require(reference[8:] in root['$defs'], 'unresolved schema reference')
    for group in ('properties', '$defs'):
        for child in schema.get(group, {}).values():
            validate_schema_definition(child, root)
    if 'items' in schema:
        validate_schema_definition(schema['items'], root)


def validate_schema(value, schema, root=None, path='$'):
    root = schema if root is None else root
    if '$ref' in schema:
        return validate_schema(value, root['$defs'][schema['$ref'][8:]], root, path)
    types = schema.get('type', [])
    types = [types] if isinstance(types, str) else types
    if types:
        valid_type = any(isinstance(value, TYPES[t]) and
                         not (isinstance(value, bool) and t in ('integer', 'number')) for t in types)
        require(valid_type, f'{path}: expected {types}')
    if 'const' in schema:
        require(type(value) is type(schema['const']) and value == schema['const'], f'{path}: wrong constant')
    if 'enum' in schema:
        require(any(type(value) is type(v) and value == v for v in schema['enum']), f'{path}: invalid enum')
    if isinstance(value, dict):
        require(set(schema.get('required', [])) <= set(value), f'{path}: missing required field')
        properties = schema.get('properties', {})
        if schema.get('additionalProperties') is False:
            require(set(value) <= set(properties), f'{path}: unexpected fields {set(value) - set(properties)}')
        for key in value.keys() & properties.keys():
            validate_schema(value[key], properties[key], root, path + '.' + key)
    elif isinstance(value, list):
        require(len(value) >= schema.get('minItems', 0), f'{path}: too few items')
        if schema.get('uniqueItems'):
            serialized = [json.dumps(v, sort_keys=True, ensure_ascii=False) for v in value]
            require(len(set(serialized)) == len(value), f'{path}: duplicate items')
        for i, item in enumerate(value):
            validate_schema(item, schema.get('items', {}), root, f'{path}[{i}]')
    elif isinstance(value, str):
        require(len(value) >= schema.get('minLength', 0), f'{path}: empty string')
        if 'pattern' in schema:
            require(re.search(schema['pattern'], value), f'{path}: invalid pattern')
    elif type(value) in (int, float):
        require(value >= schema.get('minimum', float('-inf')), f'{path}: below minimum')
        require(value <= schema.get('maximum', float('inf')), f'{path}: above maximum')


def editorial_hash(data):
    payload = [(n['id'], n['playerText'], [(r['id'], r['text']) for r in n['responses']],
                n.get('leadIn', {}).get('text')) for n in data['nodes']]
    return hashlib.sha256(json.dumps(payload, ensure_ascii=False, separators=(',', ':')).encode()).hexdigest().upper()


def approved_amendment_nodes(data):
    """Verify separately approved additions without weakening original-source coverage."""
    nodes = {n['id']: n for n in data['nodes']}
    amended = set()
    for amendment in data['source']['amendments']:
        ids = amendment['nodeIds']
        require(not amended.intersection(ids), 'duplicate amended node')
        require(set(ids) <= nodes.keys(), 'missing approved amendment node')
        payload = [(id_, [(r['id'], r['text']) for r in nodes[id_]['responses']]) for id_ in ids]
        digest = hashlib.sha256(json.dumps(payload, ensure_ascii=False, separators=(',', ':')).encode()).hexdigest().upper()
        require(digest == amendment['sha256'], 'approved amendment text changed')
        amended.update(ids)
    return amended


def condition_matches(condition, context, classes):
    kind, op, expected = condition['kind'], condition['operator'], condition['value']
    require(kind in context, f'missing condition input: {kind}')
    value = context[kind]
    if op == 'Unknown':
        require(kind == 'ClassId' and expected is None, 'Unknown is only a class fallback')
        return value not in classes
    if op == 'Equals':
        return type(value) is type(expected) and value == expected
    require(type(value) is int and type(expected) is int, 'numeric comparison requires integers')
    if op == 'LessThan':
        return value < expected
    if op == 'GreaterThan':
        return value > expected
    if op == 'GreaterOrEqual':
        return value >= expected
    raise ValueError('unknown condition operator')


def matches(conditions, context, classes):
    return all(condition_matches(c, context, classes) for c in conditions)


def survivor_selection(data, hadvar_discovered, ralof_discovered, hadvar_state, ralof_state):
    """Return the rule result without relying on order to hide overlapping rules."""
    inputs = ((hadvar_discovered, hadvar_state), (ralof_discovered, ralof_state))
    require(all(type(d) is bool and type(s) is int and 0 <= s <= 4 for d, s in inputs), 'invalid survivor input')
    # A not-yet-delivered discovery for a freed/departed actor is not permission
    # to disclose it. Wait for the independent discovery fact first.
    coherent = all((d and s != 0) or (not d and s in (0, 1, 2)) for d, s in inputs)
    matching = [rule for rule in data['survivors']['rules']
                if rule['hadvarDiscovered'] == hadvar_discovered
                and rule['ralofDiscovered'] == ralof_discovered
                and hadvar_state in rule['hadvarStates']
                and ralof_state in rule['ralofStates']]
    require(len(matching) <= 1, f'overlapping survivor conditions: {inputs}')
    if not coherent:
        require(not matching, f'incoherent state must fail closed: {inputs}')
        return {'kind': 'incoherent', 'node': None, 'reason': 'discovery/state mismatch; no INFO'}
    if not matching:
        return {'kind': 'uncovered', 'node': None, 'reason': 'no approved rule for coherent survivor combination'}
    rule = matching[0]
    return {'kind': rule['selection'], 'node': rule['node'], 'reason': rule['id']}


def survivor_rows(data):
    rescued = data['survivors']['bothRescuedPairs']
    for hd, rd, hs, rs in itertools.product((False, True), (False, True), range(5), range(5)):
        selected = survivor_selection(data, hd, rd, hs, rs)
        consistent = selected['kind'] != 'incoherent'
        report = selected['node'] if selected['kind'] == 'explicit' else ''
        yield {'HadvarDiscovered': int(hd), 'RalofDiscovered': int(rd),
               'HadvarState': hs, 'RalofState': rs,
               'genericINFO': selected['node'] if selected['kind'] == 'automatic' else 'UNAVAILABLE',
               'explicitReport': report,
               'bothRescuedTopic': int(consistent and hd and rd and [hs, rs] in rescued),
               'result': selected['kind'], 'reason': selected['reason']}


def survivor_counts(rows):
    counts = Counter(row['result'] for row in rows)
    return {kind: counts[kind] for kind in ('automatic', 'explicit', 'incoherent', 'uncovered')}


def validate(data):
    schema = read_json(SCHEMA)
    validate_schema_definition(schema)
    validate_schema(data, schema)
    require(editorial_hash(data) == data['source']['editorialSha256'], 'frozen editorial text changed')
    amended = approved_amendment_nodes(data)
    nodes = {n['id']: n for n in data['nodes']}
    require(len(nodes) == len(data['nodes']), 'duplicate node ID')
    require(nodes['VALEN_ROOT']['kind'] == 'root', 'missing root')
    classes = {c['id'] for c in data['classes']}
    require(len(classes) == 21, 'exactly 21 unique classes required')
    require(sorted(c['projection'] for c in data['classes']) == list(range(1, 22)), 'unstable class projection')
    response_ids = set()
    for node in nodes.values():
        id_ = node['id']
        if node['kind'] not in ('root', 'exit'):
            require(node['parent'] in nodes and node['returnTo'] in nodes, f'{id_}: missing parent/return')
        if node['kind'] != 'root':
            require(node['playerText'] is not None, f'{id_}: missing player text')
        if node['effect'] != 'None':
            require(id_ == 'VALEN_HELGEN_ACCEPT' and node['scope'] == 'SharedQuest'
                    and node['repeatability'] == 'OnceBeforeStage10', 'unauthorized dialogue effect')
        else:
            require(node['scope'] == 'LocalPresentation', f'{id_}: unexpected shared conversation')
        for target in node['followups']:
            require(target in nodes, f'{id_}: unknown followup {target}')
            require(nodes[target]['state'] == 'approved', f'{id_}: non-approved node reachable')
        if node['state'] == 'approved' and node['kind'] == 'dialogue':
            require(node['responses'], f'{id_}: no response')
        responses = node['responses'] + ([node['leadIn']] if 'leadIn' in node else [])
        for response in responses:
            rid = response['id']
            require(rid not in response_ids, f'duplicate response ID: {rid}')
            response_ids.add(rid)
            require(''.join(response['segments']) == response['text'], f'{rid}: segmentation changed text')
            for segment in response['segments'][:-1]:
                require(segment[-1] in '.!?…', f'{rid}: split inside a sentence')
            for segment in response['segments'][1:]:
                require(segment[0].isspace(), f'{rid}: lost boundary whitespace')
            response['text'].encode('cp1252', errors='strict')
        if node['playerText'] is not None:
            node['playerText'].encode('cp1252', errors='strict')
        if node.get('route') == 'generic-survivor-topic':
            require(node['playerText'] == data['survivors']['genericPlayerText'], 'invented survivor prompt')
            require(node['repeatability'] == 'Repeatable' and node['effect'] == 'None'
                    and node['scope'] == 'LocalPresentation', 'generic survivor reply must be repeatable and have no effect')
        if node['kind'] not in ('root', 'exit'):
            current, visited = node, set()
            while current['kind'] != 'root':
                require(current['id'] not in visited, f'{id_}: return cycle cannot reach root')
                visited.add(current['id'])
                require(current['returnTo'] in nodes, f'{id_}: broken return path')
                current = nodes[current['returnTo']]
    reached, pending = set(), ['VALEN_ROOT']
    while pending:
        id_ = pending.pop()
        if id_ not in reached:
            reached.add(id_)
            pending.extend(nodes[id_]['followups'])
    require(reached == {n['id'] for n in nodes.values() if n['state'] == 'approved'}, 'approved graph is not fully reachable')

    variants = nodes['VALEN_MENU_RECRUIT']['responses']
    require(len(variants) == 22, '21 class variants and one fallback required')
    for class_id in (*sorted(classes), None, '', 'class.unknown', 'class.Warrior'):
        selected = [r for r in variants if matches(r['conditions'], {'ClassId': class_id}, classes)]
        require(len(selected) == 1, f'non-exclusive class variants: {class_id}')
        if class_id not in classes:
            require(selected[0]['id'] == 'VALEN_CLASS_FALLBACK', 'unknown class leaked into a class variant')
    for size in range(1, 11):
        selected = [r for r in nodes['VALEN_GROUP_010']['responses']
                    if matches(r['conditions'], {'RosterSize': size}, classes)]
        require(len(selected) == 1, 'non-exclusive Solo/group variants')
    for stage in range(65536):
        selected = [label for label in data['helgenMenuLabels']
                    if stage >= label['minimumStage'] and
                    (label['maximumStage'] is None or stage <= label['maximumStage'])]
        require(len(selected) == 1, f'non-exclusive stage labels: {stage}')
    rows = list(survivor_rows(data))
    used = {r['genericINFO'] for r in rows} - {'UNAVAILABLE'}
    expected = {n['id'] for n in nodes.values() if n.get('route') == 'generic-survivor-topic'}
    require(used == expected, 'generic survivor reply missing or unexpected')
    for rule in data['survivors']['rules']:
        require(rule['node'] in nodes, 'unknown survivor target')
        text = ' '.join(r['text'] for r in nodes[rule['node']]['responses']).casefold()
        if not rule['hadvarDiscovered']:
            require('hadvar' not in text, 'premature Hadvar disclosure')
        if not rule['ralofDiscovered']:
            require('ralof' not in text, 'premature Ralof disclosure')
        if not rule['hadvarDiscovered'] or not rule['ralofDiscovered']:
            require('l’autre' not in text and 'tous les deux' not in text and 'deux hommes' not in text,
                    'premature disclosure of the other survivor')
    require(nodes['VALEN_SURVIVOR_HADVAR']['repeatability'] == 'OncePerLocalSave' and
            nodes['VALEN_SURVIVOR_RALOF']['repeatability'] == 'OncePerLocalSave' and
            nodes['VALEN_SURVIVOR_BOTH']['repeatability'] == 'OncePerLocalSave', 'first reports must be Say Once')
    return {'nodes': len(nodes), 'responsesIncludingLeadIn': len(response_ids), 'classes': len(classes),
            'approvedAmendmentReplies': sum(len(nodes[id_]['responses']) for id_ in amended),
            'survivorCombinations': len(rows), 'survivorResults': survivor_counts(rows)}


def require_authoring_ready(data):
    validate(data)
    pending = [d['id'] for d in data['authoringDecisions'] if d['status'] == 'pending']
    uncovered = [r for r in survivor_rows(data) if r['result'] == 'uncovered']
    require(not pending and not uncovered,
            f'AUTHORING_BLOCKED: {len(uncovered)} uncovered coherent survivor combinations; pending: {pending}')


def verify_approved_source(data, path):
    raw = Path(path).read_bytes()
    require(hashlib.sha256(raw).hexdigest().upper() == data['source']['sha256'], 'different approved mission source')
    text = raw.decode('utf-8-sig').replace('\r\n', '\n')
    body = text.split('# 19. Corpus français figé\n', 1)[1].split('# 20. Critères', 1)[0]
    quotes = []
    for match in re.finditer(r'(?m)^>[^\n]*(?:\n>[^\n]*)*', body):
        quote = '\n'.join(re.sub(r'^> ?', '', line) for line in match[0].splitlines()).strip()
        require(quote.startswith('« ') and quote.endswith(' »'), 'malformed approved quotation')
        quotes.append(quote[2:-2])
    # The sole player choice written as a blockquote is preserved separately in
    # the existing ESP's encoding. It is not another spoken Valen response.
    quotes.remove('Nous irons peut-être voir ce qui s’y est passé.')
    actual = []
    amended = approved_amendment_nodes(data)
    for node in data['nodes']:
        if node['id'] not in {'VALEN_HELGEN_NEWS', 'VALEN_HELGEN_ACCEPT'} | amended:
            actual.extend(r['text'] for r in node['responses'])
            if 'leadIn' in node:
                actual.append(node['leadIn']['text'])
    require(Counter(quotes) == Counter(actual), 'missing, duplicated, or altered approved reply')
    return len(actual)


def render_review(data):
    lines = ['# Valen dialogue matrix v1 — generated review', '',
             '<!-- Generated by Tools/Scripts/valen_matrix.py. Edit the canonical JSON, not this file. -->', '',
             'Editorial preparation only. No CK candidate, import, runtime or xEdit validation is asserted.', '',
             'Authoring decisions:', '']
    for decision in data['authoringDecisions']:
        lines.append(f"- **{decision['status']}** `{decision['id']}`: {decision['reason']}")
    lines += ['', '## Nodes', '']
    for node in data['nodes']:
        lines += [f"### {node['id']}", '', f"- Category: `{node['category']}`; kind: `{node['kind']}`; status: `{node['state']}`.",
                  f"- Parent: `{node['parent']}`; return: `{node['returnTo']}`.",
                  f"- Scope: `{node['scope']}`; effect: `{node['effect']}`; repeatability: `{node['repeatability']}`.",
                  '- Conditions: `' + json.dumps(node['conditions'], ensure_ascii=False) + '`.',
                  '- Followups: ' + ', '.join(f"[{id_}](#{id_.lower()})" for id_ in node['followups']) + '.', '']
        if node['playerText']:
            lines += ['**Player:** ' + node['playerText'], '']
        responses = ([node['leadIn']] if 'leadIn' in node else []) + node['responses']
        for response in responses:
            lines += [f"**{response['id']}** — {len(response['segments'])} sentence segments.", '',
                      'Conditions: `' + json.dumps(response['conditions'], ensure_ascii=False) + '`.', '',
                      f"Direction: {response['intent']}; {response['emotion']}; intensity {response['intensity']}.", '']
            lines.extend('> ' + line if line else '>' for line in response['text'].splitlines())
            lines.append('')
    return '\n'.join(lines).rstrip('\n') + '\n'


def render_table(data, markdown=False):
    rows = list(survivor_rows(data))
    if not markdown:
        out = io.StringIO(newline='')
        writer = csv.DictWriter(out, fieldnames=list(rows[0]), lineterminator='\n')
        writer.writeheader()
        writer.writerows(rows)
        return out.getvalue()
    lines = ['# Survivor condition matrix v1 — generated review', '',
             '<!-- Generated from VALEN_DIALOGUE_MATRIX_V1.json; not an xEdit or runtime attestation. -->', '',
             'States: 0 Uninitialized; 1 WoundedInCave; 2 CapturedInKeep; 3 Freed; 4 Departed.', '',
             'The generic subject is **Nous devons parler de ce que nous avons trouvé à Helgen.**', '',
             '`automatic`: selected response after clicking that subject. `explicit`: generic response unavailable; use the separately approved first-report/navigation path. First discovery reports are local Say Once. `uncovered`: physically consistent but no safe approved reply; authoring is blocked. `incoherent`: fail closed until the facts and physical state agree. No actor name/state may be inferred from the other actor.', '',
             'The both-rescued topic is separately eligible for exactly (3,3), (3,4), (4,3), (4,4), with both discoveries known. This does not equate Freed with Departed in the generic selection.', '',
             '| H discovered | R discovered | H state | R state | Generic INFO | Separate explicit report | Both rescued topic | Result |',
             '|---|---|---|---|---|---|---|---|']
    for row in rows:
        lines.append('| ' + ' | '.join(str(row[key]) for key in list(row)[:-1]) + ' |')
    lines += ['', '## Coverage', '']
    lines.extend(f'- {key}: {value}' for key, value in survivor_counts(rows).items())
    lines += ['', 'The approved SINGLE_KNOWN_FREED and SINGLE_KNOWN_DEPARTED replies cover one known survivor in state 3 or 4 while the other remains undiscovered in state 0, 1 or 2. Each reply applies symmetrically to Hadvar or Ralof, remains repeatable under the generic subject, and creates no Reported fact. Any new uncovered coherent combination blocks authoring.', '']
    return '\n'.join(lines)


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--generate-review', action='store_true')
    parser.add_argument('--check-generated', action='store_true')
    parser.add_argument('--for-authoring', action='store_true')
    parser.add_argument('--approved-source', type=Path)
    args = parser.parse_args(argv)
    try:
        data = read_json(CORPUS)
        result = validate(data)
        if args.approved_source:
            result['exactApprovedReplies'] = verify_approved_source(data, args.approved_source)
        products = {REVIEW: render_review(data), TABLE: render_table(data), TABLE_REVIEW: render_table(data, True)}
        if args.generate_review:
            for path, content in products.items():
                path.write_text(content, encoding='utf-8', newline='\n')
        if args.check_generated:
            for path, content in products.items():
                require(path.read_text(encoding='utf-8') == content, f'stale generated document: {path.name}')
        if args.for_authoring:
            require_authoring_ready(data)
        result.update(success=True, authoringReady=False if any(d['status'] == 'pending' for d in data['authoringDecisions'])
                      or any(r['result'] == 'uncovered' for r in survivor_rows(data)) else True)
        print(json.dumps(result, ensure_ascii=False))
        return 0
    except (ValueError, KeyError, TypeError, UnicodeError, OSError) as error:
        print(json.dumps({'success': False, 'error': str(error)}, ensure_ascii=False))
        return 1


if __name__ == '__main__':
    sys.exit(main())
