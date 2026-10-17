"""Audit the six native FUZ assets of the bounded Valen C experiment.

This checks serialized files and retained generation evidence, not engine use or
human skip behavior. STATUS owns those runtime results. No asset is generated.
"""
import argparse
import json
from pathlib import Path
import struct

from valen_presentation import Plugin, frozen_corpus, inspect_trial, sha, text
from valen_helgen import require

ROOT = Path(__file__).resolve().parents[2]
RECEIPT = ROOT / 'docs/features/alternate-start/VALEN_VOICE_C_PROTOTYPE.json'
GAME = ROOT / 'GameFiles/Skyrim'


def audit(data_root=GAME):
    receipt = json.loads(RECEIPT.read_text('utf-8'))
    plugin_bytes = (GAME / 'STRE_AlternateStart.esp').read_bytes()
    require(sha(plugin_bytes) == receipt['pluginSha256'], 'Reviewed B plugin changed')
    frozen_corpus()
    plugin = Plugin(plugin_bytes)
    trial = inspect_trial(plugin, GAME)
    require(trial['info'] == receipt['info'] and trial['formKey'] == receipt['formKey'], 'Wrong INFO')
    require(trial['flags'] == 0x0200, 'C requires Force Subtitle and No LIP File off')
    require(not plugin.values(plugin.ids[trial['info']], 'VMAD'), 'Unexpected INFO script timer')
    transcripts = plugin.values(plugin.ids[trial['info']], 'NAM1')
    expected = receipt['assets']
    require(len(expected) == len(trial['assets']) == len(transcripts) == 6, 'Expected six responses on one INFO')
    voice_root = data_root / 'Sound/Voice/STRE_AlternateStart.esp'
    actual = {p.relative_to(data_root).as_posix() for p in voice_root.rglob('*')
              if p.is_file() and p.suffix.lower() in ('.lip', '.xwm', '.fuz')}
    require(actual == {a['path'] for a in expected}, 'Unexpected voice prototype asset scope')
    for wav, transcript, asset in zip(trial['assets'], transcripts, expected):
        relative = Path(wav['path']).with_suffix('.fuz').as_posix()
        require(asset['response'] == wav['response'] and asset['path'] == relative, 'Voice filename mismatch')
        require(wav['sha256'] == asset['wavSha256'], 'Source WAV changed')
        require(sha(text(transcript).encode('utf-8')) == asset['textSha256'], 'Generated transcript changed')
        payload = (data_root / relative).read_bytes()
        require(len(payload) == asset['fuzBytes'] and sha(payload) == asset['fuzSha256'], 'FUZ bytes changed')
        magic, version, lip_size = struct.unpack_from('<4sII', payload)
        require((magic, version, lip_size) == (b'FUZE', 1, asset['lipBytes']), 'Invalid FUZ header')
        lip, xwm = payload[12:12+lip_size], payload[12+lip_size:]
        require(len(lip) > 16 and sha(lip) == asset['lipSha256'], 'Native LIP changed/truncated')
        require(len(xwm) == asset['xwmBytes'] and sha(xwm) == asset['xwmSha256'], 'Native XWM changed')
        require(xwm[:4] == b'RIFF' and xwm[8:12] == b'XWMA', 'Expected native xWMA RIFF')
    return dict(success=True, info=trial['info'], responses=6, fuzFiles=6,
                pluginSha256=sha(plugin_bytes), records=len(plugin.records), recordsChangedFromB=0,
                forceSubtitle=True, noLipFile=False, corpusUnchanged=True,
                generationEvidence='Pinned native tool outputs; see receipt and private decode/unpack logs',
                runtimeC='NOT_TESTED_BY_THIS_AUDIT')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--data-root', type=Path, default=GAME, help='FUZ root; reviewed ESP/WAV read from repository')
    args = parser.parse_args()
    print(json.dumps(audit(args.data_root), indent=2))
