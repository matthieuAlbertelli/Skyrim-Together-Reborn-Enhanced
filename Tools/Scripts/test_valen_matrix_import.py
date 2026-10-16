"""Import evidence checks on invented xEdit fixtures, never human/production proof."""
from copy import deepcopy
import json
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch

import valen_matrix as corpus
import valen_matrix_assets as assets
from valen_matrix_audit import audit
from valen_matrix_plan import build_plan
from valen_helgen import Plugin
import valen_matrix_import as importer

SOURCE = Path(__file__).parent


def write(path, value):
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(value, indent=2) + '\n', encoding='utf-8')


class MatrixImportEvidenceTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.tmp = tempfile.TemporaryDirectory(prefix='stre-matrix-import-')
        cls.repo = Path(cls.tmp.name)
        cls.root = cls.repo/'_audit/modding'/('a'*32)
        root, data = cls.root, cls.root/'Data'
        data.mkdir(parents=True)
        before = (SOURCE/'valen/fixtures/authored.esp').read_bytes()
        candidate = (SOURCE/'valen/matrix/fixtures/authored.esp').read_bytes()
        for name in ('before.esp', 'repository-before.esp', 'writer-input.esp'):
            (root/name).write_bytes(before)
        (data/importer.PLUGIN).write_bytes(candidate)
        (root/'xedit-validation.log').write_text('SIMULATED HUMAN RECEIPT: invented fixture only\n')
        tool = cls.repo/'xEdit.fixture'
        tool.write_text('invented tool identity')
        original_paths = {importer.PLUGIN: importer.sha(root/'before.esp')}
        qf = audit(before, candidate)['QF']
        for name in (f'Source/Scripts/{qf}.psc', f'Scripts/{qf}.pex'):
            original_paths[name] = 'F'*64
        with patch.object(corpus, 'ROOT', cls.repo):
            assets.generate(root, before, candidate)
            for name in (qf, 'STRE_ValenHelgenObserver'):
                path = data/f'Scripts/{name}.pex'
                path.parent.mkdir(parents=True, exist_ok=True)
                path.write_bytes(bytes.fromhex('fa57c0de03020001') + b'fixture only')
            asset_report = assets.verify(root, before, candidate)
        result = audit(before, candidate)
        plan = build_plan(corpus.read_json(corpus.CORPUS))
        write(root/'candidate-audit.json', result)
        write(root/'record-plan.json', plan)
        (root/'author.ssepas').write_text('invented writer identity; no execution')
        write(root/'review-graph.json', dict(records=result['records'], edges=importer.graph_edges(plan)))
        base_manifest = dict(schemaVersion=3, pluginName=importer.PLUGIN, records=[], notes=[])
        cls.manifest = cls.repo/'docs/features/alternate-start/CK_RECORDS_M7_IMPLEMENTED.json'
        write(cls.manifest, base_manifest)
        proposed = deepcopy(base_manifest)
        proposed['records'] = [dict(signature=r['signature'], editorId=r['editorId'],
                                    expectedLocalFormId='0x'+r['formKey'].split(':')[0])
                               for r in result['records'] if r['action']=='create']
        write(root/'review-proposed-record-manifest.json', proposed)
        companions = asset_report['hashes']
        write(root/'review-private-hashes.json', dict(companions, **{importer.PLUGIN: importer.sha(data/importer.PLUGIN)}))
        write(root/'papyrus-status.json', {'success': True, 'scope': 'simulated fixture'})
        state = dict(Schema=1, State='AwaitingHumanXEdit', Plugin=importer.PLUGIN,
                     Request=dict(Operation='AddValenDialogueMatrixV1'), Git=dict(Head='1'*40, Branch='feat/fixture'),
                     Config=dict(XEditPath=str(tool), RepoRoot=str(cls.repo)),
                     CandidateHash=importer.sha(data/importer.PLUGIN), BeforeHash=importer.sha(root/'before.esp'),
                     PlanHash=importer.sha(root/'record-plan.json'), WriterHash=importer.sha(root/'author.ssepas'),
                     XEditHash=importer.sha(tool), Protected={str(cls.manifest): importer.sha(cls.manifest)},
                     PrivateMasters={}, PrivateOriginals=original_paths, Companions=companions)
        write(root/'session.json', state)
        write(root/'validation.json', dict(Schema=1, Reviewer='SIMULATED fixture', SortMasters=True,
              CheckForErrors=True, SavedAndClosed=True, PluginSha256=state['CandidateHash'],
              XEditSha256=state['XEditHash'], LogSha256=importer.sha(root/'xedit-validation.log')))
        write(root/'review-evidence-hashes.json', {'session.json': importer.sha(root/'session.json')})
        importer.seal_review(root, state['CandidateHash'], state['Git']['Head'])
        cls.state = state
        cls.asset = data/next(iter(companions))

    @classmethod
    def tearDownClass(cls):
        cls.tmp.cleanup()

    def validate(self):
        with patch.object(corpus, 'ROOT', self.repo):
            return importer.validate(self.root)

    def test_valid_review_rereads_actual_graph_classes_pcm_and_exact_delta(self):
        before = {p: importer.sha(self.root/p) for p in importer.REQUIRED}
        result = self.validate()
        self.assertTrue(result['success'])
        self.assertEqual(len(result['Files']), 673)
        self.assertEqual(result['ClassProjections'], 22)
        self.assertEqual(result['Coverage'], dict(automatic=32, explicit=17, incoherent=51, uncovered=0))
        self.assertEqual(before, {p: importer.sha(self.root/p) for p in importer.REQUIRED})

    def test_changed_frozen_evidence_is_rejected(self):
        for name in ('validation.json', 'xedit-validation.log', 'record-plan.json', 'author.ssepas',
                     'candidate-audit.json', 'review-graph.json', 'assets-audit.json',
                     'review-private-hashes.json', 'review-proposed-record-manifest.json'):
            with self.subTest(name=name):
                path = self.root/name
                original = path.read_bytes()
                try:
                    path.write_bytes(original+b' ')
                    with self.assertRaisesRegex(ValueError, 'Frozen file changed'):
                        self.validate()
                finally:
                    path.write_bytes(original)

    def test_candidate_changed(self):
        path = self.root/'Data'/importer.PLUGIN
        original = path.read_bytes()
        try:
            path.write_bytes(original+b'x')
            with self.assertRaisesRegex(ValueError, 'candidate changed'):
                self.validate()
        finally:
            path.write_bytes(original)

    def test_companion_changed_or_missing(self):
        original = self.asset.read_bytes()
        try:
            for missing in (False, True):
                with self.subTest(missing=missing):
                    if missing:
                        self.asset.unlink()
                    else:
                        self.asset.write_bytes(original+b'x')
                    with self.assertRaisesRegex(ValueError, 'Frozen file changed or missing'):
                        self.validate()
        finally:
            self.asset.write_bytes(original)

    def test_extra_files_never_enter_snapshot(self):
        for name in ('Scripts/unapproved.pex', 'SkyrimTogether.esp', 'unapproved.esm',
                     'Sound/Voice/STRE_AlternateStart.esp/MaleWarlock/test.wav.vortex_backup',
                     'SSEEdit Cache/new.refcache'):
            with self.subTest(name=name):
                path = self.root/'Data'/name
                path.parent.mkdir(parents=True, exist_ok=True)
                try:
                    path.write_bytes(b'unexpected')
                    with self.assertRaisesRegex(ValueError, 'Unexpected private file'):
                        self.validate()
                finally:
                    path.unlink()

    def test_protected_file_drift(self):
        original = self.manifest.read_bytes()
        try:
            self.manifest.write_bytes(original+b' ')
            with self.assertRaisesRegex(ValueError, 'Protected original/tool changed'):
                self.validate()
        finally:
            self.manifest.write_bytes(original)

    def test_imported_session_cannot_be_reimported(self):
        path = self.root/'session.json'
        original = path.read_bytes()
        try:
            write(path, dict(self.state, State='Imported'))
            with self.assertRaisesRegex(ValueError, 'not awaiting'):
                self.validate()
        finally:
            path.write_bytes(original)

    def test_receipt_requires_all_true_and_real_identity(self):
        original = importer.read(self.root/'validation.json')
        for key, value in [('SortMasters', False), ('CheckForErrors', 'true'), ('SavedAndClosed', 1),
                           ('Reviewer', ' '), ('PluginSha256', '0'*64), ('XEditSha256', '0'*64),
                           ('LogSha256', '0'*64)]:
            with self.subTest(key=key):
                raw = (self.root/'validation.json').read_bytes()
                try:
                    write(self.root/'validation.json', dict(original, **{key: value}))
                    with self.assertRaises(ValueError):
                        importer.receipt(self.root, self.state)
                finally:
                    (self.root/'validation.json').write_bytes(raw)

    def test_manifest_cannot_broaden_override_policy(self):
        base = importer.read(self.manifest)
        proposed = importer.read(self.root/'review-proposed-record-manifest.json')
        proposed['approvedMasterOverrides'] = ['unapproved']
        with self.assertRaisesRegex(ValueError, 'Manifest policy changed'):
            importer.verify_manifest(base, proposed, importer.read(self.root/'candidate-audit.json'))

    def test_current_binary_auditor_must_reproduce_historical_semantics(self):
        changed = importer.read(self.root/'candidate-audit.json')
        changed['actualChanges']['updated'] += 1
        with patch.object(importer, 'audit', return_value=changed):
            with self.assertRaisesRegex(ValueError, 'semantic reread differs'):
                self.validate()

    def test_current_asset_auditor_must_reproduce_historical_result(self):
        with patch.object(assets, 'verify', return_value={'success': True}):
            with self.assertRaisesRegex(ValueError, 'Asset reread differs'):
                self.validate()

    def test_paths_cannot_escape_or_alias(self):
        for name in ('../out', '/absolute', 'C:/absolute', 'Scripts/../out', 'Scripts//file', 'Scripts\\file'):
            with self.subTest(name=name), self.assertRaises(ValueError):
                importer.safe_child(self.root, name)


if __name__ == '__main__':
    unittest.main()
