"""Invented test payload only; never accepts or rebuilds a production ESP."""
import importlib.util
from pathlib import Path
import sys

SCRIPTS=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(SCRIPTS))
from test_valen_presentation import projected

def create(root):
    spec=importlib.util.spec_from_file_location('matrix_fixture',SCRIPTS/'valen/matrix/fixture.py')
    module=importlib.util.module_from_spec(spec); spec.loader.exec_module(module)
    module.create(root)
    # Fixed synthetic #96 fixture, with the already tested typography projection.
    # This is only test setup; the navigation output is serialized by xEdit.
    baseline=projected('A')
    (root/'before.esp').write_bytes(baseline)
    (root/'Data/STRE_AlternateStart.esp').write_bytes(baseline)

if __name__=='__main__': create(Path(sys.argv[1]))
