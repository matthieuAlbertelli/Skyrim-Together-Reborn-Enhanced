"""Validate the frozen human-reviewed #96 bundle; never write an ESP or promote data.

--seal-review pins an existing explicit human receipt and historical evidence once.
It is a checksum inventory, not a substitute for human review or a signed receipt.
Normal invocation is read-only, including the binary and PCM reread.
"""
from __future__ import annotations

import argparse
from collections import Counter
import hashlib
import json
from pathlib import Path, PurePosixPath
import re

import valen_matrix_assets as assets
from valen_matrix_audit import audit
from valen_helgen import Plugin, require

PLUGIN = 'STRE_AlternateStart.esp'
SEAL = 'import-evidence.json'
REQUIRED = {'session.json', 'record-plan.json', 'author.ssepas', 'candidate-audit.json',
            'review-graph.json', 'review-private-hashes.json', 'assets.json', 'assets-audit.json',
            'review-proposed-record-manifest.json', 'review-evidence-hashes.json',
            'validation.json', 'xedit-validation.log', 'before.esp', 'repository-before.esp',
            'writer-input.esp', 'papyrus-status.json'}


def read(path):
    return json.loads(path.read_text('utf-8-sig'))


def sha(path):
    digest = hashlib.sha256()
    with path.open('rb') as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b''):
            digest.update(block)
    return digest.hexdigest().upper()


def safe_child(root, relative):
    require(isinstance(relative, str) and '\\' not in relative and ':' not in relative,
            'Unsafe relative path')
    parts = PurePosixPath(relative).parts
    require(parts and not relative.startswith('/') and all(p not in ('.', '..') for p in parts)
            and '/'.join(parts) == relative, 'Unsafe relative path')
    child = root.joinpath(*parts)
    for path in (child, *child.parents):
        require(not path.is_symlink() and not (hasattr(path, 'is_junction') and path.is_junction()),
                'Linked path refused')
    require(child.resolve().is_relative_to(root.resolve()), 'Path escaped root')
    return child


def verify_hashes(root, hashes):
    for name, expected in hashes.items():
        require(isinstance(expected, str) and re.fullmatch('[A-F0-9]{64}', expected), 'Invalid SHA256')
        path = safe_child(root, name)
        require(path.is_file() and sha(path) == expected, 'Frozen file changed or missing: ' + name)


def receipt(root, state):
    value = read(root/'validation.json')
    require(value['Schema'] == 1 and isinstance(value['Reviewer'], str) and value['Reviewer'].strip(),
            'Missing human reviewer/schema')
    for flag in ('SortMasters', 'CheckForErrors', 'SavedAndClosed'):
        require(value[flag] is True, 'Human check not attested: ' + flag)
    require(value['PluginSha256'] == state['CandidateHash'] == sha(root/'Data'/PLUGIN),
            'Human candidate changed')
    require(value['XEditSha256'] == state['XEditHash'] == sha(Path(state['Config']['XEditPath'])),
            'xEdit changed')
    require((root/'xedit-validation.log').stat().st_size > 0 and
            value['LogSha256'] == sha(root/'xedit-validation.log'), 'Human log changed')
    return value


def seal_review(root, expected_candidate, expected_head):
    """Call only after explicit human review. Preserve every existing evidence file."""
    state = read(root/'session.json')
    require(state['State'] == 'AwaitingHumanXEdit' and state['Git']['Head'] == expected_head
            and state['CandidateHash'] == expected_candidate, 'Review identity mismatch')
    receipt(root, state)
    historical = read(root/'review-evidence-hashes.json')
    verify_hashes(root, historical)
    names = REQUIRED | historical.keys()
    hashes = {name: sha(safe_child(root, name)) for name in sorted(names)}
    # Human xEdit can leave refcaches. Pin those exact non-import files separately;
    # any later extra file, even a cache, is rejected. Never put them in the delta.
    caches = {}
    directory = root/'Data'/'SSEEdit Cache'
    if directory.exists():
        for path in directory.rglob('*'):
            if path.is_file():
                name = path.relative_to(root/'Data').as_posix()
                require(re.fullmatch(r'SSEEdit Cache/[^/]+\.refcache', name), 'Unexpected cache file')
                caches[name] = sha(safe_child(root/'Data', name))
    value = dict(Schema=1, PreparationHead=expected_head, CandidateSha256=expected_candidate,
                 Files=hashes, ExcludedCaches=caches)
    with (root/SEAL).open('x', encoding='utf-8') as stream:
        json.dump(value, stream, indent=2)
        stream.write('\n')
    return dict(success=True, evidenceFiles=len(hashes), excludedCaches=len(caches))


