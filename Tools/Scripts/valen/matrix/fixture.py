"""Extend the checked-in, official-xEdit #94 synthetic output; no game payloads."""
from pathlib import Path
import sys
sys.path.insert(0,str(Path(__file__).resolve().parents[1]))
from fixture import header, group, sub, record


def create(root):
    data=root/'Data'
    data.mkdir(parents=True)
    voice=record('VTYP',0x9843A,sub('EDID',b'MaleWarlock\0')+sub('DNAM',b'\0'))
    location=record('LCTN',0x18A4A,sub('EDID',b'HelgenLocation\0')+sub('FULL',b'Fixture Helgen\0'))
    # 00000014 is xEdit's built-in PLYR record. Never redefine it as an ACHR:
    # that collides during the loader, before the authoring script can run.
    (data/'Skyrim.esm').write_bytes(header([],2,1)+group('VTYP',voice)+group('LCTN',location))
    (data/'Update.esm').write_bytes(header(['Skyrim.esm'],flags=1))
    (data/'Dragonborn.esm').write_bytes(header(['Skyrim.esm','Update.esm'],flags=1))
    baseline=(Path(__file__).resolve().parents[1]/'fixtures/authored.esp').read_bytes()
    (data/'STRE_AlternateStart.esp').write_bytes(baseline)
    (root/'before.esp').write_bytes(baseline)


if __name__=='__main__':
    create(Path(sys.argv[1]))
