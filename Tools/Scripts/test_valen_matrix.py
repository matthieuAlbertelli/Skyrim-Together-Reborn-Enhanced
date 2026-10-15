"""Editorial/condition tests for #96. No engine/runtime validation is implied."""
from copy import deepcopy
import itertools
import json
from pathlib import Path
import tempfile
import unittest

import valen_matrix as matrix
from valen_helgen import Plugin

DATA = matrix.read_json(matrix.CORPUS)


class MatrixEditorialTests(unittest.TestCase):
    def test_schema_corpus_and_all_three_generated_reviews_agree(self):
        report = matrix.validate(DATA)
        self.assertEqual((report['nodes'], report['responsesIncludingLeadIn'], report['classes']), (133, 144, 21))
        for path, expected in ((matrix.REVIEW, matrix.render_review(DATA)),
                               (matrix.TABLE, matrix.render_table(DATA)),
                               (matrix.TABLE_REVIEW, matrix.render_table(DATA, True))):
            self.assertEqual(path.read_text(encoding='utf-8'), expected)

    def test_frozen_editorial_fingerprint(self):
        # Approval pin is deliberately independent of the editable JSON's hash.
        self.assertEqual(matrix.editorial_hash(DATA), '9B29AFABA5E75CF9436817AA77619B3408DB9530D5884F896E05C94EA122D4F5')
        changed = deepcopy(DATA)
        next(n for n in changed['nodes'] if n['id'] == 'VALEN_SELF_010')['responses'][0]['text'] += ' '
        with self.assertRaisesRegex(ValueError, 'frozen editorial text'):
            matrix.validate(changed)

    def test_original_approved_corpus_is_unchanged_by_the_two_additions(self):
        original = deepcopy(DATA)
        amended = matrix.approved_amendment_nodes(DATA)
        original['nodes'] = [n for n in original['nodes'] if n['id'] not in amended]
        # This fingerprint was verified against all 140 reply blocks in the
        # original attachment at 414556a9, before the two approved additions.
        self.assertEqual(matrix.editorial_hash(original), '4574A7EB686DDE79F123113EE8D7796B19C9CB9A26F0B85FAC5A6364FA184452')

    def test_legacy_acceptance_and_news_keep_exact_esp_text(self):
        plugin = Plugin((matrix.ROOT / 'GameFiles/Skyrim/STRE_AlternateStart.esp').read_bytes())
        nodes = {n['id']: n for n in DATA['nodes']}
        for semantic, record in (('VALEN_HELGEN_ACCEPT', 'Accept'), ('VALEN_HELGEN_NEWS', 'News')):
            prompt = plugin.one(plugin.ids['STRE_DIAL_ValenHelgen_' + record], 'FULL').rstrip(b'\0').decode('cp1252')
            replies = [raw.rstrip(b'\0').decode('cp1252') for raw in plugin.values(plugin.ids['STRE_INFO_ValenHelgen_' + record], 'NAM1')]
            self.assertEqual(nodes[semantic]['playerText'], prompt)
            self.assertEqual(nodes[semantic]['responses'][0]['text'], '\n\n'.join(replies))

    def test_class_roster_has_exactly_the_21_product_ids(self):
        roster = (matrix.ROOT / 'docs/features/alternate-start/CLASS_ROSTER_V1.md').read_text(encoding='utf-8')
        import re
        self.assertEqual({c['id'] for c in DATA['classes']}, set(re.findall(r'`(class\.[a-z]+)`', roster)))

    def test_missing_and_duplicate_class_conditions_are_rejected(self):
        changed = deepcopy(DATA)
        variants = next(n for n in changed['nodes'] if n['id'] == 'VALEN_MENU_RECRUIT')['responses']
        variants[1]['conditions'] = deepcopy(variants[0]['conditions'])
        with self.assertRaisesRegex(ValueError, 'non-exclusive class'):
            matrix.validate(changed)

    def test_stage_boundary_overlap_is_rejected(self):
        changed = deepcopy(DATA)
        changed['helgenMenuLabels'][0]['maximumStage'] = 10
        with self.assertRaisesRegex(ValueError, 'stage labels: 10'):
            matrix.validate(changed)

    def test_reserved_alduin_cannot_enter_approved_graph(self):
        changed = deepcopy(DATA)
        changed['nodes'][0]['followups'].append('VALEN_DRAGONS_ALDUIN_RESERVED')
        with self.assertRaisesRegex(ValueError, 'non-approved node reachable'):
            matrix.validate(changed)

    def test_invalid_followup_return_cycle_and_unreachable_approved_node_fail(self):
        for mutation, message in (
            (lambda d: d['nodes'][0]['followups'].append('VALEN_MISSING'), 'unknown followup'),
            (lambda d: d['nodes'][1].update(returnTo=d['nodes'][1]['id']), 'return cycle'),
            (lambda d: d['nodes'][0]['followups'].remove('VALEN_MENU_SELF'), 'not fully reachable'),
        ):
            changed = deepcopy(DATA)
            mutation(changed)
            with self.subTest(message=message), self.assertRaisesRegex(ValueError, message):
                matrix.validate(changed)

    def test_only_acceptance_can_mutate_shared_state(self):
        changed = deepcopy(DATA)
        changed['nodes'][1].update(effect='AcceptHelgenStage10', scope='SharedQuest')
        with self.assertRaisesRegex(ValueError, 'unauthorized dialogue effect'):
            matrix.validate(changed)

    def test_sentence_segmentation_cannot_change_or_cut_words(self):
        for segments, message in ((['replacement'], 'segmentation changed text'), (None, 'split inside a sentence')):
            changed = deepcopy(DATA)
            response = next(n for n in changed['nodes'] if n['id'] == 'VALEN_SELF_010')['responses'][0]
            response['segments'] = segments or [response['text'][:3], response['text'][3:]]
            with self.subTest(message=message), self.assertRaisesRegex(ValueError, message):
                matrix.validate(changed)

    def test_unknown_fields_schema_keywords_and_duplicate_json_keys_fail(self):
        changed = deepcopy(DATA)
        changed['arbitraryXEditScript'] = 'forbidden'
        with self.assertRaisesRegex(ValueError, 'unexpected fields'):
            matrix.validate(changed)
        with self.assertRaisesRegex(ValueError, 'unsupported schema keywords'):
            matrix.validate_schema_definition({'type': 'string', 'unimplementedKeyword': True})
        with tempfile.TemporaryDirectory() as folder:
            path = Path(folder) / 'duplicate.json'
            path.write_text('{"node":1,"node":2}', encoding='utf-8')
            with self.assertRaisesRegex(ValueError, 'duplicate JSON key'):
                matrix.read_json(path)

    def test_boolean_is_not_a_numeric_class_projection(self):
        changed = deepcopy(DATA)
        changed['classes'][0]['projection'] = True
        with self.assertRaisesRegex(ValueError, 'expected'):
            matrix.validate(changed)