def graph_edges(plan):
    edges = []
    for r in plan['records']:
        def link(target, kind):
            edges.append(dict(**{'from': r['edid'], 'to': target}, kind=kind))
        if r['signature'] in ('DLBR', 'DIAL'):
            link('STRE_QUEST_ValenDialogue', 'owner')
            link(r['start'] if r['signature'] == 'DLBR' else r['branch'],
                 'start' if r['signature'] == 'DLBR' else 'branch')
        elif r['signature'] == 'INFO':
            link(r['topic'], 'parent-topic')
            for target in r['links']:
                link(target, 'TCLT')
    return edges


def verify_manifest(before, proposed, audit_result):
    # Only the records array and explanatory notes may grow. Existing constraints,
    # spell expectations and vanilla override allowlists must remain identical.
    require({k:v for k,v in before.items() if k not in ('records', 'notes')} ==
            {k:v for k,v in proposed.items() if k not in ('records', 'notes')}, 'Manifest policy changed')
    old = before['records']
    require(proposed['records'][:len(old)] == old, 'Existing manifest records changed')
    additions = proposed['records'][len(old):]
    expected = {r['editorId']: r for r in audit_result['records'] if r['action'] == 'create'}
    require(len(additions) == len(expected) and len({r['editorId'] for r in additions}) == len(expected),
            'Manifest additions mismatch')
    for r in additions:
        require(set(r) == {'signature', 'editorId', 'expectedLocalFormId'}, 'Unexpected manifest field')
        e = expected.get(r['editorId'])
        require(e and r['signature'] == e['signature'] and
                int(r['expectedLocalFormId'], 16) == int(e['formKey'].split(':')[0], 16),
                'Manifest FormKey mismatch')


