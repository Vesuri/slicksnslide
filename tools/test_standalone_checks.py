"""Host checks for isolated release diagnostic includes; no emulator needed."""
from pathlib import Path
import tempfile
import unittest
from test_standalone_release import load_checks, ROOT

class DiagnosticIncludes(unittest.TestCase):
    def test_nested_relative_includes_and_dump_paths(self):
        with tempfile.TemporaryDirectory() as directory:
            root=Path(directory)
            (root/'nested').mkdir()
            (root/'main.gdb').write_text('set $a=1\nsource nested/child.gdb\ncontinue\n')
            (root/'nested/child.gdb').write_text('source "../leaf.gdb" # relative include\nset $b=2\n')
            (root/'leaf.gdb').write_text('dump binary memory .run/check/data 0 1\n')
            result=load_checks(root/'main.gdb')
            self.assertNotIn('source ',result)
            self.assertLess(result.index('set $a'),result.index('dump binary'))
            self.assertLess(result.index('dump binary'),result.index('set $b'))
            self.assertLess(result.index('set $b'),result.index('continue'))

    def test_cycle_and_dynamic_source_rejected(self):
        with tempfile.TemporaryDirectory() as directory:
            path=Path(directory)/'cycle.gdb'
            path.write_text('source cycle.gdb\n')
            with self.assertRaises(ValueError):load_checks(path)
            path.write_text('source $unresolved\n')
            with self.assertRaises(ValueError):load_checks(path)

    def test_current_save_gate_includes_resident_assertions(self):
        checks=load_checks(ROOT/'amiga/diag_championship_save.gdb')
        self.assertIn('SAVED_PICKER_CLOSE_WITH_DISPLAY_RELEASED',checks)
        self.assertIn('NATIVE_CHAMPIONSHIP_MENU_SAVE_EXIT_OK',checks)
        self.assertNotIn('source diag_saved_resident.gdb',checks)

if __name__=='__main__':unittest.main()