class SurvivorMatrixTests(unittest.TestCase):
    def select(self, hs, rs, hd=True, rd=True):
        return matrix.survivor_selection(DATA, hd, rd, hs, rs)

    def test_all_100_combinations_are_explicitly_accounted_for(self):
        rows = list(matrix.survivor_rows(DATA))
        self.assertEqual(len(rows), 100)
        self.assertEqual(matrix.validate(DATA)['survivorResults'],
                         {'automatic': 32, 'explicit': 17, 'incoherent': 51, 'uncovered': 0})

    def test_distinct_freed_departed_cases_and_specificity(self):
        for hs, rs, suffix in ((4,4,'BOTH_DEPARTED'), (4,2,'ONE_DEPARTED'), (2,4,'ONE_DEPARTED'),
                                (2,2,'BOTH_CAPTURED'), (2,3,'HADVAR_CAPTURED'), (3,2,'RALOF_CAPTURED'),
                                (3,1,'HADVAR_SAFE'), (1,3,'RALOF_SAFE'), (1,2,'ONE_CAPTURED'),
                                (2,1,'ONE_CAPTURED')):
            with self.subTest(hadvar=hs, ralof=rs):
                self.assertEqual(self.select(hs,rs)['node'], 'VALEN_SURVIVOR_' + suffix)
        self.assertEqual(self.select(3,3)['kind'], 'explicit')
        self.assertNotEqual(self.select(3,3)['node'], self.select(4,4)['node'])

    def test_unknown_physical_state_cannot_leak_a_hidden_name(self):
        for hidden_state in (0,1,2):
            self.assertEqual(self.select(2,hidden_state,True,False)['node'], 'VALEN_SURVIVOR_ONE_CAPTURED')
            self.assertEqual(self.select(hidden_state,2,False,True)['node'], 'VALEN_SURVIVOR_ONE_CAPTURED')
            self.assertEqual(self.select(hidden_state,hidden_state,False,False)['node'], 'VALEN_SURVIVOR_NONE')

    def test_all_twelve_known_safe_other_unknown_cases_select_exact_symmetric_reply(self):
        for known, unknown in itertools.product((3,4), (0,1,2)):
            for args in ((known,unknown,True,False), (unknown,known,False,True)):
                with self.subTest(args=args):
                    selected = self.select(*args)
                    self.assertEqual(selected['kind'], 'automatic')
                    suffix = 'FREED' if known == 3 else 'DEPARTED'
                    self.assertEqual(selected['node'], 'VALEN_SURVIVOR_SINGLE_KNOWN_' + suffix)

    def test_approved_single_known_texts_are_exact_repeatable_and_have_no_effect(self):
        ids = matrix.approved_amendment_nodes(DATA)
        self.assertEqual(ids, {'VALEN_SURVIVOR_SINGLE_KNOWN_FREED', 'VALEN_SURVIVOR_SINGLE_KNOWN_DEPARTED'})
        self.assertEqual(DATA['source']['amendments'][0]['sha256'], '4EBDDAEBD8FEDDBA5EA7C368012567D87F47ED37525150C452081FCA1A62B746')
        for node in DATA['nodes']:
            if node['id'] in ids:
                self.assertEqual(node['playerText'], DATA['survivors']['genericPlayerText'])
                self.assertEqual(node['route'], 'generic-survivor-topic')
                self.assertEqual((node['repeatability'],node['effect'],node['scope']), ('Repeatable','None','LocalPresentation'))
                text = node['responses'][0]['text'].casefold()
                for forbidden in ('hadvar','ralof','l’autre','un autre','deux','reported'):
                    self.assertNotIn(forbidden, text)

    def test_new_replies_never_replace_both_known_or_incoherent_cases(self):
        for row in matrix.survivor_rows(DATA):
            if row['genericINFO'].startswith('VALEN_SURVIVOR_SINGLE_KNOWN_'):
                self.assertNotEqual(row['HadvarDiscovered'], row['RalofDiscovered'])
                known = row['HadvarState'] if row['HadvarDiscovered'] else row['RalofState']
                unknown = row['RalofState'] if row['HadvarDiscovered'] else row['HadvarState']
                self.assertIn(unknown, (0,1,2))
                self.assertEqual(known, 3 if row['genericINFO'].endswith('_FREED') else 4)

    def test_generic_reply_cannot_become_a_say_once_discovery_report(self):
        changed = deepcopy(DATA)
        next(n for n in changed['nodes'] if n['id']=='VALEN_SURVIVOR_SINGLE_KNOWN_FREED')['repeatability']='OncePerLocalSave'
        with self.assertRaisesRegex(ValueError, 'generic survivor reply must be repeatable'):
            matrix.require_authoring_ready(changed)

    def test_overlapping_conditions_fail_even_when_order_would_choose_one(self):
        changed = deepcopy(DATA)
        changed['survivors']['rules'].append(deepcopy(changed['survivors']['rules'][0]))
        with self.assertRaisesRegex(ValueError, 'overlapping survivor conditions'):
            matrix.validate(changed)

    def test_authoring_passes_only_with_complete_unambiguous_conditions(self):
        matrix.require_authoring_ready(DATA)
        # Remove one symmetric case: the reply is still used on the other side,
        # so this must detect a coverage hole, not merely an unused node.
        for missing in ('only-known-hadvar-freed','only-known-ralof-departed','only-known-hadvar-wounded'):
            changed = deepcopy(DATA)
            changed['survivors']['rules'] = [r for r in changed['survivors']['rules'] if r['id'] != missing]
            with self.subTest(missing=missing), self.assertRaisesRegex(ValueError, 'AUTHORING_BLOCKED: 3'):
                matrix.require_authoring_ready(changed)

    def test_both_rescued_topic_uses_four_explicit_freed_departed_pairs(self):
        rows = [row for row in matrix.survivor_rows(DATA) if row['bothRescuedTopic']]
        self.assertEqual({(r['HadvarState'],r['RalofState']) for r in rows}, {(3,3),(3,4),(4,3),(4,4)})
        self.assertTrue(all(r['HadvarDiscovered'] and r['RalofDiscovered'] for r in rows))

    def test_transport_source_sends_event_stage_and_does_not_reject_lower_uncompleted_stage(self):
        client = (matrix.ROOT/'Code/client/Services/Generic/QuestService.cpp').read_text()
        engine = (matrix.ROOT/'Code/client/Games/Skyrim/Forms/TESQuest.cpp').read_text()
        server = (matrix.ROOT/'Code/server/Services/QuestService.cpp').read_text()
        self.assertIn('stageId = apEvent->stageId', client)
        self.assertIn('notify.Stage = message.Stage', server)
        native = engine.split('void TESQuest::ScriptSetStage(',1)[1].split('void TESQuest::SetStopped',1)[0]
        self.assertIn('currentStage == stageIndex || IsStageDone(stageIndex)', native)
        self.assertNotIn('currentStage >', native)
        self.assertNotIn('currentStage <', native)

    def test_discovery_fact_contract_is_order_independent_and_idempotent(self):
        # Reference algebra of the proposed completed-stage facts, not execution
        # of Papyrus/the engine. Native transport/save/recovery still need smoke.
        for order in ((11,12), (12,11), (11,12,11,12), (12,12,11,11)):
            completed = {10,20}
            for stage in order:
                completed.add(stage)
            self.assertEqual(completed, {10,11,12,20})
            self.assertEqual(max(completed), 20)
            restored_model = set(json.loads(json.dumps(sorted(completed))))
            self.assertEqual(restored_model, completed)


if __name__ == '__main__':
    unittest.main()