def validate(root, *, promoted=False):
    current, seal = read(root/'session.json'), read(root/SEAL)
    state = read(root/'preparation-session.json') if promoted else current
    if promoted:
        require(current['State'] == 'ImportFailed', 'Only an interrupted import can be finalized')
        require(all(current[k] == v for k,v in state.items() if k != 'State'), 'Historical session fields changed')
        require(current['PreparationHead'] == state['Git']['Head'] and
                current['ImportEvidenceHash'] == sha(root/SEAL) and
                current['ImportInventoryHash'] == sha(root/'import-inventory.json'), 'Interrupted import evidence changed')
        require(read(root/'import-result.json')['ExitCode'] != 0, 'Missing interrupted importer result')
    require(state['Schema'] == 1 and state['State'] == 'AwaitingHumanXEdit', 'Session not awaiting human import')
    require(state['Plugin'] == PLUGIN and state['Request']['Operation'] == 'AddValenDialogueMatrixV1',
            'Wrong matrix operation/plugin')
    require(seal['Schema'] == 1 and REQUIRED <= seal['Files'].keys(), 'Incomplete import evidence')
    require(seal['PreparationHead'] == state['Git']['Head'] and
            seal['CandidateSha256'] == state['CandidateHash'], 'Frozen provenance changed')
    def original_names(hashes):
        return {('preparation-session.json' if promoted and k == 'session.json' else k):v for k,v in hashes.items()}
    verify_hashes(root, original_names(seal['Files']))
    verify_hashes(root, original_names(read(root/'review-evidence-hashes.json')))
    receipt(root, state)
    require(sha(root/'record-plan.json') == state['PlanHash'] and
            sha(root/'author.ssepas') == state['WriterHash'], 'Plan/writer changed')
    for name in ('before.esp', 'repository-before.esp', 'writer-input.esp'):
        require(sha(root/name) == state['BeforeHash'], 'Backup changed')
    promoted_paths = {}
    if promoted:
        delta = {p:h for p,h in state['Companions'].items() if state['PrivateOriginals'].get(p) != h}
        delta[PLUGIN] = state['CandidateHash']
        for base in (root/'validated', Path(state['Config']['DataRoot']), Path(state['Config']['RepoRoot'])/'GameFiles/Skyrim'):
            verify_hashes(base, delta)
            promoted_paths.update({safe_child(base, p).resolve():h for p,h in delta.items()})
        actual = {p.relative_to(root/'validated').as_posix() for p in (root/'validated').rglob('*') if p.is_file()}
        require(actual == set(delta), 'Unexpected interrupted snapshot file')
    for group in ('Protected', 'PrivateMasters'):
        for path, expected in state[group].items():
            expected = promoted_paths.get(Path(path).resolve(), expected)
            require(sha(Path(path)) == expected, 'Protected original/tool changed: ' + path)
    plan = read(root/'record-plan.json')
    result = audit((root/'before.esp').read_bytes(), (root/'Data'/PLUGIN).read_bytes(), plan)
    require(result == read(root/'candidate-audit.json'), 'Binary semantic reread differs')
    require(result['coverage'] == dict(automatic=32, explicit=17, incoherent=51, uncovered=0),
            'Survivor coverage changed')
    graph = read(root/'review-graph.json')
    edge_key = lambda e: (e['from'], e['to'], e['kind'])
    require(graph['records'] == result['records'] and
            Counter(map(edge_key, graph['edges'])) == Counter(map(edge_key, graph_edges(plan))),
            'Reviewed graph differs')
    data = root/'Data'
    companions, originals = state['Companions'], state['PrivateOriginals']
    approved = read(root/'assets-audit.json')
    require(approved['success'] is True and len(approved['paths']) == len(set(approved['paths']))
            and set(approved['paths']) == set(approved['hashes']), 'Invalid asset inventory')
    expected = (set(originals) - {PLUGIN}) | set(approved['paths'])
    require(set(companions) == expected, 'Companion inventory differs')
    require(read(root/'review-private-hashes.json') == dict(companions, **{PLUGIN: state['CandidateHash']}),
            'Reviewed private inventory differs')
    for relative in companions:
        require(not relative.lower().endswith('.vortex_backup') and re.fullmatch(
            r'(Source/Scripts/[^/]+\.psc|Scripts/[^/]+\.pex|SEQ/STRE_AlternateStart\.seq|'
            r'Sound/Voice/STRE_AlternateStart\.esp/MaleWarlock/[^/]+\.wav)', relative),
            'Forbidden companion path: ' + relative)
    verify_hashes(data, companions)
    caches = seal['ExcludedCaches']
    require(all(re.fullmatch(r'SSEEdit Cache/[^/]+\.refcache', p) for p in caches), 'Unsafe cache exclusion')
    verify_hashes(data, caches)
    master_names = {Path(p).name for p in state['PrivateMasters']}
    actual = {p.relative_to(data).as_posix() for p in data.rglob('*') if p.is_file()}
    require(actual == set(companions) | {PLUGIN} | master_names | set(caches), 'Unexpected private file')
    # This reuses the sample/timing/header/template checks without rewriting its old report.
    require(assets.verify(root, (root/'before.esp').read_bytes(), (data/PLUGIN).read_bytes(),
                          write_report=False) == approved, 'Asset reread differs')
    delta = {p:h for p,h in companions.items() if originals.get(p) != h}
    require(set(delta) == set(approved['paths']) and all(approved['hashes'][p] == h for p,h in delta.items()),
            'Delta differs from approved generated assets')
    qf = result['QF']
    require({p for p in delta if p in originals} == {f'Source/Scripts/{qf}.psc', f'Scripts/{qf}.pex'},
            'Unexpected existing companion replacement')
    repo = Path(state['Config']['RepoRoot'])
    manifest = 'docs/features/alternate-start/CK_RECORDS_M7_IMPLEMENTED.json'
    verify_manifest(read(repo/manifest), read(root/'review-proposed-record-manifest.json'), result)
    result = dict(success=True, PreparationHead=state['Git']['Head'], PluginSha256=state['CandidateHash'],
                Files=dict(sorted(dict(delta, **{PLUGIN: state['CandidateHash']}).items())),
                CompanionCount=len(companions), ClassProjections=22, Coverage=result['coverage'],
                ActualChanges=result['actualChanges'], ManifestPath=manifest,
                ManifestSha256=sha(root/'review-proposed-record-manifest.json'))
    if promoted:
        require(result == read(root/'import-inventory.json'), 'Interrupted inventory semantic mismatch')
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--root', type=Path, required=True)
    parser.add_argument('--seal-review', action='store_true')
    parser.add_argument('--expected-candidate-sha256')
    parser.add_argument('--expected-preparation-head')
    parser.add_argument('--verify-promoted', action='store_true')
    args = parser.parse_args()
    root = args.root.resolve()
    require(root.parent == (assets.corpus.ROOT/'_audit/modding').resolve(), 'Private session required')
    result = (seal_review(root, args.expected_candidate_sha256, args.expected_preparation_head)
              if args.seal_review else validate(root, promoted=args.verify_promoted))
    print(json.dumps(result))


if __name__ == '__main__':
    main()
